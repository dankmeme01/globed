#pragma once
#include <chrono>
#include <thread>
#include "game_socket.hpp"

constexpr const char* PROTOCOL_VERSION = "7";
constexpr std::chrono::seconds KEEPALIVE_DELAY = std::chrono::seconds(5);

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
    std::thread mainThread;
    std::thread recvThread;
    std::thread keepaliveThread;

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
