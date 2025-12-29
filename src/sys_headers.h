#pragma once
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
#include <sstream>
#include <nlohmann/json.hpp>


using namespace std;
using json = nlohmann::json;

extern bool     libSet;
extern bool     serverMode;
extern string     remoteAddr;


#if defined (PLATFORM_LINUX)

#include <filesystem>

#endif

#if defined (PLATFORM_WINDOWS)

#include <filesystem>

inline std::filesystem::path path(const std::string& p) {
    return std::filesystem::u8path(p);
}


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
