#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>

#define LUA_LIB
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

#include "httpd-config.h"
#include "log.h"

#ifdef ENABLE_EMBEDED_LUA
#include "gen-lua-scripts.c"
#include "gen-lua-htdocs.c"

#define LUA_SCRIPT_DATA      LUA_SCRIPT_SYMBOLS ## _data
#define LUA_SCRIPT_DATA_LEN  LUA_SCRIPT_SYMBOLS ## _data_len

#define LUA_HTDOCS_DATA     LUA_HTDOCS_SYMBOLS ## _data
#define LUA_HTDOCS_DATA_LEN LUA_HTDOCS_SYMBOLS ## _data_len
#define LUA_HTDOCS_MAP      LUA_HTDOCS_SYMBOLS ## _map_file
#define LUA_HTDOCS_MAP_COUNT LUA_HTDOCS_SYMBOLS ## _map_file_count

#endif



void cannot_execute(int client);
int get_line(int sock, char *buf, int size);

/**********************************************************************/
/* Execute a lua script.  Will need to set environment variables as
 * appropriate.
 * Parameters: client socket descriptor
 *             path to the CGI script */
/**********************************************************************/
static void setglobal(lua_State *L, const char* name, const char* value) {
    lua_pushstring(L, value);
    lua_setglobal(L, name);
}

static void push_query_string(lua_State* L, const char* query_string, const char* table_name) {
    if (query_string == NULL) {
        return ;
    }
    int tbl;
    char szname[256];
    char value[1024];
    const char* str = query_string;

    lua_newtable(L);
    tbl = lua_gettop(L);

    while (str) {
        int len = 0;
        const char* tend = strchr(str, '=');
        if (!tend)
            break;
        //get name
        len = tend - str;
        strncpy(szname, str, len);
        szname[len] = 0;

        str = tend + 1;
        tend = strchr(str, '&');
        if (tend == NULL)
            len = strlen(str);
        else
            len = tend - str;
        
        strncpy(value, str, len);
        value[len] = 0;

        lua_pushstring(L, value);
        lua_pushstring(L, szname);
        lua_settable(L, -2);

        str = tend ? tend + 1 : NULL;
    }

    lua_setglobal(L, table_name);
}

static int read_line(lua_State* L) {
    FILE* fin = (FILE*)lua_touserdata(L, lua_upvalueindex(1));
    char szbuf[1024];
    char *pbuf = szbuf;
    int idx = 0;
    int max = sizeof(szbuf)- 1;
    int len = 0;

    while (!feof(fin)) {
        char c = fgetc(fin);

        pbuf[idx++] = c;
        if (c == '\n') {
            break;
        }

        if (idx >= max - 2) {
            if (pbuf == szbuf) {
                pbuf = (char*)malloc(idx + sizeof(szbuf));
                strncpy(pbuf, szbuf, idx);
            } else {
                pbuf = (char*)realloc(pbuf, idx + sizeof(szbuf));
            }
            max += sizeof(szbuf);
        }
    }


    pbuf[idx] = '\0';
    //printf("read_line: %s\n", pbuf);

    lua_pushstring(L, pbuf);

    if (pbuf != szbuf) {
        free(pbuf);
    }

    return 1;
}

static int read_all(lua_State* L) {
    FILE* fin = (FILE*)lua_touserdata(L, lua_upvalueindex(1));

    int len = 0;
    int idx = 0;

    const int SIZE = 1024*4096;
    char *pbuf = (char*)malloc(SIZE);
    int   bufleft = SIZE;

    while ((len = fread(pbuf + idx, 1, bufleft, fin)) > 0) {
        bufleft -= len;
        idx += len;
        if (bufleft <= 0) {
            pbuf = (char*)realloc(pbuf, idx + SIZE);
            bufleft = SIZE;
        }
    }

    pbuf[idx] = 0;
    
    lua_pushstring(L, pbuf);

    free(pbuf);

    return 1;
}

static int write_out(lua_State* L) {
    int fd = (int)lua_tointeger(L, lua_upvalueindex(1));
    
    int top = lua_gettop(L);
    int i;

    for (i = 1; i <= top; i++) {
        if (lua_isstring(L, i)) {
            const char* str = lua_tostring(L, i);
            DEBUG("write_out:%s\n", str);
            write(fd, str, strlen(str));
        }
    }
}


static void set_input_function(lua_State* L, FILE* fin) {
    lua_pushlightuserdata(L, fin);
    lua_pushcclosure(L, read_line, 1);
    lua_setglobal(L, "read_line");

    lua_pushlightuserdata(L, fin);
    lua_pushcclosure(L, read_all, 1);
    lua_setglobal(L, "read_all");
}

static void set_output_function (lua_State* L, int fd) {
    lua_pushinteger(L, fd);
    lua_pushcclosure(L, write_out, 1);
    lua_setglobal(L, "write_out");
}

