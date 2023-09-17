#pragma once

#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <chrono>

#include "../global_data.hpp"
#include "../ppa/ppa.hpp"
#include "../util.hpp"

#define ERROR_CORRECTION 1

using namespace geode::prelude;

const float OVERLAY_PAD_X = 5.f;
const float OVERLAY_PAD_Y = 1.f;

class $modify(ModifiedPlayLayer, PlayLayer) {
    bool m_markedDead = false;
    std::unordered_map<int, std::pair<CCSprite*, CCSprite*>> m_players;

    CCLabelBMFont *m_overlay = nullptr;
    long long m_previousPing = -2;
    float m_targetUpdateDelay = 0.f;
    std::unique_ptr<PPAEngine> m_ppaEngine;

    bool init(GJGameLevel* level) {
        if (!PlayLayer::init(level)) {
            return false;
        }

        m_fields->m_ppaEngine =
            pickPPAEngine(Mod::get()->getSettingValue<int64_t>("ppa-engine"));

        if (g_networkHandler->established()) {
            float delta = 1.f / g_gameServerTps.load();
            m_fields->m_targetUpdateDelay = delta;
            m_fields->m_ppaEngine->setTargetDelta(delta);
        }

        // XXX for testing
        level->m_levelID = 1;

        // 0 is for created levels, skip all sending but still show overlay
        if (level->m_levelID != 0) {
            sendMessage(PlayerEnterLevelData{level->m_levelID});

            // data sending loop
            CCScheduler::get()->scheduleSelector(
                schedule_selector(ModifiedPlayLayer::sendPlayerData), this,
                m_fields->m_targetUpdateDelay, false);
        }

        // setup ping overlay
        auto overlayPos = Mod::get()->getSettingValue<int64_t>("overlay-pos");
        auto overlayOffset = Mod::get()->getSettingValue<int64_t>("overlay-off");

        if (overlayPos != 0) {
            m_fields->m_overlay = CCLabelBMFont::create("-1 ms", "bigFont.fnt");

            switch (overlayPos) {
            case 1:
                m_fields->m_overlay->setAnchorPoint({0.f, 1.f});
                m_fields->m_overlay->setPosition({OVERLAY_PAD_X,this->getContentSize().height - OVERLAY_PAD_Y - overlayOffset});
                break;
            case 2:
                m_fields->m_overlay->setAnchorPoint({1.f, 1.f});
                m_fields->m_overlay->setPosition({this->getContentSize().width - OVERLAY_PAD_X, this->getContentSize().height - OVERLAY_PAD_Y - overlayOffset});
                break;
            case 3:
                m_fields->m_overlay->setAnchorPoint({0.f, 0.f});
                m_fields->m_overlay->setPosition({OVERLAY_PAD_X, OVERLAY_PAD_Y + overlayOffset});
                break;
            case 4:
                m_fields->m_overlay->setAnchorPoint({1.f, 0.f});
                m_fields->m_overlay->setPosition({this->getContentSize().width - OVERLAY_PAD_X, OVERLAY_PAD_Y + overlayOffset});
                break;
            }

            m_fields->m_overlay->setZOrder(99);
            m_fields->m_overlay->setOpacity(64);
            m_fields->m_overlay->setScale(0.4f);
            this->addChild(m_fields->m_overlay);
        }

        return true;
    }

    void update(float dt) {
        PlayLayer::update(dt);

        if (m_fields->m_overlay != nullptr) {
            // minor optimization, don't update if ping is the same as last tick
            long long currentPing = g_gameServerPing.load();
            if (currentPing != m_fields->m_previousPing) {
                m_fields->m_previousPing = currentPing;
                m_fields->m_overlay->setString(
                    fmt::format("{} ms", currentPing).c_str());
            }
        }

        // skip custom levels
        if (m_level->m_levelID == 0) {
            return;
        }

        if (!g_networkHandler->established())
            return;

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
        for (const auto &[key, _] : m_fields->m_players) {
            if (!players->contains(key)) {
                toRemove.push_back(key);
            }
        }

        for (const auto &key : toRemove) {
            removePlayer(key);
        }

        // add/update everyone else
        for (const auto &[key, data] : *players) {
            if (!m_fields->m_players.contains(key)) {
                addPlayer(key, data);
            } else {
                m_fields->m_ppaEngine->updatePlayer(m_fields->m_players.at(key), data, dt, key);
            }
        }
    }

    void removePlayer(int playerId) {
        log::debug("removing player {}", playerId);
        m_fields->m_players.at(playerId).first->removeFromParent();
        m_fields->m_players.at(playerId).second->removeFromParent();
        m_fields->m_players.erase(playerId);
        m_fields->m_ppaEngine->removePlayer(playerId);
    }

    void addPlayer(int playerId, const PlayerData &data) {
        log::debug("adding player {}", playerId);
        // auto playZone =
        // static_cast<CCNode*>(this->getChildren()->objectAtIndex(3));
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
        m_fields->m_ppaEngine->addPlayer(playerId, data);
    }

    void onQuit() {
        PlayLayer::onQuit();
        if (m_level->m_levelID != 0)
            sendMessage(PlayerLeaveLevelData{});
    }

    void sendPlayerData(float dt) {
        auto data = PlayerData{
            .player1 = gatherSpecificPlayerData(m_player1, false),
            .player2 = gatherSpecificPlayerData(m_player2, true),
            .isPractice = m_isPracticeMode,
        };

        sendMessage(data);
    }

    SpecificIconData gatherSpecificPlayerData(PlayerObject *player, bool second) {
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

        return SpecificIconData{
            .x = player->m_position.x,
            .y = player->m_position.y,
            .xRot = player->getRotationX(),
            .yRot = player->getRotationY(),
            .gameMode = gameMode,
            .isHidden = player->m_isHidden || (second && player->m_position.x < 1),
            .isDashing = player->m_isDashing,
            .isUpsideDown = player->m_isUpsideDown,
        };
    }

    void sendMessage(Message msg) {
        g_netMsgQueue.push(msg);
    }
};
