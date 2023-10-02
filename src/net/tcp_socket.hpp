/*
* This TcpSocket implementation was never used or tested at all.
* If you're just passing by and want to copy it, beware. It is here for completeness sake.
*/

#pragma once 
#include "socket.hpp"

class TcpSocket : public Socket {
public:
    using Socket::send;
    using Socket::sendAll;
    TcpSocket();

    bool create() override;
    bool connect(const std::string& serverIp, unsigned short port) override;
    int send(const char* data, unsigned int dataSize) override;
    int receive(char* buffer, int bufferSize) override;
    bool close() override;
    bool poll(long msDelay) override;
private:
    int socket_;
};