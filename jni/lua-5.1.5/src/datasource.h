#ifndef DATA_SOURCE_H
#define DATA_SOURCE_H

enum {
    FUNCTION = 0,
};

typedef int (*Register)(void*, const char* name, void *data, int type);
typedef int (*DataSourceOnLoad)(Register, void*);

typedef struct _Arguments Arguments;
struct _Arguments {
    int (*intArg)(Arguments*, int idx);
    double (*doubleArg)(Arguments*, int idx);
    const char* (*strArg)(Arguments*, int idx);

    void (*pushIntRet)(Arguments*, int value);
    void (*pushFloatRet)(Arguments*, double value);
    void (*pushStrRet)(Arguments*, const char* str);
    void (*commitReturn)(Arguments*);
};

typedef int (*PFunction)(Arguments *pargs);


#endif

