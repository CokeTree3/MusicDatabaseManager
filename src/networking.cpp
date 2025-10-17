#include "networking.h"
#include "config.h"

using asio::ip::tcp;

void sendData(tcp::socket *socket, const void *data, size_t dataSize, error_code &ec) {
    size_t request_length = htonl(dataSize);

    vector<asio::const_buffer> dataBuf;
    dataBuf.push_back(asio::buffer(&request_length, sizeof(request_length)));
    dataBuf.push_back(asio::buffer(data, dataSize));

    asio::write(*socket, dataBuf, ec);

    if(ec) throw ec;
}

void readDataSize(tcp::socket *socket, size_t* dataSize, error_code &ec){
    asio::read(*socket, asio::buffer(dataSize, sizeof(dataSize)), ec);
    if(!ec) {
        *dataSize = ntohl(*dataSize);
    }else {
        throw ec;
    }
}

void readData(tcp::socket *socket, void* dataBuf, size_t dataSize, error_code &ec){
    asio::read(*socket, asio::buffer(dataBuf, dataSize), ec);
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

        size_t recvSize;
        asio::error_code ec;

        readDataSize(&socket_, &recvSize, ec);
        if (recvSize > 16384){
            sendData(&socket_, (const void*)RESP_ERR, sizeof(RESP_ERR), ec);
            return;
        }
        string recvBuf(recvSize, '\0');
        readData(&socket_, recvBuf.data(), recvSize, ec);

        if(!ec) {
            if(recvBuf[0] == 'S' && recvBuf[1] == 'R' && recvBuf[2] == 'Q') {                               // SYNC request
                cout << "received json request\n";
                vector<uint8_t> jsonToSend = json::to_bjdata(*jsonData);
                void* data = jsonToSend.data();

                sendData(&socket_, data, jsonToSend.size(), ec);

            }else if(recvBuf[0] == 'G' && recvBuf[1] == 'E' && recvBuf[2] == 'T'){                          // GET request
                sendData(&socket_, (const void*)RESP_OK, sizeof(RESP_OK), ec);
                cout << "Wrote OK\n";

                //read next data as json

            }else{
                cout << "server received: \n";
                cout << recvBuf << endl;
                sendData(&socket_, (const void*)RESP_ERR, sizeof(RESP_ERR), ec);
            }
        }else {
            throw ec;
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

void initConn(json* jsonData, string addr) {
    if(serverMode){
        // init server worker
        cout << "server run\n";
        try {
          asio::io_context io_context;
          server s(io_context, (unsigned short)CONNECT_PORT, jsonData);
          io_context.run();

        } catch (std::exception &e) {
          cout << "net error " << e.what() << endl;
          return;
        }

    }else{
        // init connection
        cout << "client run\n";
        try {
            asio::io_context io_context;

            tcp::socket s(io_context);

            tcp::resolver resolver(io_context);
            asio::connect(s, resolver.resolve(addr, to_string(CONNECT_PORT)));
            if(jsonData == nullptr){
                throw;
            }
            error_code ec;

            sendData(&s, (const void*)RQ_SYNC, sizeof(RQ_SYNC), ec);

            size_t jsonSize;
            readDataSize(&s, &jsonSize, ec);

            void* jsonBuf = malloc(jsonSize);
            readData(&s, jsonBuf, jsonSize, ec);

            vector<char> jsonRecv((char*)jsonBuf, (char*)jsonBuf + jsonSize);
            *jsonData = json::from_bjdata(jsonRecv);
            free(jsonBuf);

        }catch (std::exception &e){
            cout << "net error " << e.what() << endl;
        }
    }
}