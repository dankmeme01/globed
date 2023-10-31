#pragma once

#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <chrono>
#include <unordered_set>

#include <global_data.hpp>
#include <ui/game.hpp>
#include <util.hpp>
#include <correction/corrector.hpp>

using namespace geode::prelude;

const float OVERLAY_PAD_X = 5.f;
const float OVERLAY_PAD_Y = 0.f;
const float CHAT_LINE_LENGTH = 190.f;

class DummyFriendlistNode : public UserListDelegate {
public:
    void getUserListFinished(CCArray* userArray, UserListType listType) override {
        if (listType == UserListType::Friends) {
            log::debug("fetched {} friends", userArray->count());

            for (auto* elem : CCArrayExt<GJUserScore>(userArray)) {
                friends.insert(elem->m_accountID);
            }
        }

        GameLevelManager::get()->m_userListDelegate = nullptr;
    }

    void getUserListFailed(UserListType listType, GJErrorCode errorCode) override {
        log::warn("failed to obtain friend list, err: {}", (int)errorCode);
        GameLevelManager::get()->m_userListDelegate = nullptr;
    }

    std::unordered_set<int> friends;
};

class $modify(ModifiedPlayLayer, PlayLayer) {
    std::unordered_map<int, std::pair<RemotePlayer*, RemotePlayer*>> m_players;
    std::unordered_map<int, PlayerProgressBase*> m_playerProgresses;

    CCLabelBMFont *m_overlay = nullptr;
    CCNode* m_progressWrapperNode = nullptr;
    float m_targetUpdateDelay = 0.f;

    float m_ptTimestamp = 0.0;

    float m_spectateLastReset = 0.0f;
    bool m_wasSpectating = false;
    bool m_readyForMP = false;
    bool m_hasStartPositions = false;
    bool m_justExited = false;

    DummyFriendlistNode m_friendlist; // its not even a node lol

    PlayerProgressNew* m_selfProgress = nullptr;

    // CCCappedQueue isn't a cocos thing, it's defined in data/capped_queue.hpp
    CCCappedQueue<10> m_messageList;
    CCTextInputNode* m_messageInput;
    CCMenuItemSpriteExtra* m_sendBtn;
    CCMenuItemSpriteExtra* m_chatToggleBtn;
    CCSprite* m_chatBackgroundSprite;
    CCNode* m_chatBox = nullptr;
    bool m_chatExpanded = true;

    // blocklist/whitelist of chat users
    std::unordered_set<int> m_chatBlacklist, m_chatWhitelist;

    // settings
    GlobedGameSettings m_settings;

