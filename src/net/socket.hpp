#pragma once
#ifdef _WIN32

#include <ws2tcpip.h>
#define GLOBED_SOCKET_POLL WSAPoll
#define GLOBED_POLLFD WSAPOLLFD

#else

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cerrno>
#include <arpa/inet.h>
#include <poll.h>

#define GLOBED_SOCKET_POLL ::poll
#define GLOBED_POLLFD struct pollfd

#endif

class Socket {
public:
    virtual bool create() = 0;
    virtual bool connect(const std::string& serverIp, unsigned short port) = 0;
    virtual int send(const char* data, unsigned int dataSize) = 0;
    int send(const std::string& data);
    void sendAll(const char* data, unsigned int dataSize);
    void sendAll(const std::string& data);
    virtual int receive(char* buffer, int bufferSize) = 0;
    void receiveExact(char* buffer, int bufferSize);
    virtual bool close();
    virtual ~Socket();
    virtual bool poll(long msDelay) = 0;
};

bool loadNetLibraries();
void unloadNetLibraries();
int getLastNetError();