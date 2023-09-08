#pragma once

#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <chrono>

#include "inter_thread_data.hpp"
#include "util.hpp"

using namespace geode::prelude;

class $modify(ModifiedPlayLayer, PlayLayer) {
    bool m_markedDead = false;

    bool init(GJGameLevel* level) {
        if (!PlayLayer::init(level)) {
            return false;
        }

        sendMessage(PlayerEnterLevelData { level->m_levelID });

        return true;
    }

    void sendMessage(Message msg) {
        std::lock_guard<std::mutex> lock(g_netMutex);
        g_netMsgQueue.push(msg);
        g_netCVar.notify_one();
    }

    void update(float dt) {
        PlayLayer::update(dt);

        if (!m_isDead && m_fields->m_markedDead) {
            m_fields->m_markedDead = false;
        }

        if (m_isDead && !m_fields->m_markedDead) {
            sendMessage(PlayerDeadData{});
            m_fields->m_markedDead = true;
        }

        g_playerIsPractice = m_isPracticeMode;
    }

    void onQuit() {
        PlayLayer::onQuit();
        sendMessage(PlayerLeaveLevelData{});
    }
};
