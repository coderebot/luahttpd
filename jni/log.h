#ifndef _HTTPD_LOG_H
#define _HTTPD_LOG_H


#define ENABLE_DEBUG

#ifdef ENABLE_DEBUG
#if __ANDROID__
#include <android/log.h>
#define LOG_TAG "LUAHTTPD"

#define ALOG(TAG, LEVEL, ...) do {__android_log_print(ANDROID_LOG_##LEVEL, TAG, __VA_ARGS__); } while(0)

#define ALOGI(TAG,...) ALOG(TAG, INFO, __VA_ARGS__)
#define ALOGD(TAG,...) ALOG(TAG, DEBUG, __VA_ARGS__)
#define ALOGE(TAG,...) ALOG(TAG, ERROR, __VA_ARGS__)
#define ALOGV(TAG,...) ALOG(TAG, VERBOSE, __VA_ARGS__)


#define DEBUG(...)  ALOGI(LOG_TAG, __VA_ARGS__)

#else

#define DEBUG(...) printf(__VA_ARGS__)

#endif

#else

#define DEBUG(...)

#endif



#endif

