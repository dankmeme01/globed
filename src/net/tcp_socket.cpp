#include "tcp_socket.hpp"

TcpSocket::TcpSocket() : socket_(0) {}

bool TcpSocket::create() {
    socket_ = socket(AF_INET, SOCK_STREAM, 0);
    return socket_ != -1;
}

bool TcpSocket::connect(const std::string& serverIp, unsigned short port) {
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    if (inet_pton(AF_INET, serverIp.c_str(), &(serverAddr.sin_addr)) <= 0) {
        std::cerr << "Invalid address or address not supported" << std::endl;
        return false;
    }
    return ::connect(socket_, reinterpret_cast<struct sockaddr*>(&serverAddr), sizeof(serverAddr)) == 0;
}

int TcpSocket::send(const char* data, unsigned int dataSize) {
    return ::send(socket_, data, dataSize, 0);
}

int TcpSocket::receive(char* buffer, int bufferSize) {
    return ::recv(socket_, buffer, bufferSize, 0);
}

bool TcpSocket::close() {
#ifdef _WIN32
    return ::closesocket(socket_) == 0;
#else
    return ::close(socket_) == 0;
#endif
}