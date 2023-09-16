#pragma once
#include <chrono>
#include "game_socket.hpp"

constexpr const char* PROTOCOL_VERSION = "1";
constexpr int DEFAULT_TPS = 30;
constexpr std::chrono::duration<double> THREAD_SLEEP_DELAY = std::chrono::duration<double> (1.0f / DEFAULT_TPS);
constexpr std::chrono::seconds KEEPALIVE_DELAY = std::chrono::seconds(5);

class NetworkHandler {
public:
    NetworkHandler(int secretKey);
    ~NetworkHandler();
    void run();

    bool connectToServer(const std::string& id);
    void disconnect(bool quiet = false, bool save = true);

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
