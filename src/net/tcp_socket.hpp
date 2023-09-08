#pragma once 
#include "socket.hpp"

class TcpSocket : public Socket {
public:
    using Socket::send;
    using Socket::sendall;
    TcpSocket();

    bool create() override;
    bool connect(const std::string& serverIp, unsigned short port) override;
    int send(const char* data, unsigned int dataSize) override;
    int receive(char* buffer, int bufferSize) override;
    bool close() override;
private:
    int socket_;
};