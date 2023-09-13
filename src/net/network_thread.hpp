#pragma once
#include <chrono>
constexpr int DEFAULT_TPS = 30;
constexpr std::chrono::duration<double> THREAD_SLEEP_DELAY = std::chrono::duration<double> (1.0f / DEFAULT_TPS);
constexpr std::chrono::seconds KEEPALIVE_DELAY = std::chrono::seconds(5);

void networkThread();

void recvThread();
void keepaliveThread();

bool shouldContinueLooping();