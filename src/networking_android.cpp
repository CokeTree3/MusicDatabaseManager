#include "networking.h"

#if defined (PLATFORM_ANDROID)

namespace {
    QTcpSocket sock;
    //std::unique_ptr<tcp::socket> sock;
}

void sendData(const void *data, uint64_t dataSize) {

    if(sock.state() != QAbstractSocket::ConnectedState){
        cout << "not connected" << endl;
        //throw system_error();
    }

    uint64_t request_length = htonl(dataSize);

    QByteArray dataBuf;
    dataBuf.push_back(QByteArray::fromRawData(reinterpret_cast<const char *>(&request_length), sizeof(request_length)));           // size_t writes diff data on win vs linux
    dataBuf.push_back(QByteArray(static_cast<const char*>(data), dataSize));

    int res = sock.write(dataBuf);
    cout << "sent " << res << endl;
    if(res < 1) throw std::exception();   // better error handling
}

void readDataSize(uint64_t* dataSize){
    if(!sock.isOpen() || sock.state() != QAbstractSocket::ConnectedState){
        throw std::exception();
    }

    if (!sock.waitForReadyRead(5000)) throw std::exception(); 
    
    int res = sock.read(reinterpret_cast<char*>(dataSize), sizeof(*dataSize));
    if(res == sizeof(*dataSize)){
        uint64_t sizeConverted = ntohl(*dataSize);
        *dataSize = sizeConverted;
    }else {
        throw std::exception();
    }
}

void readData(void* dataBuf, uint64_t dataSize){
    if(!sock.isOpen() || sock.state() != QAbstractSocket::ConnectedState){
        throw std::exception();
    }

    uint64_t totalRead = 0;
    while (totalRead < dataSize) {
        if (!sock.waitForReadyRead(5000)) throw std::exception();
        QByteArray buf = sock.read(dataSize - totalRead);

        memcpy(static_cast<char*>(dataBuf) + totalRead, buf.constData(), buf.size());

        totalRead += buf.size();
    }
}

void initConn(json* jsonData, string addr) {
    if(serverMode){
        // init server worker
        cout << "Server operation not supported\n";
        
    }else{
        // init connection
        cout << "client run\n";
        try {
            sock.connectToHost(QString::fromStdString(addr), (unsigned short)CONNECT_PORT);
            sock.waitForConnected(5000);
            if(jsonData == nullptr){
                throw;
            }
            if(sock.isOpen()){
                cout << "init is open\n"; 
            }else{
                throw std::exception();
            }
            sendData(static_cast<const void*>(RQ_SYNC), sizeof(RQ_SYNC));

            uint64_t jsonSize;
            readDataSize(&jsonSize);

            char* jsonBuf = static_cast<char*>(malloc(jsonSize));

            if(jsonBuf == nullptr) throw bad_alloc();

            readData(jsonBuf, jsonSize);

            if(jsonBuf[0] == 'E' && jsonBuf[1] == 'R' && jsonBuf[2] == 'R'){
                cout << "Error requesting server library" << endl;
            }else{
                vector<char> jsonRecv(jsonBuf, jsonBuf + jsonSize);
                json test = json::from_bson(jsonRecv);
                *jsonData = std::move(test);
                
                cout << "Received server json Library\n";
            }
            
            free(jsonBuf);

        }catch (std::exception &e){
            cout << "net error " << e.what() << endl;
        }
    }
}

void requestTrack(string requestPath, vector<char>& fileData){
    if(!sock.isOpen()){
        return;                                                                         // try to reconnect
    }
    cout << "requesting " << requestPath << endl;
    try{

        char* request = new char[3 + requestPath.length()];

        if(request == nullptr) throw bad_alloc();

        memcpy(request, RQ_GET, 3);
        memcpy(request + 3, requestPath.data(), requestPath.length());
        sendData(static_cast<const void*>(request), 3 + requestPath.length());

        uint64_t fileSize = 0;
        readDataSize(&fileSize);
        if(fileSize > 0){
            char* dataBuf = static_cast<char *>(malloc(fileSize));

            if(dataBuf == nullptr) throw bad_alloc();

            readData(dataBuf, fileSize);
            if(dataBuf[0] == 'E' && dataBuf[1] == 'R' && dataBuf[2] == 'R'){
                cout << "Error requesting file " << requestPath << endl;
            }
            else{
                fileData.resize(fileSize);
                memcpy(fileData.data(), dataBuf, fileSize);
            }
            
            free(dataBuf);
        }
        
    }catch (std::exception &e){
        cout << "net error " << e.what() << endl;
    } 
}

#endif