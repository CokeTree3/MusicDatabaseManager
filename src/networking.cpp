#include "networking.h"

#if defined (PLATFORM_LINUX) || defined (PLATFORM_WINDOWS)
using asio::ip::tcp;

void sendData(tcp::socket *socket, const void *data, uint64_t dataSize, error_code& ec) {
    uint64_t request_length = htonl(dataSize);

    vector<asio::const_buffer> dataBuf;
    dataBuf.push_back(asio::buffer(&request_length, sizeof(request_length)));           // size_t writes diff data on win vs linux
    dataBuf.push_back(asio::buffer(data, dataSize));
    asio::write(*socket, dataBuf, ec);
}

void readDataSize(tcp::socket *socket, uint64_t* dataSize, error_code& ec){
    asio::read(*socket, asio::buffer(dataSize, sizeof(*dataSize)), ec);
    if(!ec) {
        uint64_t sizeConverted = ntohl(*dataSize);
        *dataSize = sizeConverted;
    }
}

void readData(tcp::socket *socket, void* dataBuf, uint64_t dataSize, error_code& ec){
    uint64_t readSize = 0;
    readSize = asio::read(*socket, asio::buffer(dataBuf, dataSize), ec);

    if (readSize < dataSize){
        cout << "not enough\n";
    }
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
        while(true){
            uint64_t recvSize = 0;
            error_code ec;
            readDataSize(&socket_, &recvSize, ec);
            if(ec == asio::error::eof || ec == asio::error::connection_reset){
                cout << "Conn closed" << endl;
                break;
            }else if(ec){
                cout << "Server Error!!\n" << ec.message() << endl;
                break;
            }
            string recvBuf(recvSize, '\0');
            readData(&socket_, recvBuf.data(), recvSize, ec);
            if(!ec) {
                if(recvBuf[0] == 'S' && recvBuf[1] == 'R' && recvBuf[2] == 'Q') {                               // SYNC request
                    cout << "received a JSON request" << endl;
                    json jsonToSend = *jsonData;
                    if(jsonToSend.contains("LibraryPath")){
                        jsonToSend.erase("LibraryPath");
                    }
                    vector<uint8_t> jsonBufToSend = json::to_bson(jsonToSend);

                    sendData(&socket_, jsonBufToSend.data(), jsonBufToSend.size(), ec);

                }else if(recvBuf[0] == 'G' && recvBuf[1] == 'E' && recvBuf[2] == 'T'){                          // GET request
                    
                    string req = recvBuf.substr(4);

                    cout << "Request for " << req << endl;

                    filesystem::path fsRelPath(req);

                    if(distance(fsRelPath.begin(), fsRelPath.end()) != 3){
                        cout << "invalid request\n";
                        sendData(&socket_, static_cast<const void*>(RESP_ERR), sizeof(RESP_ERR), ec);
                        continue;
                    }
                    for(auto const& part : fsRelPath){
                        if(part == "." || part == ".."){
                            cout << "invalid request\n";
                            sendData(&socket_, static_cast<const void*>(RESP_ERR), sizeof(RESP_ERR), ec);
                            continue;
                        }
                    }

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
                    continue;
                }
            }else if(ec == asio::error::eof || ec == asio::error::connection_reset){
                cout << "Conn closed\n";
                break;
            }else{
                cout << "Server Error!!\n" << ec.message() << endl;
                break;
            }
        }
        socket_.close();
    }
    tcp::socket socket_;
    json* jsonData;
};

class server {
public:
    server(asio::io_context &io_context, short port, json* jsonData)
          : acceptor_(io_context, tcp::endpoint(tcp::v4(), port)), socket_(io_context) {
        this->jsonData = jsonData;
        do_accept();
    }

    void server_stop(){
        error_code ec;
        acceptor_.close(ec);
        if(ec){
            cout << "net error closing connection: " << ec.message() << endl;
        }
    }

private:
    void do_accept() {
        acceptor_.async_accept(socket_, [this](std::error_code ec) {
            if(!ec){
                std::make_shared<session>(std::move(socket_))->start(jsonData);
            }
            if(acceptor_.is_open()){
                do_accept();                
            }
        });
    }

    tcp::acceptor acceptor_;
    tcp::socket socket_;
    json* jsonData;
};


namespace {
    asio::io_context io_context;
    std::unique_ptr<tcp::socket> sock;
    server* server_;
}

int initConn(json* jsonData, string addr) {
    if(serverMode){
        // init server worker
        cout << "server run\n";
        try {
            asio::io_context io_context;
            server s(io_context, (unsigned short)CONNECT_PORT, jsonData);
            server_ = &s;
            io_context.run();

        } catch (std::exception &e) {
            cout << "net error " << e.what() << endl;
            return 1;
        }catch (...){
            cout << "Network error: unknown! " << endl;
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

        }catch (exception &e){
            cout << "net error " << e.what() << endl;
            return 1;
        }catch (...){
            cout << "Network error: unknown! " << endl;
            return 1;
        }
    }
    return 0;
}

void requestTrack(string requestPath, vector<char>& fileData){
    if(!sock->is_open()){
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
                fileData.resize(fileSize);
                memcpy(fileData.data(), fileBuf, fileSize);
            }
            
            free(fileBuf);
        }
        
    }catch (error_code e){
        cout << "net error " << e.message() << endl;
    }
    
}

void stopServer(){
    if(server_ != nullptr){
        server_->server_stop();
        io_context.stop();
    }
}

#endif