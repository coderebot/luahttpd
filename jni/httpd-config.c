
#include "httpd-config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static HttpdConfig _httpd_config;

HttpdConfig* GetConfig() {
    return &_httpd_config;
}

void parse_config(int argc, const char* argv[]) {
    int i;
    
#if __ANDROID__
#define LUA_HTTPD_PATH "/sdcard/lua-httpd"
    sprintf(_httpd_config.lua_scripts, "%s/lua-scripts", LUA_HTTPD_PATH);
    sprintf(_httpd_config.htdocs, "%s/htdocs", LUA_HTTPD_PATH);
#else
    char szPwd[256];
    char *str;
    strcpy(szPwd, argv[0]);
    str = strrchr(szPwd, '/');
    if (str) {
        *str = '\0';
    } else {
        strcpy(szPwd, ".");
    }

    //set default values
    
    sprintf(_httpd_config.lua_scripts, "%s/lua-scripts", szPwd);
    sprintf(_httpd_config.htdocs, "%s/htdocs", szPwd);
#endif
    _httpd_config.port = 5533;

    i = 1;
    while(i < argc) {
        if (strcmp(argv[i], "--port") == 0) {
            _httpd_config.port = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--htdocs") == 0) {
            strcpy(_httpd_config.htdocs, argv[++i]);
        } else if (strcmp(argv[i], "--lua-scripts") == 0) {
            strcpy(_httpd_config.lua_scripts, argv[++i]);
        }

        ++i;
    }

}


