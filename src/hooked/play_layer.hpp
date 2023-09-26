#pragma once

#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <chrono>

#include "../global_data.hpp"
#include "../ui/remote_player.hpp"
#include "../ui/player_progress.hpp"
#include "../util.hpp"
#include "../correction/corrector.hpp"

#define ERROR_CORRECTION 1

using namespace geode::prelude;

const float OVERLAY_PAD_X = 5.f;
const float OVERLAY_PAD_Y = 0.f;

class $modify(ModifiedPlayLayer, PlayLayer) {
    bool m_markedDead = false;
    std::unordered_map<int, std::pair<RemotePlayer*, RemotePlayer*>> m_players;
    std::unordered_map<int, PlayerProgress*> m_playerProgresses;

    CCLabelBMFont *m_overlay = nullptr;
    long long m_previousPing = -2;
    float m_targetUpdateDelay = 0.f;

    float m_ptTimestamp = 0.0;
    bool m_wasSpectating;
    float m_sentCamera = 1.f;
    float m_syncingCamera = 0.0f;

    // settings
    bool m_displayPlayerProgress, m_showProgressMoving;
    unsigned char m_playerOpacity;

    RemotePlayerSettings m_rpSettings;

    bool init(GJGameLevel* level) {
        if (!PlayLayer::init(level)) {
            return false;
        }

        if (g_networkHandler->established()) {
            float delta = 1.f / g_gameServerTps.load();
            m_fields->m_targetUpdateDelay = delta;
            g_pCorrector.setTargetDelta(delta);
        }

        m_fields->m_rpSettings = RemotePlayerSettings {
            .defaultMiniIcons = Mod::get()->getSettingValue<bool>("default-mini-icon"),
            .practiceIcon = Mod::get()->getSettingValue<bool>("practice-icon"),
            .secondNameEnabled = Mod::get()->getSettingValue<bool>("show-names-dual"),
            .nameColors = Mod::get()->getSettingValue<bool>("name-colors"),
            .nameOpacity = static_cast<unsigned char>(Mod::get()->getSettingValue<int64_t>("show-names-opacity"))
        };

        m_fields->m_displayPlayerProgress = Mod::get()->getSettingValue<bool>("show-progress");
        m_fields->m_showProgressMoving = Mod::get()->getSettingValue<bool>("show-progress-moving");
        m_fields->m_playerOpacity = static_cast<unsigned char>(Mod::get()->getSettingValue<int64_t>("player-opacity"));

        if (g_debug && level->m_levelID == 0) {
            level->m_levelID = 1;
        }

        g_pCorrector.joinedLevel();

        // 0 is for created levels, skip all sending but still show overlay
        if (level->m_levelID != 0) {
            sendMessage(NMPlayerLevelEntry{level->m_levelID});

            // data sending loop
            CCScheduler::get()->scheduleSelector(
                schedule_selector(ModifiedPlayLayer::sendPlayerData), this,
                m_fields->m_targetUpdateDelay, false);

            CCScheduler::get()->scheduleSelector(
                schedule_selector(ModifiedPlayLayer::updateStuff), this, 1.0f, false);
        }

        // setup ping overlay
        auto overlayPos = Mod::get()->getSettingValue<int64_t>("overlay-pos");
        auto overlayOffset = Mod::get()->getSettingValue<int64_t>("overlay-off");

        if (overlayPos != 0) {
            m_fields->m_overlay = CCLabelBMFont::create(level->m_levelID == 0 ? "N/A (custom level)" : "Not connected", "bigFont.fnt");

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
        m_fields->m_ptTimestamp += dt;

        // skip custom levels
        if (m_level->m_levelID == 0) {
            return;
        }

        // skip disconnected
        if (!g_networkHandler->established())
            return;

        if (!m_isDead && m_fields->m_markedDead) {
            m_fields->m_markedDead = false;
        }

        if (m_isDead && !m_fields->m_markedDead) {
            sendMessage(NMPlayerDied {});
            m_fields->m_markedDead = true;
        }
        
        // update everyone
        for (const auto &[key, players] : m_fields->m_players) {
            g_pCorrector.interpolate(players, dt, key);
            auto playerPos = players.first->getPosition();

            // display progress
            if (m_fields->m_displayPlayerProgress) {
                auto progress = m_fields->m_playerProgresses.at(key);
                float progressVal = playerPos.x / m_levelLength;

                if (!isPlayerVisible(players.first->getPosition()) || g_debug) {
                    bool onRightSide = playerPos.x > m_player1->getPositionX();
                    progress->updateValues(progressVal * 100, onRightSide);
                    progress->setVisible(true);
                    progress->setAnchorPoint({onRightSide ? 1.f : 0.f, 0.5f});
                    float prHeight;

                    auto winSize = CCDirector::get()->getWinSize();
                    if (m_fields->m_showProgressMoving) {
                        auto maxHeight = winSize.height * 0.85f;
                        auto minHeight = winSize.height - maxHeight;
                        prHeight = minHeight + (maxHeight - minHeight) * progressVal;
                    } else {
                        prHeight = 60.f;
                    }
                    progress->setPosition({onRightSide ? (winSize.width - 5.f) : 5.f, prHeight});
                } else {
                    progress->setVisible(false);
                }
            }
        }

        if (m_isDualMode) {
            moveCameraToV2Dual({m_player1->getPositionX(), m_bottomGround->getPositionY()});
        } else {
            moveCameraToV2(m_player1->getPosition());
        }

        if (g_spectatedPlayer != 0) {
            if (!m_fields->m_players.contains(g_spectatedPlayer)) {
                leaveSpectate();
                return;
            }

            m_fields->m_wasSpectating = true;

            auto& data = m_fields->m_players.at(g_spectatedPlayer);
            m_player1->setPosition(data.first->getPosition());
            m_player2->setPosition(data.second->getPosition());
            m_player1->setVisible(false);
            m_player2->setVisible(false);

            if (data.second->isVisible()) {
                moveCameraToV2Dual({data.first->getPositionX(), m_bottomGround->getPositionY()}, 0.0f);
            } else {
                moveCameraToV2(data.first->getPosition(), 0.0f);
            }

            m_player1->m_playerGroundParticles->setVisible(false);
            m_player2->m_playerGroundParticles->setVisible(false);
            updateProgressbar();

            // log::debug("player pos: {}, {}; camera pos: {}, {}", m_player1->getPositionX(), m_player1->getPositionY(), m_cameraPosition.x, m_cameraPosition.y);
        } else if (g_spectatedPlayer == 0 && m_fields->m_wasSpectating) {
            leaveSpectate();
        } else if (false) {
            // ^ put some terribly complicated calculation that defines whether the spectated player died on the last attempt
            resetLevel();
        }

        if (g_debug) {
            m_player1->setOpacity(64);
            m_player2->setOpacity(64);
        }
    }

    // updateStuff is update() but less time-sensitive, runs every second rather than every frame.
    void updateStuff(float dt) {
        if (m_fields->m_overlay != nullptr && m_level->m_levelID != 0) {
            // minor optimization, don't update if ping is the same as last tick
            long long currentPing = g_gameServerPing.load();
            if (currentPing != m_fields->m_previousPing) {
                m_fields->m_previousPing = currentPing;
                if (currentPing == -1) {
                    m_fields->m_overlay->setString("Not connected");
                } else {
                    m_fields->m_overlay->setString(fmt::format("{} ms", currentPing).c_str());
                }
            }
        }

        // skip custom levels
        if (m_level->m_levelID == 0) {
            return;
        }

        // skip disconnected
        if (!g_networkHandler->established())
            return;

        // remove players that left the level
        for (const auto &playerId : g_pCorrector.getGonePlayers()) {
            if (m_fields->m_players.contains(playerId)) {
                removePlayer(playerId);
            }
        }

        // add players that aren't on the level
        for (const auto &playerId : g_pCorrector.getNewPlayers()) {
            if (!m_fields->m_players.contains(playerId)) {
                addPlayer(playerId);
            }
        }

        // update players with default data
        for (const auto &[key, player] : m_fields->m_players) {
            if (player.first->isDefault || player.second->isDefault) {
                auto dataCache = g_accDataCache.lock();
                // are they in cache? then just call updateData on RemotePlayers
                if (dataCache->contains(key)) {
                    player.first->updateData(dataCache->at(key));
                    player.second->updateData(dataCache->at(key));
                    // make sure to update opacities!
                    player.first->setOpacity(m_fields->m_playerOpacity);
                    player.second->setOpacity(m_fields->m_playerOpacity);
                } else {
                    // if not, request them from the server.
                    sendMessage(NMRequestPlayerAccount { .playerId = key });
                }
            }
        }
    }

    bool isPlayerVisible(CCPoint nodePosition) {
        auto camera = m_pCamera;
        auto cameraPosition = m_cameraPosition;
        auto cameraViewSize = CCDirector::get()->getWinSize();

        bool isVisibleInCamera = (nodePosition.x + m_player1->getContentSize().width >= cameraPosition.x &&
                          nodePosition.x <= cameraPosition.x + cameraViewSize.width &&
                          nodePosition.y + m_player1->getContentSize().height >= cameraPosition.y &&
                          nodePosition.y <= cameraPosition.y + cameraViewSize.height);

        return isVisibleInCamera;
    }

    void removePlayer(int playerId) {
        log::debug("removing player {}", playerId);
        m_fields->m_players.at(playerId).first->removeFromParent();
        m_fields->m_players.at(playerId).second->removeFromParent();
        m_fields->m_players.erase(playerId);
        m_fields->m_playerProgresses.at(playerId)->removeFromParent();
        m_fields->m_playerProgresses.erase(playerId);
        log::debug("removed player {}", playerId);
    }

    void addPlayer(int playerId) {
        log::debug("adding player {}", playerId);
        auto playZone = m_objectLayer;

        RemotePlayer *player1, *player2;

        // note - playerprogress must be initialized here, before locking g_accDataCache
        auto progress = PlayerProgress::create(playerId);
        progress->setZOrder(4);
        progress->setID(fmt::format("dankmeme.globed/player-progress-{}", playerId));
        if (!m_fields->m_displayPlayerProgress) {
            progress->setVisible(false);
        }
        this->addChild(progress);

        auto iconCache = g_accDataCache.lock();
        if (iconCache->contains(playerId)) {
            auto icons = iconCache->at(playerId);
            player1 = RemotePlayer::create(false, m_fields->m_rpSettings, icons);
            player2 = RemotePlayer::create(true, m_fields->m_rpSettings, icons);
        } else {
            player1 = RemotePlayer::create(false, m_fields->m_rpSettings);
            player2 = RemotePlayer::create(true, m_fields->m_rpSettings);
        }

        player1->setZOrder(0);
        player1->setID(fmt::format("dankmeme.globed/player-icon-{}", playerId));
        player1->setAnchorPoint({0.5f, 0.5f});
        player1->setOpacity(m_fields->m_playerOpacity);

        playZone->addChild(player1);

        player2->setZOrder(0);
        player2->setID(fmt::format("dankmeme.globed/player-icon-dual-{}", playerId));
        player2->setAnchorPoint({0.5f, 0.5f});
        player2->setOpacity(m_fields->m_playerOpacity);

        playZone->addChild(player2);

        m_fields->m_players.insert(std::make_pair(playerId, std::make_pair(player1, player2)));
        m_fields->m_playerProgresses.insert(std::make_pair(playerId, progress));
        log::debug("added player {}", playerId);
    }

    void onQuit() {
        PlayLayer::onQuit();
        if (m_level->m_levelID != 0)
            sendMessage(NMPlayerLevelExit{});

        g_spectatedPlayer = 0;
    }

    void sendPlayerData(float dt) {
        // change to true when server updated pls
        if (g_spectatedPlayer != 0 && false) {
            sendMessage(NMSpectatingNoData {});
        }
        
        auto data = PlayerData{
            .timestamp = m_fields->m_ptTimestamp,
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
            .rot = player->getRotation(),
            .gameMode = gameMode,
            .isHidden = player->m_isHidden || (second && !m_isDualMode),
            .isDashing = player->m_isDashing,
            .isUpsideDown = player->m_isUpsideDown,
            .isMini = player->m_vehicleSize == 0.6f,
            .isGrounded = player->m_isOnGround
        };
    }

    void sendMessage(NetworkThreadMessage msg) {
        g_netMsgQueue.push(msg);
    }

    /* !! DANGER ZONE AHEAD !! */
    // this is code for spectating, and it is horrid.

    void leaveSpectate() {
        g_spectatedPlayer = 0;
        m_fields->m_wasSpectating = false;
        m_player1->m_regularTrail->setVisible(true);
        m_player1->m_playerGroundParticles->setVisible(true);
        m_player2->m_playerGroundParticles->setVisible(true);
        resetLevel();
    }

    // destroyPlayer and vfDChk are noclip
    void destroyPlayer(PlayerObject* p0, GameObject* p1) {
        if (g_spectatedPlayer == 0) PlayLayer::destroyPlayer(p0, p1);
    }

    void vfDChk() {
        if (g_spectatedPlayer == 0) PlayLayer::vfDChk();
    }
    
    // this moves camera to a point with an optional transition
    void moveCameraTo(CCPoint point, float dt = 0.0f) {
        // implementation of cameraMoveX
        stopActionByTag(10);
        m_cameraYLocked = true;
        if (dt == 0.0f) {
            m_cameraPosition.x = point.x;
        } else {
            auto tween = CCActionTween::create(dt, "cTX", this->m_cameraPosition.x, point.x);
            // auto ease = CCEaseInOut::create(tween, 1.8f);
            tween->setTag(10); // this is (**(code **)(*(int *)ease + 0x20))(10);
            auto act1 = runAction(tween);
        }

        // implementation of cameraMoveY
        stopActionByTag(11);
        m_cameraXLocked = true;
        if (dt == 0.0f) {
            m_cameraPosition.y = point.y;
        } else {
            auto yTween = CCActionTween::create(dt, "cTY", this->m_cameraPosition.y, point.y);
            // auto yEase = CCEaseInOut::create(yTween, 1.8f);
            yTween->setTag(11);
            auto act2 = runAction(yTween);
        }
    }

    // this is moveCameraTo but it makes it so that you can see the actual player not in the bottom left corner
    void moveCameraToV2(CCPoint point, float dt = 0.0f) {
        auto winSize = CCDirector::get()->getWinSize();
        auto camX = point.x - winSize.width / 2.4f;
        auto camY = point.y - 140.f;
        moveCameraTo({camX, camY}, dt);
    }

    // this is for duals
    void moveCameraToV2Dual(CCPoint point, float dt = 0.0f) {
        auto winSize = CCDirector::get()->getWinSize();
        auto camX = point.x - winSize.width / 2.4f;
        auto camY = point.y - 50.f; // 30.f
        moveCameraTo({camX, camY}, dt);
    }

    void maybeSyncCamera(float dt, float maxTime = 1.0f) {
        m_fields->m_syncingCamera += dt;
        if (m_fields->m_syncingCamera > maxTime) {
            m_fields->m_syncingCamera = 0.0f;
            moveCameraToV2(m_player1->getPosition());
        }
    }
};
