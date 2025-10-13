#ifndef SYS_HEADERS_H
#define SYS_HEADERS_H


#include <iostream>
#include <list>
#include <vector>
#include <cstdlib>
#include <unistd.h>
#include <fstream>
#include <QWidget>
#include <QApplication>


using namespace std;

extern bool libSet;
extern bool serverMode;

#if defined (__linux__) && !defined (__ANDROID__)

#define PLATFORM_LINUX
#include <filesystem>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>

#endif

#if defined (_WIN32)

#define PLATFORM_WINDOWS
#include <filesystem>

#endif

#if defined (__ANDROID__)

#define PLATFORM_ANDROID
#include <QDirIterator>

#endif

#endif
