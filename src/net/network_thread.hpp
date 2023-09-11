#pragma once
constexpr int DEFAULT_TPS = 30;
constexpr float THREAD_SLEEP_DELAY_MS = (static_cast<float>(1000) / DEFAULT_TPS);

void networkThread();