#pragma once
#ifndef CONFIG_H
#define CONFIG_H

#if defined (__linux__) && !defined (__ANDROID__)

#define PLATFORM_LINUX

#endif


#if defined (_WIN32)

#define PLATFORM_WINDOWS

#endif


#if defined (__ANDROID__)

#define PLATFORM_ANDROID

#endif


#define CONNECT_PORT 41845

#endif