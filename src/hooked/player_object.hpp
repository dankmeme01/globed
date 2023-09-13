#pragma once

#include <Geode/Geode.hpp>
#include <Geode/modify/PlayerObject.hpp>
#include <chrono>

#include "../global_data.hpp"

using namespace geode::prelude;

constexpr short PLAY_OBJECT_TARGET_TPS = 30; // TODO - change this for network packets per second
constexpr std::chrono::duration PLAY_OBJECT_TARGET_UPDATE_DT = std::chrono::duration<double>(1.f / PLAY_OBJECT_TARGET_TPS * 0.96); // for lagspikes and stuff multiply by 96%

class $modify(ModifiedPlayerObject, PlayerObject) {
    std::chrono::high_resolution_clock::time_point m_lastUpdateTime;

    void update(float dt) {
        PlayerObject::update(dt);

        if (!m_isInPlayLayer) {
            return;
        }

        auto currentTime = std::chrono::high_resolution_clock::now();
        auto difference = currentTime - m_fields->m_lastUpdateTime;

        if (difference > PLAY_OBJECT_TARGET_UPDATE_DT) {
            m_fields->m_lastUpdateTime = currentTime;
            sendMessage(gatherData());
        }
    }

    void sendMessage(Message msg) {
        g_netMsgQueue.lock()->push(msg);
    }
    

    PlayerData gatherData() {
        return PlayerData {
            g_playerIsPractice,
            m_position.x,
            m_position.y,
            getRotationX(),
            getRotationY(),
            m_isHidden,
            m_isDashing,
            m_playerColor1,
            m_playerColor2
        };
    }
};