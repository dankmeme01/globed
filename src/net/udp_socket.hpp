#pragma once
#include "socket.hpp"

class UdpSocket : public Socket {
public:
    using Socket::send;
    using Socket::sendall;
    UdpSocket();

    bool create() override;
    bool connect(const std::string& serverIp, unsigned short port) override;
    int send(const char* data, unsigned int dataSize) override;
    int receive(char* buffer, int bufferSize) override;
    bool close() override;

private:
    int socket_;
    sockaddr_in destAddr_;
};
