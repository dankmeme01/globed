#pragma once

#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/GJEffectManager.hpp>
#include <chrono>

#include <global_data.hpp>
#include <ui/game.hpp>
#include <util.hpp>
#include <correction/corrector.hpp>

#define ERROR_CORRECTION 1

using namespace geode::prelude;

const float OVERLAY_PAD_X = 5.f;
const float OVERLAY_PAD_Y = 0.f;

class $modify(ModifiedPlayLayer, PlayLayer) {
    std::unordered_map<int, std::pair<RemotePlayer*, RemotePlayer*>> m_players;
    std::unordered_map<int, PlayerProgressBase*> m_playerProgresses;

    CCLabelBMFont *m_overlay = nullptr;
    long long m_previousPing = -2;
    float m_targetUpdateDelay = 0.f;

    float m_ptTimestamp = 0.0;
    bool m_wasSpectating;

    PlayerProgressNew* m_selfProgress = nullptr;

    // settings
    GlobedGameSettings m_settings;

    bool init(GJGameLevel* level) {
        if (!PlayLayer::init(level)) {
            return false;
        }

        if (g_networkHandler->established()) {
            float delta = 1.f / g_gameServerTps.load();
            m_fields->m_targetUpdateDelay = delta;
            g_pCorrector.setTargetDelta(delta);
        }

        m_fields->m_settings = GlobedGameSettings {
            .displayProgress = Mod::get()->getSettingValue<bool>("show-progress"),
            .newProgress = !Mod::get()->getSettingValue<bool>("old-progress"),
            .playerOpacity = static_cast<unsigned char>(Mod::get()->getSettingValue<int64_t>("player-opacity")),
            .progressScale = static_cast<float>(Mod::get()->getSettingValue<double>("show-progress-scale")),
            .showSelfProgress = Mod::get()->getSettingValue<bool>("show-progress-self"),
            .progressOffset = static_cast<float>(Mod::get()->getSettingValue<int64_t>("show-progress-offset")),
            .progressAltColor = Mod::get()->getSettingValue<bool>("show-progress-altcolor"),
            .hideOverlayCond = Mod::get()->getSettingValue<bool>("overlay-hide-dc"),
            .rpSettings = RemotePlayerSettings {
                .practiceIcon = Mod::get()->getSettingValue<bool>("practice-icon"),
                .secondNameEnabled = Mod::get()->getSettingValue<bool>("show-names-dual"),
                .nameColors = Mod::get()->getSettingValue<bool>("name-colors"),
                .nameOpacity = static_cast<unsigned char>(Mod::get()->getSettingValue<int64_t>("show-names-opacity")),
                .namesEnabled = Mod::get()->getSettingValue<bool>("show-names"),
                .nameScale = static_cast<float>(Mod::get()->getSettingValue<double>("show-names-scale")),
                .nameOffset = static_cast<float>(Mod::get()->getSettingValue<int64_t>("show-names-offset")),
                .deathEffects = Mod::get()->getSettingValue<bool>("death-effects"),
                .defaultDeathEffects = Mod::get()->getSettingValue<bool>("default-death-effects"),
            }
        };

        if (g_debug && level->m_levelID == 0) {
            level->m_levelID = 1;
        }

        g_pCorrector.joinedLevel();

        // 0 is for created levels, skip all sending but still show overlay
        if (level->m_levelID != 0) {
            sendMessage(NMPlayerLevelEntry{level->m_levelID});
            g_currentLevelId = level->m_levelID;

            // scheduled stuff (including PlayLayer::update) doesnt get called while paused
            // use a workaround for it
            // thanks mat <3
            Loader::get()->queueInMainThread([=] {
                auto parent = this->getParent();
                if (parent == nullptr) {
                    log::debug("hey ca7x3 you broke things again :(");
                    return;
                }
                parent->schedule(schedule_selector(ModifiedPlayLayer::sendPlayerData), m_fields->m_targetUpdateDelay);
                parent->schedule(schedule_selector(ModifiedPlayLayer::updateStuff), 1.0f);
                parent->schedule(schedule_selector(ModifiedPlayLayer::updateTick), 0.f);
            });
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

        // add self progress
        if (m_fields->m_settings.showSelfProgress && m_fields->m_settings.displayProgress && m_fields->m_settings.newProgress) {
            m_fields->m_selfProgress = PlayerProgressNew::create(0, m_fields->m_settings.progressOffset);
            static_cast<PlayerProgressNew*>(m_fields->m_selfProgress)->setAltLineColor(m_fields->m_settings.progressAltColor);
            m_fields->m_selfProgress->setIconScale(0.55f * m_fields->m_settings.progressScale);
            m_fields->m_selfProgress->setZOrder(-1);
            m_fields->m_selfProgress->setID("dankmeme.globed/player-progress-self");
            m_fields->m_selfProgress->setAnchorPoint({0.f, 1.f});
            m_fields->m_selfProgress->updateData(*g_accountData.lock());
            m_sliderGrooveSprite->addChild(m_fields->m_selfProgress);

            if (level->m_levelID == 0 || !g_networkHandler->established()) {
                m_fields->m_selfProgress->setVisible(false);
            }
        }

        return true;
    }

    void updateTick(float dt) {
        auto* self = static_cast<ModifiedPlayLayer*>(PlayLayer::get());
        self->m_fields->m_ptTimestamp += dt;

        // skip custom levels
        if (self->m_level->m_levelID == 0) {
            if (self->m_fields->m_overlay && self->m_fields->m_settings.hideOverlayCond) self->m_fields->m_overlay->setVisible(false);
            return;
        }

        // skip disconnected
        if (!g_networkHandler->established()) {
            if (self->m_fields->m_overlay && self->m_fields->m_settings.hideOverlayCond) self->m_fields->m_overlay->setVisible(false);
            return;
        }
        
        // update everyone
        for (const auto &[key, players] : self->m_fields->m_players) {
            g_pCorrector.interpolate(players, dt, key);

            // update progress indicator
            if (self->m_fields->m_settings.displayProgress) {
                self->updateProgress(key, players);
            }
        }

        if (g_spectatedPlayer != 0) {
            if (!self->m_fields->m_players.contains(g_spectatedPlayer)) {
                self->leaveSpectate();
                return;
            }

            self->m_fields->m_selfProgress->setVisible(false);
            self->m_isTestMode = true; // disable progress
            self->m_fields->m_wasSpectating = true;

            auto& data = self->m_fields->m_players.at(g_spectatedPlayer);
            self->m_player1->m_position = data.first->getPosition();
            self->m_player2->m_position = data.second->getPosition();
            self->m_player1->setVisible(false);
            self->m_player2->setVisible(false);

            // disable trails and various particles
            self->m_player1->m_playerGroundParticles->setVisible(false);
            self->m_player2->m_playerGroundParticles->setVisible(false);

            self->m_player1->m_waveTrail->setVisible(false);
            self->m_player2->m_waveTrail->setVisible(false);

            self->m_player1->m_regularTrail->setVisible(false);
            self->m_player2->m_regularTrail->setVisible(false);

            self->moveCameraTo({data.first->camX, data.first->camY}, 0.0f);
            self->maybeSyncMusic(data.first->haltedMovement);

            // log::debug("player pos: {}; camera pos: {}, {}", data.first->getPosition(), self->m_cameraPosition.x, self->m_cameraPosition.y);
        } else if (g_spectatedPlayer == 0 && self->m_fields->m_wasSpectating) {
            self->leaveSpectate();
        }

        self->updateSelfProgress();

        if (g_debug) {
            // self->m_player1->setOpacity(64);
            // self->m_player2->setOpacity(64);
        }
    }

    // updateStuff is update() but less time-sensitive, runs every second rather than every frame.
    void updateStuff(float dt) {
        auto* self = static_cast<ModifiedPlayLayer*>(PlayLayer::get());
        if (self->m_fields->m_overlay != nullptr && self->m_level->m_levelID != 0) {
            // minor optimization, don't update if ping is the same as last tick
            long long currentPing = g_gameServerPing.load();
            if (currentPing != self->m_fields->m_previousPing) {
                self->m_fields->m_previousPing = currentPing;
                if (currentPing == -1) {
                    self->m_fields->m_overlay->setString("Not connected");
                } else {
                    self->m_fields->m_overlay->setString(fmt::format("{} ms", currentPing).c_str());
                }
            }
        }

        // skip custom levels
        if (self->m_level->m_levelID == 0)
            return;

        // skip disconnected
        if (!g_networkHandler->established())
            return;

        // remove players that left the level
        for (const auto &playerId : g_pCorrector.getGonePlayers()) {
            if (self->m_fields->m_players.contains(playerId)) {
                self->removePlayer(playerId);
            }
        }

        // add players that aren't on the level
        for (const auto &playerId : g_pCorrector.getNewPlayers()) {
            if (!self->m_fields->m_players.contains(playerId)) {
                self->addPlayer(playerId);
            }
        }

        // update players with default data
        for (const auto &[key, player] : self->m_fields->m_players) {
            if (player.first->isDefault || player.second->isDefault) {
                auto dataCache = g_accDataCache.lock();
                // are they in cache? then just call updateData on RemotePlayers
                if (dataCache->contains(key)) {
                    auto playerData = dataCache->at(key);
                    player.first->updateData(playerData);
                    player.second->updateData(playerData);
                    // make sure to update opacities!
                    player.first->setOpacity(self->m_fields->m_settings.playerOpacity);
                    player.second->setOpacity(self->m_fields->m_settings.playerOpacity);
                    // and progress!
                    if (self->m_fields->m_playerProgresses.contains(key))
                        self->m_fields->m_playerProgresses.at(key)->updateData(playerData);

                } else {
                    // if not, request them from the server.
                    self->sendMessage(NMRequestPlayerAccount { .playerId = key });
                }
            }
        }
    }

    void updateProgress(int playerId, const std::pair<RemotePlayer*, RemotePlayer*>& players) {
        auto playerPos = players.first->getPosition();
        auto progress = m_fields->m_playerProgresses.at(playerId);
        float progressVal = getProgressVal(playerPos.x);

        if (progress->m_isDefault) {
            auto cache = g_accDataCache.lock();
            if (cache->contains(playerId)) {
                progress->updateData(cache->at(playerId));
            }
        }
        
        // new progress
        if (m_fields->m_settings.newProgress) {
            auto progressBar = m_sliderGrooveSprite;
            if (!progressBar || !progressBar->isVisible()) {
                progress->setVisible(false);
                return;
            };

            progress->setVisible(true);

            updateNewProgress(progressVal, static_cast<PlayerProgressNew*>(progress));
        } else { // old progress
            if (!isPlayerVisible(players.first->getPosition()) || g_debug) {
                bool onRightSide = playerPos.x > m_player1->getPositionX();
                progress->updateValues(progressVal * 100, onRightSide);
                progress->setVisible(true);
                progress->setAnchorPoint({onRightSide ? 1.f : 0.f, 0.5f});
                float prHeight;

                auto winSize = CCDirector::get()->getWinSize();
                auto maxHeight = winSize.height * 0.85f;
                auto minHeight = winSize.height - maxHeight;
                prHeight = minHeight + (maxHeight - minHeight) * progressVal;
                progress->setPosition({onRightSide ? (winSize.width - 5.f) : 5.f, prHeight});
            } else {
                progress->setVisible(false);
            }
        }
    }

    void updateSelfProgress() {
        if (!m_fields->m_selfProgress) return;
        auto progress = m_fields->m_selfProgress;

        auto progressBar = m_sliderGrooveSprite;
        if (!progressBar || !progressBar->isVisible()) {
            progress->setVisible(false);
            return;
        }

        progress->setVisible(true);
        updateNewProgress(getProgressVal(m_player1->getPositionX()), progress);
    }

    // progressVal is between 0.f and 1.f
    void updateNewProgress(float progressVal, PlayerProgressNew* progress) {
        auto progressBar = m_sliderGrooveSprite;
        auto pbSize = progressBar->getScaledContentSize();
        auto prOffset = (pbSize.width - 2.f) * progressVal;

        if (progressVal < 0.01f || progressVal > 0.99f) {
            progress->hideLine();
        } else {
            progress->showLine();
        }

        progress->setPosition({
            prOffset,
            8.f,
        });
    }

    // returns progress value from 0.f to 1.f
    inline float getProgressVal(float x) {
        return std::clamp(x / (m_levelLength + 50), 0.0f, 0.99f);
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
        m_fields->m_players.at(playerId).first->removeFromParent();
        m_fields->m_players.at(playerId).second->removeFromParent();
        m_fields->m_players.erase(playerId);
        m_fields->m_playerProgresses.at(playerId)->removeFromParent();
        m_fields->m_playerProgresses.erase(playerId);
        log::debug("removed player {}", playerId);
    }

    void addPlayer(int playerId) {
        auto playZone = m_objectLayer;

        RemotePlayer *player1, *player2;

        // note - playerprogress must be initialized here, before locking g_accDataCache
        PlayerProgressBase* progress;
        if (m_fields->m_settings.newProgress) {
            progress = PlayerProgressNew::create(playerId, m_fields->m_settings.progressOffset);
            progress->setAnchorPoint({0.f, 1.f});
            static_cast<PlayerProgressNew*>(progress)->setIconScale(0.55f * m_fields->m_settings.progressScale);
            static_cast<PlayerProgressNew*>(progress)->setAltLineColor(m_fields->m_settings.progressAltColor);
            progress->setZOrder(-1);
        } else {
            progress = PlayerProgress::create(playerId);
            progress->setScale(1.0f * m_fields->m_settings.progressScale);
            progress->setZOrder(9);
        }

        progress->setID(fmt::format("dankmeme.globed/player-progress-{}", playerId));
        if (!m_fields->m_settings.displayProgress) {
            progress->setVisible(false);
        }

        if (m_fields->m_settings.newProgress) {
            m_sliderGrooveSprite->addChild(progress);
        } else {
            this->addChild(progress);
        }

        auto iconCache = g_accDataCache.lock();
        if (iconCache->contains(playerId)) {
            auto icons = iconCache->at(playerId);
            player1 = RemotePlayer::create(false, m_fields->m_settings.rpSettings, icons);
            player2 = RemotePlayer::create(true, m_fields->m_settings.rpSettings, icons);
        } else {
            player1 = RemotePlayer::create(false, m_fields->m_settings.rpSettings);
            player2 = RemotePlayer::create(true, m_fields->m_settings.rpSettings);
        }

        player1->setZOrder(0);
        player1->setID(fmt::format("dankmeme.globed/player-icon-{}", playerId));
        player1->setAnchorPoint({0.5f, 0.5f});
        player1->setOpacity(m_fields->m_settings.playerOpacity);

        playZone->addChild(player1);

        player2->setZOrder(0);
        player2->setID(fmt::format("dankmeme.globed/player-icon-dual-{}", playerId));
        player2->setAnchorPoint({0.5f, 0.5f});
        player2->setOpacity(m_fields->m_settings.playerOpacity);

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
        g_currentLevelId = 0;
    }

    void sendPlayerData(float dt) {
        auto* self = static_cast<ModifiedPlayLayer*>(PlayLayer::get());
        if (!g_debug && g_currentLevelId < 1) {
            return;
        }

        if (g_spectatedPlayer != 0) {
            self->sendMessage(NMSpectatingNoData {});
            return;
        }
        
        auto data = PlayerData {
            .timestamp = self->m_fields->m_ptTimestamp,
            .player1 = self->gatherSpecificPlayerData(self->m_player1, false),
            .player2 = self->gatherSpecificPlayerData(self->m_player2, true),
            .camX = self->m_cameraPosition.x,
            .camY = self->m_cameraPosition.y,
            .isPractice = self->m_isPracticeMode,
            .isDead = self->m_isDead,
            .isPaused = self->getParent()->getChildByID("PauseLayer") != nullptr,
        };

        self->sendMessage(data);
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
            .x = player->getPositionX(),
            .y = player->getPositionY(),
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
        log::debug("stop spectating {}", g_spectatedPlayer.load());
        g_spectatedPlayer = 0;
        m_fields->m_wasSpectating = false;
        m_player1->setVisible(true);

        m_player1->m_playerGroundParticles->setVisible(true);
        m_player2->m_playerGroundParticles->setVisible(true);

        m_player1->m_waveTrail->setVisible(true);
        m_player2->m_waveTrail->setVisible(true);

        m_player1->m_regularTrail->setVisible(true);
        m_player2->m_regularTrail->setVisible(true);

        m_isTestMode = false;

        if (m_fields->m_settings.showSelfProgress && m_fields->m_settings.displayProgress && m_fields->m_settings.newProgress) {
            m_fields->m_selfProgress->setVisible(true);
        }
        resetLevel();
    }

    // this *may* have been copied from GDMO
    // called every frame when spectating
    void maybeSyncMusic(bool isPlayerMoving) {
    #ifndef GEODE_IS_ANDROID
        auto engine = FMODAudioEngine::sharedEngine();

        if (!isPlayerMoving) {
            engine->m_globalChannel->setPaused(true);
        }

        engine->m_globalChannel->setPaused(false);

        float f = this->timeForXPos(m_player1->getPositionX());
        unsigned int p;
        float offset = m_levelSettings->m_songOffset * 1000;

        engine->m_globalChannel->getPosition(&p, FMOD_TIMEUNIT_MS);
        if (std::abs((int)(f * 1000) - (int)p + offset) > 60 && !m_hasCompletedLevel) {
            engine->m_globalChannel->setPosition(
                static_cast<uint32_t>(f * 1000) + static_cast<uint32_t>(offset), FMOD_TIMEUNIT_MS);
        }
    #endif
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
        m_cameraYLocked = true;
        m_cameraXLocked = true;

        // implementation of cameraMoveX
        stopActionByTag(10);
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
        auto camY = point.y - winSize.height - 16.f;
        moveCameraTo({camX, camY}, dt);
    }

    // void maybeSyncCamera(float dt, float maxTime = 1.0f) {
    //     m_fields->m_syncingCamera += dt;
    //     if (m_fields->m_syncingCamera > maxTime) {
    //         m_fields->m_syncingCamera = 0.0f;
    //         moveCameraToV2(m_player1->getPosition());
    //     }
    // }
};
