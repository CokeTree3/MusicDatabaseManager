#ifndef SYS_HEADERS_H
#define SYS_HEADERS_H

#include "config.h"

#include <iostream>
#include <list>
#include <vector>
#include <cstdlib>
#include <cstdint>
#include <unistd.h>
#include <fstream>
#include <nlohmann/json.hpp>
#include <QWidget>
#include <QApplication>



using namespace std;
using json = nlohmann::json;

extern bool     libSet;
extern bool     serverMode;
extern string     remoteAddr;


#if defined (PLATFORM_LINUX)

#include <filesystem>
#include <QDirIterator>
#include <QFile>
#include <QDir>
#include <QFileInfo>

#endif

#if defined (PLATFORM_WINDOWS)

#include <filesystem>
#include <QDirIterator>
#include <QFile>
#include <QDir>
#include <QFileInfo>

#endif

#if defined (PLATFORM_ANDROID)

#include <QDirIterator>
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QtAndroid>
#include <QAndroidJniObject>
#include <QAndroidJniEnvironment>

#include "android_compat.h"


#endif

#endif
