#pragma once

#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <chrono>

#include "../global_data.hpp"
#include "../util.hpp"

using namespace geode::prelude;

constexpr short PLAY_LAYER_TARGET_TPS = 30; // TODO might change to 60 if performance allows it
constexpr std::chrono::duration PLAY_LAYER_TARGET_UPDATE_DT = std::chrono::duration<double>(1.f / PLAY_LAYER_TARGET_TPS * 0.96); // for lagspikes and stuff multiply by 96%

class $modify(ModifiedPlayLayer, PlayLayer) {
    bool m_markedDead = false;
    std::unordered_map<int, CCSprite*> m_players;
    std::chrono::high_resolution_clock::time_point m_lastUpdateTime;

    bool init(GJGameLevel* level) {
        if (!PlayLayer::init(level)) {
            return false;
        }

        sendMessage(PlayerEnterLevelData { level->m_levelID });

        return true;
    }

    void sendMessage(Message msg) {
        g_netMsgQueue.lock()->push(msg);
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

        // maybe update
        
        auto currentTime = std::chrono::high_resolution_clock::now();
        auto difference = currentTime - m_fields->m_lastUpdateTime;

        if (difference > PLAY_LAYER_TARGET_UPDATE_DT) {
            m_fields->m_lastUpdateTime = currentTime;

            auto players = g_netRPlayers.lock();

            // remove players that left the level
            std::vector<int> toRemove;
            for (const auto& [key, _] : m_fields->m_players)  {
                if (!players->contains(key)) {
                    toRemove.push_back(key);
                }
            }

            for (const auto& key : toRemove) {
                removePlayer(key);
            }

            // add/update everyone else
            for (const auto& [key, data] : *players) {
                if (!m_fields->m_players.contains(key)) {
                    addPlayer(key, data);
                } else {
                    updatePlayer(key, data);
                }
            }
        }
    }

    void removePlayer(int playerId) {
        log::debug("removing player {}", playerId);
        m_fields->m_players.at(playerId)->removeFromParent();
        m_fields->m_players.erase(playerId);
    }

    void addPlayer(int playerId, const PlayerData& data) {
        log::debug("adding player {}", playerId);
        auto playZone = static_cast<CCNode*>(this->getChildren()->objectAtIndex(3));
        auto sprite = CCSprite::create("globedMenuIcon.png"_spr);
        sprite->setZOrder(99);
        sprite->setID(fmt::format("dankmeme.globed/player-icon-{}", playerId));

        playZone->addChild(sprite);

        m_fields->m_players.insert(std::make_pair(playerId, sprite));
    }

    void updatePlayer(int playerId, const PlayerData& data) {
        // log::debug("updating player {}, x: {}, y: {}", playerId, data.x, data.y);
        auto player = m_fields->m_players.at(playerId);
        player->setPosition({data.x, data.y});
        player->setAnchorPoint({.5f, .5f});
        player->setVisible(!data.isHidden);
        player->setRotationX(data.xRot);
        player->setRotationY(data.yRot);
    }

    void onQuit() {
        PlayLayer::onQuit();
        sendMessage(PlayerLeaveLevelData{});
    }
};
