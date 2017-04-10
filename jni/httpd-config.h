#ifndef HTTP_CONFIG_H
#define HTTP_CONFIG_H

typedef struct _HttpdConfig {
    char lua_scripts[256];
    char htdocs[256];
    int  port;
}HttpdConfig;


HttpdConfig* GetConfig();

void parse_config(int argc, const char* argv[]);

#endif

