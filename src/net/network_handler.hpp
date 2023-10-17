#pragma once
#include <chrono>
#include <thread>
#include "game_socket.hpp"

constexpr const char* PROTOCOL_VERSION = "7";
constexpr std::chrono::seconds KEEPALIVE_DELAY = std::chrono::seconds(5);

// Mac has no std::jthread yet
#ifdef GEODE_IS_MACOS
#define GLOBED_THREAD std::thread
#else
#define GLOBED_THREAD std::jthread
#define GLOBED_JTHREADS
#endif

class NetworkHandler {
public:
    NetworkHandler(int secretKey);
    ~NetworkHandler();
    void run();

    bool connectToServer(const std::string& id);
    void disconnect(bool quiet = false, bool save = true);

    int getAccountId();
    int getSecretKey();

    bool established();

protected:
    GLOBED_THREAD mainThread;
    GLOBED_THREAD recvThread;
    GLOBED_THREAD keepaliveThread;

    GameSocket gameSocket;
    std::mutex mtxMain;
    std::condition_variable cvarMain;

    bool borked = false;

    void tKeepalive();
    void tRecv();
    void tMain();

    bool shouldContinueLooping();

    void testCentralServer(const std::string& modVersion, std::string url);
    void pingAllServers();
};
