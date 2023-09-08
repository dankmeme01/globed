#pragma once

#include <Geode/Geode.hpp>
#include <Geode/modify/PlayerObject.hpp>
#include <chrono>

#include "inter_thread_data.hpp"

using namespace geode::prelude;

constexpr short TARGET_TPS = 30; // TODO - change this for network packets per second
constexpr float TARGET_UPDATE_DT = static_cast<float>(1000) / TARGET_TPS * 0.96; // for lagspikes and stuff multiply by 96%

class $modify(ModifiedPlayerObject, PlayerObject) {
    std::chrono::milliseconds m_lastUpdateTime;

    void update(float dt) {
        PlayerObject::update(dt);

        if (!m_isInPlayLayer) {
            return;
        }

        auto rawTime = std::chrono::high_resolution_clock::now();
        auto currentTime = std::chrono::duration_cast<std::chrono::milliseconds>(rawTime.time_since_epoch());

        auto difference = currentTime - m_fields->m_lastUpdateTime;

        if (difference.count() > TARGET_UPDATE_DT) {
            m_fields->m_lastUpdateTime = currentTime;
            log::debug("tick at {}, diff was {}", currentTime, difference);
            sendMessage(gatherData());
        }
    }

    void sendMessage(Message msg) {
        std::lock_guard<std::mutex> lock(g_netMutex);
        g_netMsgQueue.push(msg);
        g_netCVar.notify_one();
    }

    PlayerData gatherData() {
        return PlayerData {
            g_playerIsPractice,
            m_position.x,
            m_position.y,
            m_isHidden,
            m_isUpsideDown,
            m_isDashing,
            m_playerColor1,
            m_playerColor2
        };
    }
};