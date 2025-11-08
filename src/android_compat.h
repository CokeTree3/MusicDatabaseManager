#ifndef ANDROID_COMPAT_H
#define ANDROID_COMPAT_H

#include "sys_headers.h"

#if defined (PLATFORM_ANDROID)

int createFileAndroid(string fileName, string relativePath, void* dataBuf, uint64_t dataBufSize);
void requestPermissions();


#endif
#endif