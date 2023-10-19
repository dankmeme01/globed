#include "socket.hpp"

#ifdef _WIN32
#pragma comment(lib, "ws2_32.lib")
#endif

// Socket

int Socket::send(const std::string& data) {
    return send(data.c_str(), data.size());
}

void Socket::sendAll(const char* data, unsigned int dataSize) {
    const char* sendbuf = data;
    do {
        auto sent = send(sendbuf, dataSize - (sendbuf - data));
        if (sent == -1) {
            auto msg = fmt::format("failed to send(): {}", getLastNetErrorPretty());
            geode::log::warn(msg);
            // i hate it here
#ifndef GEODE_IS_ANDROID
            throw std::runtime_error(msg);
#else
            return;
#endif
        }
        sendbuf += sent;
    } while (data + dataSize > sendbuf);
}

void Socket::sendAll(const std::string& data) {
    sendAll(data.c_str(), data.size());
}

void Socket::receiveExact(char* buffer, int bufferSize) {
    char* bufptr = buffer;

    do {
        auto received = receive(bufptr, bufferSize - (bufptr - buffer));
        if (received == -1) {
            auto msg = fmt::format("failed to receive(): {}", getLastNetErrorPretty());
            geode::log::warn(msg);
            throw std::runtime_error(msg);
        }
        bufptr += received;
    } while (buffer + bufferSize > bufptr);
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

int getLastNetError() {
#ifdef _WIN32
    return WSAGetLastError();
#else
    return errno;
#endif
}

std::string getLastNetErrorPretty() {
    auto err = getLastNetError();
#ifdef _WIN32
    char *s = NULL;
    FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&s, 0, NULL);
    std::string formatted = fmt::format("(Win32) error code {}: {}", err, s);
    LocalFree(s);
#else
    std::string formatted = fmt::format("(Posix) error code {}: {}", err, strerror(err));
#endif

    return formatted;
}