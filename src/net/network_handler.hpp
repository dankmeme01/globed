#pragma once
#include <chrono>
#include "game_socket.hpp"
constexpr int DEFAULT_TPS = 30;
constexpr std::chrono::duration<double> THREAD_SLEEP_DELAY = std::chrono::duration<double> (1.0f / DEFAULT_TPS);
constexpr std::chrono::seconds KEEPALIVE_DELAY = std::chrono::seconds(5);

class NetworkHandler {
public:
    NetworkHandler();
    ~NetworkHandler();
    void run();

    bool connectToServer(const std::string& id);
    void disconnect(bool quiet = false, bool save = true);

protected:
    std::thread mainThread;
    std::thread recvThread;
    std::thread keepaliveThread;

    GameSocket gameSocket;
    std::mutex modLoadedMtxMain, modLoadedMtxKeepalive;
    std::condition_variable cvarMain, cvarKeepalive;

    bool borked = false;

    void tKeepalive();
    void tRecv();
    void tMain();

    bool shouldContinueLooping();

    void testCentralServer(const std::string& modVersion, std::string url);
    void pingAllServers();
};
