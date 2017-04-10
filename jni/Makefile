

enable_embed_lua:=true

APP=luahttpd


all:$(APP)

LUA_SRCS= \
	lapi.c lcode.c ldebug.c ldo.c ldump.c lfunc.c lgc.c llex.c lmem.c \
	lobject.c lopcodes.c lparser.c lstate.c lstring.c ltable.c ltm.c  \
	lundump.c lvm.c lzio.c \
	lauxlib.c lbaselib.c ldblib.c liolib.c lmathlib.c loslib.c ltablib.c \
	lstrlib.c loadlib.c linit.c

LUA_OBJS=$(patsubst %.c, %.o, $(LUA_SRCS))
OBJS:=httpd.o doluafile.o httpd-config.o \
	$(addprefix lua-5.1.5/src/, $(LUA_OBJS))


CFLAGS:=-g -Ilua-5.1.5/src

ifeq (true, $(enable_embed_lua))

doluafile.c:gen-lua-scripts.c gen-lua-htdocs.c

LUA_SCRIPT_SYMBOLS=lua_script_src
LUA_HTDOCS_SYMBOLS=lua_htdocs_src

CFLAGS += -DENABLE_EMBEDED_LUA \
		  -DLUA_SCRIPT_SYMBOLS=$(LUA_SCRIPT_SYMBOLS) \
		  -DLUA_HTDOCS_SYMBOLS=$(LUA_HTDOCS_SYMBOLS)

endif


$(APP):$(OBJS)
	$(CC) -o $@ $(OBJS) -lpthread -lm 

%.o:%.c
	$(CC) -g -Ilua-5.1.5/src  -o $@ -c $<


LUA_SCRIPTS_FILES:=lua-scripts/proc-http.lua

LUA_HTDOCS_FILES:=htdocs/test2.lt

LUA2C_PY:=tools/gen-scripts.py


gen-lua-scripts.c:$(LUA_SCRIPTS_FILES) $(LUA2C_PY)
	$(LUA2C_PY) --script $(LUA_SCRIPTS_FILES) --output $@ --symbol $(LUA_SCRIPT_SYMBOLS)

gen-lua-htdocs.c:$(LUA_HTDOCS_FILES) $(LUA2C_PY)
	$(LUA2C_PY) $(LUA_HTDOCS_FILES) --output $@ --symbol $(LUA_HTDOCS_SYMBOLS)

clean:
	rm -f $(OBJS) $(APP) gen-lua-scripts.c gen-lua-htdocs.c