    bool init(GJGameLevel* level) {
        if (!PlayLayer::init(level)) {
            return false;
        }

        g_messages.popAll(); // clear the message queue

        m_fields->m_hasStartPositions = m_isTestMode;

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
            .chatWhitelist = Mod::get()->getSettingValue<bool>("chat-whitelist"),
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

        // only do stuff if we are not in editor (or if we are in debug) and connected
        m_fields->m_readyForMP = (g_debug || level->m_levelType != GJLevelType::Editor) && g_networkHandler->established();

        if (m_fields->m_readyForMP) {
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
            m_fields->m_overlay = CCLabelBMFont::create(level->m_levelType == GJLevelType::Editor ? "N/A (editor)" : "Not connected", "bigFont.fnt");

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

        // add the progressbar wrapper
        m_fields->m_progressWrapperNode = CCNode::create();
        m_fields->m_progressWrapperNode->setZOrder(-1);
        m_fields->m_progressWrapperNode->setAnchorPoint({0.f, 0.f});
        m_fields->m_progressWrapperNode->setContentSize(m_sliderGrooveSprite->getContentSize());
        m_sliderGrooveSprite->addChild(m_fields->m_progressWrapperNode);

        // add self progress
        if (m_fields->m_settings.showSelfProgress && m_fields->m_settings.displayProgress && m_fields->m_settings.newProgress) {
            m_fields->m_selfProgress = PlayerProgressNew::create(0, m_fields->m_settings.progressOffset);
            static_cast<PlayerProgressNew*>(m_fields->m_selfProgress)->setAltLineColor(m_fields->m_settings.progressAltColor);
            m_fields->m_selfProgress->setIconScale(0.55f * m_fields->m_settings.progressScale);
            m_fields->m_selfProgress->setID("dankmeme.globed/player-progress-self");
            m_fields->m_selfProgress->setAnchorPoint({0.f, 1.f});
            m_fields->m_selfProgress->updateData(*g_accountData.lock());
            m_fields->m_progressWrapperNode->addChild(m_fields->m_selfProgress);

            if (!m_fields->m_readyForMP) {
                m_fields->m_selfProgress->setVisible(false);
            }
        }

        if (g_networkHandler->established()) {
            auto message_input = CCTextInputNode::create(175.0 * 2.0, 32.0, "Text message", "chatFont.fnt");
            message_input->setID("dankmeme.globed/chat-input");
            message_input->setMaxLabelWidth(175.0 * 2.0);
            message_input->setAnchorPoint({0.0, 0.0});
            message_input->setAllowedChars("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890!\"#$%&/()=?+.,;:_- {}[]@'*");
            message_input->setLabelPlaceholderColor({130, 130, 130});
            message_input->setPosition({0.0, 6.0});
            message_input->setScale(0.5);
            message_input->setZOrder(10);
            message_input->m_maxLabelLength = 80;

            this->addChild(message_input);
            m_fields->m_messageInput = message_input;

            auto menu = CCMenu::create();
            menu->setPosition({0.0, 0.0});
            menu->setAnchorPoint({0.0, 0.0});
            menu->setZOrder(10);

            //PLACEHOLDER SPRITE!!!!!!!!
            auto send_sprite = CircleButtonSprite::createWithSpriteFrameName("edit_upBtn_001.png", 1.0f);
            send_sprite->setRotation(90.f);
            send_sprite->setScale(0.35);

            auto send_button = CCMenuItemSpriteExtra::create(send_sprite, this, menu_selector(ModifiedPlayLayer::onSendMessage));
            send_button->setAnchorPoint({0.0, 0.0});
            send_button->setID("dankmeme.globed/chat-send");
            send_button->setPosition({180.0, 2.5});

            m_fields->m_sendBtn = send_button;
            menu->addChild(send_button);

            auto toggle_chat_sprite = CCSprite::createWithSpriteFrameName("navArrowBtn_001.png");
            toggle_chat_sprite->setScale(0.35);
            toggle_chat_sprite->setRotation(90.f);

            auto toggle_chat_button = CCMenuItemSpriteExtra::create(toggle_chat_sprite, this, menu_selector(ModifiedPlayLayer::onToggleChat));
            toggle_chat_button->setAnchorPoint({0.0, 0.0});
            toggle_chat_button->setPosition({200.0 / 2.0 - 5.0, 90.0});
            toggle_chat_button->setZOrder(10);

            m_fields->m_chatToggleBtn = toggle_chat_button;
            menu->addChild(toggle_chat_button);

            this->addChild(menu);

            auto bg_sprite = CCSprite::create("square02_001.png");
            bg_sprite->setOpacity(100);
            bg_sprite->setPositionX(2.0);
            bg_sprite->setScaleX(2.6);
            bg_sprite->setScaleY(1.35);
            bg_sprite->setAnchorPoint({0.0, 0.0});
            bg_sprite->setZOrder(9);

            m_fields->m_chatBackgroundSprite = bg_sprite;
            this->addChild(bg_sprite);

            float chatboxOffset = message_input->getScaledContentSize().height + 5.f;

            m_fields->m_chatBox = CCNode::create();
            m_fields->m_chatBox->setZOrder(10);
            m_fields->m_chatBox->setAnchorPoint({0.f, 0.f});
            m_fields->m_chatBox->setPosition({5.f, chatboxOffset});
            m_fields->m_chatBox->setContentSize(bg_sprite->getScaledContentSize() - CCSize{0.f, chatboxOffset});
            m_fields->m_chatBox->setLayout(
                ColumnLayout::create()
                    ->setGap(1.f)
                    ->setAutoScale(false)
                    ->setAxisAlignment(AxisAlignment::Start)
                    ->setAxisReverse(true)
                    ->setCrossAxisLineAlignment(AxisAlignment::Start)
            );
            m_fields->m_chatBox->setID("dankmeme.globed/chat-box");
            this->addChild(m_fields->m_chatBox);

            // setup chat blacklist and whitelist
            auto blacklistStr = Mod::get()->getSavedValue<std::string>("chat-blacklist");
            auto whitelistStr = Mod::get()->getSavedValue<std::string>("chat-whitelist");
            m_fields->m_chatBlacklist = globed_util::parseIdList(blacklistStr);
            m_fields->m_chatWhitelist = globed_util::parseIdList(whitelistStr);

            // fetch friendlist
            GameLevelManager::get()->m_userListDelegate = &m_fields->m_friendlist;
            GameLevelManager::get()->getUserList(UserListType::Friends);
        }


        return true;
    }

    void onToggleChat(CCObject*) {
        m_fields->m_chatExpanded = !m_fields->m_chatExpanded;

        if (m_fields->m_chatExpanded) {
            auto toggle_chat_button = m_fields->m_chatToggleBtn;
            toggle_chat_button->setPosition({200.0 / 2.0 - 5.0, 90.0});
            static_cast<CCSprite*>(toggle_chat_button->getChildren()->objectAtIndex(0))->setRotation(90.f);

            m_fields->m_chatBackgroundSprite->setScaleY(1.35);

            for (auto* msg : m_fields->m_messageList.extract<ChatMessage>()) {
                msg->setVisible(true);
            }

            m_fields->m_messageInput->setPosition({0.0, 6.0});
            m_fields->m_sendBtn->setPosition({180.0, 2.5});
        } else {
            auto toggle_chat_button = m_fields->m_chatToggleBtn;
            toggle_chat_button->setPosition({200.0 / 2.0 - 5.0, -2.0});
            static_cast<CCSprite*>(toggle_chat_button->getChildren()->objectAtIndex(0))->setRotation(270.f);

            m_fields->m_chatBackgroundSprite->setScaleY(0.2);

            for (auto* msg : m_fields->m_messageList.extract<ChatMessage>()) {
                msg->setVisible(false);
            }

            //shhhhhh..
            m_fields->m_messageInput->setPosition({-100.0, -100.0});
            m_fields->m_sendBtn->setPosition({-100.0, -100.0});
        }

    }

    bool isUserBlocked(int playerId) {
        // if it's ourselves, then allow messages
        if (playerId == g_networkHandler->getAccountId()) {
            return false;
        }

        // explicit blacklist/whitelist takes precedence before anything else
        if (m_fields->m_chatBlacklist.contains(playerId)) {
            return true; // if explicitly blocked, return true
        } else if (m_fields->m_chatWhitelist.contains(playerId)) {
            return false; // if explicitly allowed, return false
        }

        // now check if they are a friend
        if (m_fields->m_friendlist.friends.contains(playerId)) {
            return false; // if a friend, return false
        }

        // now check if we have blacklist or whitelist enabled
        if (m_fields->m_settings.chatWhitelist) {
            return true; // whitelist - block by default
        }

        return false; // blacklist - unblocked by default
    }

    void chatBlockUser(int playerId) {
        m_fields->m_chatWhitelist.erase(playerId);
        m_fields->m_chatBlacklist.insert(playerId);
        saveChatUserLists();
    }

    void chatUnblockUser(int playerId) {
        m_fields->m_chatBlacklist.erase(playerId);
        m_fields->m_chatWhitelist.insert(playerId);
        saveChatUserLists();
    }

    // saves blacklist and whitelist for chat users
    void saveChatUserLists() {
        auto blacklistStr = globed_util::serialzieIdList(m_fields->m_chatBlacklist);
        auto whitelistStr = globed_util::serialzieIdList(m_fields->m_chatWhitelist);

        Mod::get()->setSavedValue("chat-blacklist", blacklistStr);
        Mod::get()->setSavedValue("chat-whitelist", whitelistStr);
    }

    //TODO make enter send the message and clicking anyhwere else deselect it
    void onSendMessage(CCObject*) {
        std::string string = m_fields->m_messageInput->getString();
        if (!string.empty())
            sendMessage(NMSendTextMessage { .message = std::string(string) });
        m_fields->m_messageInput->setString("");
        //deselect it
        m_fields->m_messageInput->onClickTrackNode(false);
    }

    void updateTick(float dt) {
        auto* self = static_cast<ModifiedPlayLayer*>(PlayLayer::get());
        self->m_fields->m_ptTimestamp += dt;

        if (!self->m_fields->m_readyForMP) {
            if (self->m_fields->m_overlay && self->m_fields->m_settings.hideOverlayCond) self->m_fields->m_overlay->setVisible(false);
            return;
        }

        float mirrorScaleX = 2.f - (self->m_mirrorTransition * 2.f) - 1.f;
        // update everyone
        for (const auto &[key, players] : self->m_fields->m_players) {
            g_pCorrector.interpolate(players, dt, key);
            // i hate this kill me

            if (players.first->labelName)
                players.first->labelName->setScaleX(mirrorScaleX);

            if (players.second->labelName)
                players.second->labelName->setScaleX(mirrorScaleX);

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

            self->m_isTestMode = true; // disable progress
            self->m_fields->m_wasSpectating = true;

            auto& data = self->m_fields->m_players.at(g_spectatedPlayer);

            if (data.first->justDied) {
                // play death sound
                GameSoundManager::get()->playEffect("explode_11.ogg", 100.0f, 100.0f, 100.0f);
                data.first->justDied = false;
            }

            // if we are travelling back in time, also reset
            auto posPrev = self->m_player1->m_position.x;
            auto posNew = data.first->getPositionX();
            bool maybeRestartedLevel = (posNew < posPrev) && (posPrev - posNew > 25.f) || (posNew < 50.f && posPrev > 50.f);
            bool restartedRecently = std::fabs(self->m_fields->m_ptTimestamp - self->m_fields->m_spectateLastReset) <= 0.1f;

            if (data.first->justRespawned || (maybeRestartedLevel && !restartedRecently)) {
                // log::debug("resetting level because player just respawned, old: {}, new: {}", posPrev, posNew);
                data.first->justRespawned = false;
                self->m_player1->m_position = CCPoint{0.f, 0.f};
                self->m_player2->m_position = CCPoint{0.f, 0.f};
                self->resetLevel();
                self->maybeUnpauseMusic();
                self->m_cameraXLocked = true;
                self->m_cameraYLocked = true;
                self->m_fields->m_spectateLastReset = self->m_fields->m_ptTimestamp;
                return;
            }

            if (posNew < posPrev && restartedRecently && (posPrev - posNew) > 15.f) {
                // log::debug("just restarted and not sillly, old: {}, new: {}", posPrev, posNew);
                self->m_cameraXLocked = true;
                self->m_cameraYLocked = true;
                return;
            }

            if (restartedRecently) {
                // log::debug("just restarted and normal tick");
                self->m_player1->m_position = CCPoint{0.f, 0.f};
                self->m_player2->m_position = CCPoint{0.f, 0.f};
                self->moveCameraTo({data.first->camX, data.first->camY}, 0.0f);
                return;
            }

            // log::debug("normal spectate tick, old: {}, new: {}", posPrev, posNew);

            if (self->m_fields->m_selfProgress) {
                self->m_fields->m_selfProgress->setVisible(false);
            }

            self->m_player1->m_position = data.first->getPosition();
            self->m_player2->m_position = data.second->getPosition();

            self->m_player1->setScale(data.first->getScale());
            self->m_player2->setScale(data.second->getScale());

            self->m_player1->setVisible(false);
            self->m_player2->setVisible(false);

            // disable trails and various particles
            self->m_player1->m_playerGroundParticles->setVisible(false);
            self->m_player2->m_playerGroundParticles->setVisible(false);

            self->m_player1->m_shipBoostParticles->setVisible(false);
            self->m_player1->m_shipBoostParticles->setVisible(false);

            self->m_player1->m_waveTrail->setVisible(false);
            self->m_player2->m_waveTrail->setVisible(false);

            self->m_player1->m_regularTrail->setVisible(false);
            self->m_player2->m_regularTrail->setVisible(false);

            self->m_player1->toggleGhostEffect((GhostType)0);
            self->m_player2->toggleGhostEffect((GhostType)0);

            self->moveCameraTo({data.first->camX, data.first->camY}, 0.0f);
            if (!self->isGamePaused()) {
                self->maybeSyncMusic(!data.first->haltedMovement);
            }

            // log::debug("player pos: {}; camera pos: {}, {}", data.first->getPosition(), self->m_cameraPosition.x, self->m_cameraPosition.y);
        } else if (g_spectatedPlayer == 0 && self->m_fields->m_wasSpectating) {
            self->leaveSpectate();
        }

        self->updateMessages();

        self->updateSelfProgress();
    }

    // updateStuff is update() but less time-sensitive, runs every second rather than every frame.
    void updateStuff(float dt) {
        auto* self = static_cast<ModifiedPlayLayer*>(PlayLayer::get());

        if (self->m_fields->m_overlay != nullptr && (g_debug || self->m_level->m_levelType != GJLevelType::Editor)) {
            // minor optimization, don't update if ping is the same as last tick
            long long currentPing = g_gameServerPing.load();
            if (currentPing == -1) {
                self->m_fields->m_overlay->setString("Not connected");
            } else {
                self->m_fields->m_overlay->setString(fmt::format("{} ms", currentPing).c_str());
            }
        }

        if (!self->m_fields->m_readyForMP) {
            return;
        }

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
            if (g_spectatedPlayer == playerId) {
                progress->setVisible(false);
                return;
            }

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
        // Unfortunately, 2.2 will not be coming in October as promised and is delayed to November. It just wouldn't feel right to release an update this big without stable servers and proper bugfixing. I tried my hardest to make it in time, but some work took longer than expected.
        progress->setZOrder(20000);
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

        progress->setZOrder((int)(progressVal * 10000)); // TODO this is so bad

        progress->setPosition({
            prOffset,
            8.f,
        });
    }

    void updateMessages() {
        if (!m_fields->m_chatExpanded)
            return;

        auto dataCache = g_accDataCache.lock();
        for (auto [sender, message] : g_messages.popAll()) {
            if (this->isUserBlocked(sender)) continue;

            ChatMessage* uiMsg;
            if (sender == g_networkHandler->getAccountId()) { // ourselves
                uiMsg = ChatMessage::create(*g_accountData.lock(), message, CHAT_LINE_LENGTH);
            } else if (dataCache->contains(sender)) { // cached player
                uiMsg = ChatMessage::create(dataCache->at(sender), message, CHAT_LINE_LENGTH);
            } else { // unknown player
                uiMsg = ChatMessage::create(DEFAULT_PLAYER_ACCOUNT_DATA, message, CHAT_LINE_LENGTH);
            }

            m_fields->m_chatBox->addChild(uiMsg);
            m_fields->m_messageList.push(uiMsg);
        }

        dataCache.unlock();

        m_fields->m_chatBox->updateLayout();

        //changes to 0.5, 0.5 when clicking on it for some reason
        if (m_fields->m_messageInput != nullptr) m_fields->m_messageInput->setAnchorPoint({0.0, 0.0});
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
            m_fields->m_progressWrapperNode->addChild(progress);
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

        player1->setZOrder(10);
        player1->setID(fmt::format("dankmeme.globed/player-icon-{}", playerId));
        player1->setAnchorPoint({0.5f, 0.5f});
        player1->setOpacity(m_fields->m_settings.playerOpacity);

        playZone->addChild(player1);

        player2->setZOrder(10);
        player2->setID(fmt::format("dankmeme.globed/player-icon-dual-{}", playerId));
        player2->setAnchorPoint({0.5f, 0.5f});
        player2->setOpacity(m_fields->m_settings.playerOpacity);

        playZone->addChild(player2);

        m_fields->m_players.insert(std::make_pair(playerId, std::make_pair(player1, player2)));
        m_fields->m_playerProgresses.insert(std::make_pair(playerId, progress));
        log::debug("added player {}", playerId);
    }

    void onQuit() {
        log::debug("quitting the level");
        if (m_fields->m_wasSpectating) {
            leaveSpectate();
        }

        PlayLayer::onQuit();
        if (m_fields->m_readyForMP)
            sendMessage(NMPlayerLevelExit{});

        g_spectatedPlayer = 0;
        g_currentLevelId = 0;
        m_fields->m_justExited = true;
        GameLevelManager::get()->m_levelManagerDelegate = nullptr;
    }

    void sendPlayerData(float dt) {
        auto* self = static_cast<ModifiedPlayLayer*>(PlayLayer::get());
        if (!self->m_fields->m_readyForMP || self->m_fields->m_justExited) {
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
            .isPaused = self->isGamePaused(),
        };

        self->sendMessage(data);
    }

    bool isGamePaused() {
        // TODO this is temporary, idk why node ids are not on android
#ifdef GEODE_IS_ANDROID
        CCObject* androbj;
        CCARRAY_FOREACH(getParent()->getChildren(), androbj) {
            if (dynamic_cast<PauseLayer*>(androbj)) {
                return true;
            }
        }
        return false;
#endif

        return this->getParent()->getChildByID("PauseLayer") != nullptr;
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

        m_player1->m_shipBoostParticles->setVisible(true);
        m_player1->m_shipBoostParticles->setVisible(true);

        m_player1->m_waveTrail->setVisible(true);
        m_player2->m_waveTrail->setVisible(true);

        m_player1->m_regularTrail->setVisible(true);
        m_player2->m_regularTrail->setVisible(true);

        m_isTestMode = m_fields->m_hasStartPositions;

        if (m_fields->m_settings.showSelfProgress && m_fields->m_settings.displayProgress && m_fields->m_settings.newProgress) {
            m_fields->m_selfProgress->setVisible(true);
        }

        resetLevel();
        maybeUnpauseMusic();

        m_isTestMode = m_fields->m_hasStartPositions; // i dont even know anymore
    }

    void levelComplete() {
        if (m_fields->m_wasSpectating) m_isTestMode = true;
        PlayLayer::levelComplete();
    }

    // thanks ninxout from crystal client
    void checkCollisions(PlayerObject* player, float g) {
        PlayLayer::checkCollisions(player, g);
        if (m_fields->m_wasSpectating) {
            m_antiCheatPassed = true;
            m_shouldTryToKick = false;
            m_hasCheated = false;
            m_inlineCalculatedKickTime = 0.0;
            m_accumulatedKickCounter = 0;
            m_kickCheckDeltaSnapshot = (float)std::time(nullptr);
        }
    }

    // this *may* have been copied from GDMO
    // called every frame when spectating
    void maybeSyncMusic(bool isPlayerMoving) {
#ifndef GEODE_IS_MACOS
        auto engine = FMODAudioEngine::sharedEngine();

        if (!isPlayerMoving) {
            engine->m_globalChannel->setPaused(true);
            return;
        }

        engine->m_globalChannel->setPaused(false);

        float f = this->timeForXPos(m_player1->getPositionX());
        unsigned int p;
        float offset = m_levelSettings->m_songOffset * 1000;

        engine->m_globalChannel->getPosition(&p, FMOD_TIMEUNIT_MS);
        if (std::abs((int)(f * 1000) - (int)p + offset) > 100 && !m_hasCompletedLevel) {
            engine->m_globalChannel->setPosition(
                static_cast<uint32_t>(f * 1000) + static_cast<uint32_t>(offset), FMOD_TIMEUNIT_MS);
        }
#endif
    }

    void maybeUnpauseMusic() {
#ifndef GEODE_IS_MACOS
        FMODAudioEngine::sharedEngine()->m_globalChannel->setPaused(isGamePaused());
#endif
    }

    // destroyPlayer and vfDChk are noclip
    void destroyPlayer(PlayerObject* p0, GameObject* p1) {
        if (!m_fields->m_wasSpectating) PlayLayer::destroyPlayer(p0, p1);
    }

    void vfDChk() {
        if (!m_fields->m_wasSpectating) PlayLayer::vfDChk();
    }

    // this moves camera to a point with an optional transition
    void moveCameraTo(CCPoint point, float dt = 0.0f) {
        m_cameraYLocked = true;
        m_cameraXLocked = true;

        stopActionByTag(10);
        m_cameraPosition.x = point.x;

        stopActionByTag(11);
        m_cameraPosition.y = point.y;
    }

    // // this is moveCameraTo but it makes it so that you can see the actual player not in the bottom left corner
    // void moveCameraToV2(CCPoint point, float dt = 0.0f) {
    //     auto winSize = CCDirector::get()->getWinSize();
    //     auto camX = point.x - winSize.width / 2.4f;
    //     auto camY = point.y - 140.f;
    //     moveCameraTo({camX, camY}, dt);
    // }

    // // this is for duals
    // void moveCameraToV2Dual(CCPoint point, float dt = 0.0f) {
    //     auto winSize = CCDirector::get()->getWinSize();
    //     auto camX = point.x - winSize.width / 2.4f;
    //     auto camY = point.y - winSize.height - 16.f;
    //     moveCameraTo({camX, camY}, dt);
    // }
};