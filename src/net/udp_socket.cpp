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
        geode::log::error("Invalid game server address or address not supported");
        return false;
    }

    connected = true;
    return true; // No actual connection is established in UDP
}

int UdpSocket::send(const char* data, unsigned int dataSize) {
    return sendto(socket_, data, dataSize, 0, reinterpret_cast<struct sockaddr*>(&destAddr_), sizeof(destAddr_));
}

void UdpSocket::sendAllTo(const char* data, unsigned int dataSize, const std::string& serverIp, unsigned short port) {
    if (connected) {
        auto prevAddress = destAddr_;
        connect(serverIp, port);
        sendAll(data, dataSize);
        destAddr_ = prevAddress;
    } else {
        connect(serverIp, port);
        sendAll(data, dataSize);
        disconnect();
    }
}

void UdpSocket::disconnect() {
    connected = false;
}

int UdpSocket::receive(char* buffer, int bufferSize) {
    socklen_t addrLen = sizeof(destAddr_);
    return recvfrom(socket_, buffer, bufferSize, 0, reinterpret_cast<struct sockaddr*>(&destAddr_), &addrLen);
}

bool UdpSocket::close() {
    connected = false;
#ifdef _WIN32
    return ::closesocket(socket_) == 0;
#else
    return ::close(socket_) == 0;
#endif
}

bool UdpSocket::poll(long msDelay) {
    GLOBED_POLLFD fds[1];

    fds[0].fd = socket_;
    fds[0].events = POLLIN;

    int result = GLOBED_SOCKET_POLL(fds, 1, (int)msDelay);

    if (result == -1) {
#ifndef GEODE_IS_ANDROID
        throw std::runtime_error(fmt::format("select failed, error: {}", getLastNetErrorPretty()));
#else
        // android is stupid!!!!!
        geode::log::warn("poll failed! error: {}", getLastNetErrorPretty());
        return false;
#endif
    } 
    
    return result > 0;
}