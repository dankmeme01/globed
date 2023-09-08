#pragma once
#ifdef _WIN32
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#endif

class Socket {
public:
    virtual bool create() = 0;
    virtual bool connect(const std::string& serverIp, unsigned short port) = 0;
    virtual int send(const char* data, unsigned int dataSize) = 0;
    int send(const std::string& data);
    bool sendall(const char* data, unsigned int dataSize);
    bool sendall(const std::string& data);
    virtual int receive(char* buffer, int bufferSize) = 0;
    virtual bool close();
    virtual ~Socket();
};

bool loadNetLibraries();
void unloadNetLibraries();