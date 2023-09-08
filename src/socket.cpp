#include "socket.hpp"

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