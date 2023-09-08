#include "socket.hpp"

#ifdef _WIN32
#pragma comment(lib, "ws2_32.lib")
#endif

// Socket

int Socket::send(const std::string& data) {
    return send(data.c_str(), data.size());
}

bool Socket::sendall(const char* data, unsigned int dataSize) {
    return send(data, dataSize) == dataSize;
}

bool Socket::sendall(const std::string& data) {
    return send(data.c_str(), data.size()) == data.size();
}

bool Socket::close() {
    return false;
}

Socket::~Socket() {
    close();
}

bool loadNetLibraries() {
#ifdef _WIN32
    WSADATA wsaData;
    return WSAStartup(MAKEWORD(2, 2), &wsaData) == 0;
#else
    return true;
#endif
}

void unloadNetLibraries() {
#ifdef _WIN32
    WSACleanup();
#endif
}