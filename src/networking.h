#ifndef NETWORKING_H
#define NETWORKING_H

#include "sys_headers.h"

#define RESP_OK     "OK"
#define RESP_ERR    "ERR"
#define RQ_SYNC     "SRQ"
#define RQ_GET      "GET"


void initConn(json* jsonData, string addr = "127.0.0.1");


#if defined (PLATFORM_LINUX) || defined (PLATFORM_WINDOWS)

#define ASIO_STANDALONE
#include <asio/asio.hpp>



void requestTrack(string requestPath, ofstream* fileOutput);

#endif

#if defined (PLATFORM_ANDROID)

#include <QTcpSocket>
#include <arpa/inet.h>
#include <QByteArray>



void requestTrack(string requestPath, vector<char>& fileBuf);

#endif

#endif