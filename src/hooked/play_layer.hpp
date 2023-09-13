#pragma once

#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <chrono>

#include "../global_data.hpp"
#include "../util.hpp"

using namespace geode::prelude;

constexpr short TARGET_UPDATE_TPS = 30; // TODO might change to 60 if performance allows it
const float TARGET_UPDATE_DELAY = 1.0f / TARGET_UPDATE_TPS;

class $modify(ModifiedPlayLayer, PlayLayer) {
    bool m_markedDead = false;
    std::unordered_map<int, std::pair<CCSprite*, CCSprite*>> m_players;
    std::chrono::high_resolution_clock::time_point m_lastUpdateTime;

    bool init(GJGameLevel* level) {
        if (!PlayLayer::init(level)) {
            return false;
        }

        sendMessage(PlayerEnterLevelData { level->m_levelID });
        
        // data sending loop
        CCScheduler::get()->scheduleSelector(schedule_selector(ModifiedPlayLayer::sendPlayerData), this, TARGET_UPDATE_DELAY, false);

        return true;
    }

    void update(float dt) {
        // this updates the players' positions on the layer
        PlayLayer::update(dt);

        if (!m_isDead && m_fields->m_markedDead) {
            m_fields->m_markedDead = false;
        }

        if (m_isDead && !m_fields->m_markedDead) {
            sendMessage(PlayerDeadData{});
            m_fields->m_markedDead = true;
        }

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

    void removePlayer(int playerId) {
        log::debug("removing player {}", playerId);
        m_fields->m_players.at(playerId).first->removeFromParent();
        m_fields->m_players.at(playerId).second->removeFromParent();
        m_fields->m_players.erase(playerId);
    }

    void addPlayer(int playerId, const PlayerData& data) {
        log::debug("adding player {}", playerId);
        // auto playZone = static_cast<CCNode*>(this->getChildren()->objectAtIndex(3));
        auto playZone = m_objectLayer;
        auto sprite1 = CCSprite::create("globedMenuIcon.png"_spr);
        sprite1->setZOrder(99);
        sprite1->setID(fmt::format("dankmeme.globed/player-icon-{}", playerId));
        sprite1->setAnchorPoint({0.5f, 0.5f});

        playZone->addChild(sprite1);

        auto sprite2 = CCSprite::create("globedMenuIcon.png"_spr);
        sprite2->setZOrder(99);
        sprite2->setID(fmt::format("dankmeme.globed/player-icon-dual-{}", playerId));
        sprite2->setAnchorPoint({0.5f, 0.5f});

        playZone->addChild(sprite2);

        m_fields->m_players.insert(std::make_pair(playerId, std::make_pair(sprite1, sprite2)));
    }

    void updatePlayer(int playerId, const PlayerData& data) {
        // log::debug("updating player {}, x: {}, y: {}", playerId, data.x, data.y);
        auto player = m_fields->m_players.at(playerId);
        player.first->setPosition({data.player1.x, data.player1.y});
        player.first->setRotationX(data.player1.xRot);
        player.first->setRotationY(data.player1.yRot);
        player.first->setVisible(!data.player1.isHidden);
        log::debug("gamemode p1: {}", static_cast<int>(toUnderlying(data.player1.gameMode)));

        player.second->setPosition({data.player2.x, data.player2.y});
        player.second->setRotationX(data.player2.xRot);
        player.second->setRotationY(data.player2.yRot);
        player.second->setVisible(!data.player2.isHidden);
        log::debug("gamemode p2: {}", static_cast<int>(toUnderlying(data.player2.gameMode)));
    }

    void onQuit() {
        PlayLayer::onQuit();
        sendMessage(PlayerLeaveLevelData{});
    }

    void sendPlayerData(float dt) {
        auto data = PlayerData {
            .player1 = gatherSpecificPlayerData(m_player1, false),
            .player2 = gatherSpecificPlayerData(m_player2, true),
            .isPractice = m_isPracticeMode,
        };

        sendMessage(data);
    }

    SpecificIconData gatherSpecificPlayerData(PlayerObject* player, bool second) {
        IconGameMode gameMode;

        if (player->m_isShip) {
            gameMode = IconGameMode::SHIP;
        } else if (player->m_isBird) {
            gameMode = IconGameMode::UFO;
        } else if (player->m_isDart) {
            gameMode = IconGameMode::WAVE;
        } else if (player->m_isRobot) {
            gameMode = IconGameMode::ROBOT;
        } else if (player->m_isSpider) {
            gameMode = IconGameMode::SPIDER;
        } else if (player->m_isBall) {
            gameMode = IconGameMode::BALL;
        } else {
            gameMode = IconGameMode::CUBE;
        }

        return SpecificIconData {
            .x = player->m_position.x,
            .y = player->m_position.y,
            .xRot = player->getRotationX(),
            .yRot = player->getRotationY(),
            .gameMode = gameMode,
            .isHidden = player->m_isHidden || (second && player->m_position.x < 1),
            .isDashing = player->m_isDashing,
        };
    }

    void sendMessage(Message msg) {
        g_netMsgQueue.lock()->push(msg);
    }
};