#ifdef ENABLE_EMBEDED_LUA
static int get_htdocs(lua_State* L) {
    const char* src_file = lua_tostring(L, 1);
    const char* str_name = strrchr(str_file, '/');
    int i = 0;
    if (str_name == NULL)
        str_name = src_file;
    else
        str_name ++;

    for (i = 0; i < LUA_HTDOCS_MAP_COUNT; i ++) {
        if (strcmp(LUA_HTDOCS_MAP[i].file_name, str_name) == 0) {
            const char* value = LUA_HTDOCS_DATA + LUA_HTDOCS_MAP[i].start;
            lua_pushlstring(L, value, LUA_HTDOCS_MAP[i].length);
            return 1;
        }
    }

    return 0;
}

#endif


#define lua_valuetype(L, idx) lua_typename(L, lua_type(L, idx))

static int traceback (lua_State *L) {
  if (!lua_isstring(L, 1))  /* 'message' not a string? */
    return 1;  /* keep it intact */

  //printf("lua error:%s", lua_tostring(L, 1));

  lua_getfield(L, LUA_GLOBALSINDEX, "debug");
  if (!lua_istable(L, -1)) {
    lua_pop(L, 1);
    return 1;
  }
  lua_getfield(L, -1, "traceback");
  if (!lua_isfunction(L, -1)) {
    lua_pop(L, 2);
    return 1;
  }
  lua_pushvalue(L, 1);  /* pass error message */
  lua_pushinteger(L, 2);  /* skip this function and traceback */
  lua_call(L, 2, 1);  /* call debug.traceback */
  return 1;
}


static void do_lua_file(const char* path, const char* method, const char* query_string, int fdin, int fdout)
{
    int env_tbl = 0;
    int ret = 0;
    int func_idx = 0;
    int err_idx = 0;
    FILE *fin = fdopen(fdin, "rt");

    lua_State * L = lua_open();
    luaL_openlibs(L);

    lua_newtable(L);

    setglobal(L, "METHOD", method); 
    setglobal(L, "FILE", path);
    setglobal(L, "LUA_SCRIPTS", GetConfig()->lua_scripts);
#ifdef ENABLE_DEBUG
    lua_pushinteger(L, 1);
#else
    lua_pushinteger(L, 0);
#endif
    lua_setglobal(L, "DEBUG");

    push_query_string(L, query_string, "GET");
    
    //push the input/output handle
    set_input_function(L, fin);
    set_output_function(L, fdout);

    //load and do start file  
    //printf("start lua file\n");
    /*if((ret = luaL_loadfile(L, "./lua-scripts/proc-http.lua")) != 0) {
        printf("cannot load lua-scripts/proc-http.lua %d\n", ret);
        goto END;
    }
    func_idx = lua_gettop(L);

    lua_pushcfunction(L, traceback); 

    err_idx = lua_gettop(L);

    if((ret = lua_pcall(L, 0, LUA_MULTRET, err_idx)) != 0) {
        fprintf(stderr, "cannot execute lua function %d", ret);
        goto END;
    }*/


#ifdef ENABLE_EMBEDED_LUA
    {
        lua_pushcfunction(L, (lua_CFunction)get_htdocs);
        lua_setglobal(L, "get_htdocs");
    }
    luaL_loadbuffer(L, LUA_SCRIPT_DATA, LUA_SCRIPT_DATA_LEN, "scripts");
    lua_pcall(L, 0, LUA_MULTRET, 0);
#else
    luaL_dofile(L, "./lua-scripts/proc-http.lua");
#endif

END:
    //printf("end lua file\n");
    lua_close(L);

    fclose(fin);
}


void execute_lua(int client, const char* path,
            const char* method, const char* query_string) {

    int lua_output[2];
    int lua_input[2];

    char buf[1024];
    int numchars = -1;
    pid_t pid;
    int status;

    DEBUG("path=%s, method=%s, query_string=%s\n", path, method, query_string);

    if (pipe(lua_output) < 0) {
        cannot_execute(client);
        return ;
    }

    if (pipe(lua_input) < 0) {
        cannot_execute(client);
    }

    if ((pid = fork()) < 0) {
        cannot_execute(client);
        return ;
    }

    if (pid == 0) {
        close(lua_output[0]);
        close(lua_input[1]);
        do_lua_file(path, method, query_string, lua_input[0], lua_output[1]); 
        close(lua_output[1]);
        close(lua_input[0]);
    } else {
        close(lua_output[1]);
        close(lua_input[0]);
        
        int content_length = 0;
        //write method and header
        while ((numchars = get_line(client, buf, sizeof(buf))) > 0 && strcmp("\n", buf)) {
            DEBUG("read line:%s\n", buf);
            write(lua_input[1], buf, numchars);
            if (strncasecmp(buf, "Content-Length:", sizeof("Content-Length:")-1) == 0) {
                content_length = atoi(buf + sizeof("Content-Length:"));
            }
        }
        write(lua_input[1], "\n",1);

        while(content_length > 0) {
            int len = recv(client, buf, sizeof(buf), 0);
            if (len <= 0) break;

            write(lua_input[1], buf, len);
            content_length -= len;
        }
         
        int nr = 0;
        while ((nr = read(lua_output[0], buf, sizeof(buf)-1)) > 0) {
            buf[nr] = 0;
            DEBUG("get responed(%d):%s\n", nr, buf);
            send(client, buf, nr, 0);
        }
        
        close(lua_output[0]);
        close(lua_input[1]);
        //printf("end lua request\n");
        waitpid(pid, &status, 0);
    }

}




