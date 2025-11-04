#ifndef NETWORKING_H
#define NETWORKING_H

#include "sys_headers.h"
#include <asio/asio.hpp>

#define RESP_OK     "OK"
#define RESP_ERR    "ERR"
#define RQ_SYNC     "SRQ"
#define RQ_GET      "GET"


void initConn(json* jsonData, string addr = "127.0.0.1");

void requestTrack(string requestPath, ofstream* fileOutput);





#endif