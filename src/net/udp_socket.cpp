#include "udp_socket.hpp"

UdpSocket::UdpSocket() : socket_(0) {
    memset(&destAddr_, 0, sizeof(destAddr_));
}

bool UdpSocket::create() {
    socket_ = socket(AF_INET, SOCK_DGRAM, 0);
    return socket_ != -1;
}

bool UdpSocket::connect(const std::string& serverIp, unsigned short port) {
    destAddr_.sin_family = AF_INET;
    destAddr_.sin_port = htons(port);
    if (inet_pton(AF_INET, serverIp.c_str(), &(destAddr_.sin_addr)) <= 0) {
        std::cerr << "Invalid address or address not supported" << std::endl;
        return false;
    }
    return true; // No actual connection is established in UDP
}

int UdpSocket::send(const char* data, unsigned int dataSize) {
    return sendto(socket_, data, dataSize, 0, reinterpret_cast<struct sockaddr*>(&destAddr_), sizeof(destAddr_));
}

int UdpSocket::receive(char* buffer, int bufferSize) {
    socklen_t addrLen = sizeof(destAddr_);
    return recvfrom(socket_, buffer, bufferSize, 0, reinterpret_cast<struct sockaddr*>(&destAddr_), &addrLen);
}

bool UdpSocket::close() {
#ifdef _WIN32
    return ::closesocket(socket_) == 0;
#else
    return ::close(socket_) == 0;
#endif
}
