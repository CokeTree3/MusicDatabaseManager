#include "networking.h"

#if defined (PLATFORM_LINUX) || defined (PLATFORM_WINDOWS)
using asio::ip::tcp;

namespace {
    asio::io_context io_context;
    std::unique_ptr<tcp::socket> sock;
}

void sendData(tcp::socket *socket, const void *data, uint64_t dataSize, error_code &ec) {
    uint64_t request_length = htonl(dataSize);

    vector<asio::const_buffer> dataBuf;
    dataBuf.push_back(asio::buffer(&request_length, sizeof(request_length)));           // size_t writes diff data on win vs linux
    dataBuf.push_back(asio::buffer(data, dataSize));
    asio::write(*socket, dataBuf, ec);

    if(ec) throw ec;
}

void readDataSize(tcp::socket *socket, uint64_t* dataSize, error_code &ec){
    asio::read(*socket, asio::buffer(dataSize, sizeof(*dataSize)), ec);
    if(!ec) {
        uint64_t sizeConverted = ntohl(*dataSize);
        *dataSize = sizeConverted;
    }else {
        throw ec;
    }
}

void readData(tcp::socket *socket, void* dataBuf, uint64_t dataSize, error_code &ec){
    uint64_t readSize = 0;
    readSize = asio::read(*socket, asio::buffer(dataBuf, dataSize), ec);

    if (readSize < dataSize){
        cout << "not enough\n";
    }
    if(ec) throw ec;
}



class session : public std::enable_shared_from_this<session> {
public:
    session(tcp::socket socket) : socket_(std::move(socket)) {}

    void start(json* jsonData){
        this->jsonData = jsonData;
        do_read();
    }

private:
    void do_read(){

        uint64_t recvSize = 0;
        asio::error_code ec;
        while(true){
            readDataSize(&socket_, &recvSize, ec);
            if (recvSize > 16384){
                sendData(&socket_, (const void*)RESP_ERR, sizeof(RESP_ERR), ec);
                continue;
            }
            string recvBuf(recvSize, '\0');
            readData(&socket_, recvBuf.data(), recvSize, ec);

            if(!ec) {
                if(recvBuf[0] == 'S' && recvBuf[1] == 'R' && recvBuf[2] == 'Q') {                               // SYNC request
                    cout << "received a JSON request\n";
                    vector<uint8_t> jsonToSend = json::to_bson(*jsonData);
                    void* data = jsonToSend.data();

                    sendData(&socket_, data, jsonToSend.size(), ec);

                }else if(recvBuf[0] == 'G' && recvBuf[1] == 'E' && recvBuf[2] == 'T'){                          // GET request
                    
                    string req = recvBuf.substr(4);

                    cout << "Request for " << req << endl;

                    string requestPath = string((*jsonData)["LibraryPath"]) + "/" + req;

                    ifstream f;
                    f.open(requestPath, ios::binary);
                    if(f.is_open()){
                        vector<unsigned char> buff(std::istreambuf_iterator<char>(f), {});

                        sendData(&socket_, buff.data(), buff.size(), ec);
                    }else{
                        cout << "file error\n";
                        sendData(&socket_, static_cast<const void*>(RESP_ERR), sizeof(RESP_ERR), ec);
                    }

                }else{                                                                                          // Unknown request type
                    cout << "server received: \n";
                    cout << recvBuf << endl;
                    sendData(&socket_, static_cast<const void*>(RESP_ERR), sizeof(RESP_ERR), ec);
                }
            }else if(ec == asio::error::eof || ec == asio::error::connection_reset){
                cout << "Conn closed\n";
                break;
            }else{
                throw ec;
            }
        }
    }
    tcp::socket socket_;
    json* jsonData;
};

class server {
public:
    server(asio::io_context &io_context, short port, json* jsonData)
          : acceptor_(io_context, tcp::endpoint(tcp::v4(), port)), socket_(io_context) {
        do_accept(jsonData);
    }

private:
    void do_accept(json* jsonData) {
        acceptor_.async_accept(socket_, [this, &jsonData](std::error_code ec) {
            if(!ec){
                std::make_shared<session>(std::move(socket_))->start(jsonData);
            }

            do_accept(jsonData);
        });
    }

    tcp::acceptor acceptor_;
    tcp::socket socket_;
};

int initConn(json* jsonData, string addr) {
    if(serverMode){
        // init server worker
        cout << "server run\n";
        try {
            asio::io_context io_context;
            server s(io_context, (unsigned short)CONNECT_PORT, jsonData);
            io_context.run();

        } catch (std::exception &e) {
            cout << "net error " << e.what() << endl;
            return 1;
        }

    }else{
        // init connection
        cout << "client run\n";
        try {
            sock = std::make_unique<asio::ip::tcp::socket>(io_context);
            tcp::resolver resolver(io_context);

            asio::connect(*sock, resolver.resolve(addr, to_string(CONNECT_PORT)));
            if(jsonData == nullptr){
                throw;
            }
            error_code ec;
            if(sock->is_open()){
                cout << "init is open\n"; 
            }
            sendData(sock.get(), static_cast<const void*>(RQ_SYNC), sizeof(RQ_SYNC), ec);

            uint64_t jsonSize;
            readDataSize(sock.get(), &jsonSize, ec);

            char* jsonBuf = static_cast<char*>(malloc(jsonSize));
            readData(sock.get(), jsonBuf, jsonSize, ec);

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
            return 1;
        }
    }
    return 0;
}

void requestTrack(string requestPath, ofstream* fileOutput){
    if(!fileOutput->is_open() || !sock->is_open()){
        return;
    }
    cout << "requesting " << requestPath << endl;
    try{
        error_code ec;

        char* request = new char[3 + requestPath.length()];
        memcpy(request, RQ_GET, 3);
        memcpy(request + 3, requestPath.data(), requestPath.length());
        sendData(sock.get(), static_cast<const void*>(request), 3 + requestPath.length(), ec);

        uint64_t fileSize = 0;
        readDataSize(sock.get(), &fileSize, ec);
        if(fileSize > 0){
            char* fileBuf = static_cast<char *>(malloc(fileSize));
            readData(sock.get(), fileBuf, fileSize, ec);
            if(fileBuf[0] == 'E' && fileBuf[1] == 'R' && fileBuf[2] == 'R'){
                cout << "Error requesting file " << requestPath << endl;
            }
            else{
                fileOutput->write(static_cast<char*>(fileBuf), fileSize);
            }
            
            free(fileBuf);
        }
        
    }catch (error_code e){
        cout << "net error " << e.message() << endl;
    }
    
    // send req, read size, malloc the size, read to the mem ptr, write from mem to fileptr
    
}

#endif