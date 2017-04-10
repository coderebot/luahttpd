#LOCAL_PATH is used to locate source files in the development tree.

#the macro my-dir provided by the build system, indicates the path of the current directory

LOCAL_PATH:=$(call my-dir)


#####################################################################

#            build libnflink                    #

#####################################################################

include $(CLEAR_VARS)


LOCAL_C_INCLUDES := $(LOCAL_PATH)

LOCAL_SRC_FILES:= httpd.c doluafile.c


LOCAL_C_INCLUDES += $(LOCAL_PATH)

LUA_SRCS= \
	lapi.c lcode.c ldebug.c ldo.c ldump.c lfunc.c lgc.c llex.c lmem.c \
	lobject.c lopcodes.c lparser.c lstate.c lstring.c ltable.c ltm.c  \
	lundump.c lvm.c lzio.c \
	lauxlib.c lbaselib.c ldblib.c liolib.c lmathlib.c loslib.c ltablib.c \
	lstrlib.c loadlib.c linit.c

LOCAL_SRC_FILES += $(addprefix lua-5.1.5/src/, $(LUA_SRCS))
LOCAL_C_INCLUDES += $(LOCAL_PATH)/lua-5.1.5/src

LOCAL_MODULE:=luahttpd


LOCAL_LDLIBS:=-llog -lm

#include $(BUILD_SHARED_LIBRARY)

include $(BUILD_EXECUTABLE)
