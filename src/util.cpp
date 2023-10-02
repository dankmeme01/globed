#include "util.hpp"

#include <Geode/utils/web.hpp>
#include <global_data.hpp>

namespace web = geode::utils::web;

namespace globed_util {
    /* Returns a scene with just this layer */
    CCScene* sceneWithLayer(CCNode* layer) {
        auto scene = CCScene::create();
        
        layer->setPosition({0, 0});
        scene->addChild(layer);

        return scene;
    }

    void handleErrors() {
        auto errors = g_errMsgQueue.popAll();
        // if there are too many errors, prevent showing an endless wall of popups
        if (errors.size() > 2) {
            globed_util::errorPopup(errors[0]);
            globed_util::errorPopup(errors[1]);
            return;
        }

        for (const auto& error : errors) {
            globed_util::errorPopup(error);
        }

        auto warnings = g_warnMsgQueue.popAll();
        for (const auto& error : warnings) {
            Notification::create(error, NotificationIcon::Warning, 3.0f)->show();
        }
    }

    bool isNumeric(const std::string& str) {
        for (char c : str) {
            if (!std::isdigit(c)) {
                return false;
            }
        }
        return true;
    }

    IconType igmToIconType(IconGameMode mode) {
        switch (mode) {
        case IconGameMode::CUBE:
            return IconType::Cube;
        case IconGameMode::SHIP:
            return IconType::Ship;
        case IconGameMode::BALL:
            return IconType::Ball;
        case IconGameMode::UFO:
            return IconType::Ufo;
        case IconGameMode::WAVE:
            return IconType::Wave;
        case IconGameMode::ROBOT:
            return IconType::Robot;
        case IconGameMode::SPIDER:
            return IconType::Spider;
        case IconGameMode::NONE:
            throw std::invalid_argument("igmToIconType in util.cpp got called with an invalid IconGameMode");
        }
    }

    namespace net {
        bool updateGameServers(const std::string& url) {
            auto serverListRes = web::fetch(url);
            if (serverListRes.isErr()) {
                auto error = serverListRes.unwrapErr();
                log::warn("failed to fetch game servers: {}: {}", url, error);
                auto errMessage = fmt::format("Globed failed to fetch game servers from the central server. This is either a problem with networking on your system, or a misconfiguration of the server. If you believe this is not a problem on your side, please contact the owner of the server.\n\nError: <cy>{}</c>", error);

                g_errMsgQueue.push(errMessage);
                return false;
            }

            auto serverList = serverListRes.unwrap();

            auto servers = json::parse(serverList).as_array();

            std::lock_guard<std::mutex> lock(g_gameServerMutex);
            
            g_gameServers.clear();
            for (auto server : servers) {
                auto obj = server.as_object();

                auto name = obj["name"].as_string();
                auto id = obj["id"].as_string();
                auto region = obj["region"].as_string();
                auto address = obj["address"].as_string();

                g_gameServers.push_back(GameServer {name, id, region, address});
            }

            return true;
        }

        std::pair<std::string, unsigned short> splitAddress(const std::string& address) {
            auto colonPos = address.find(":");
            if (colonPos != std::string::npos) {
                auto ip = address.substr(0, colonPos);
                auto portString = address.substr(colonPos + 1);

                unsigned short port;
                std::istringstream portStream(portString);
                if (!(portStream >> port)) {
                    throw std::invalid_argument("invalid port number");
                }

                return std::make_pair(ip, port);
            } else {
                return std::make_pair(address, 0);
            }
        }

        void testCentralServer(const std::string& modVersion, std::string url) {
            if (url.empty()) {
                log::info("Central server was set to an empty string, disabling.");
                return;
            }

            log::info("Trying to switch to central server {}", url);

            if (!url.ends_with('/')) {
                url += '/';
            }

            auto versionURL = url + "version";
            auto serversURL = url + "servers";

            auto serverVersionRes = web::fetch(versionURL);

            if (serverVersionRes.isErr()) {
                auto error = serverVersionRes.unwrapErr();
                log::warn("failed to fetch server version: {}: {}", versionURL, error);

                std::string errMessage = fmt::format("Globed failed to reach the central server endpoint <cy>{}</c>, likely because the server is down, your internet is down, or an invalid URL was entered.\n\nError: <cy>{}</c>", versionURL, error);
                g_errMsgQueue.push(errMessage);

                return;
            }

            auto serverVersion = serverVersionRes.unwrap();
            if (!isNumeric(serverVersion)) {
                log::warn("Central server sent invalid answer: {}", serverVersion);
                g_errMsgQueue.push("The central server sent an <cr>invalid</c> response when requesting its <cy>version</c>. Please contact the server owner with the logs or use a different server.");
                return;
            }

            if (modVersion != serverVersion && !g_debug) {
                log::warn("Server version mismatch: client at {}, server at {}", modVersion, serverVersion);

                auto errMessage = fmt::format("Version mismatch! Mod's protocol version is <cy>v{}</c>, while server's protocol version is <cy>v{}</c>. To use this server, please update the mod and try connecting again.", modVersion, serverVersion);
                g_errMsgQueue.push(errMessage);

                return;
            }

            try {
                updateGameServers(serversURL);
            } catch (std::exception e) {
                log::warn("updateGameServers failed: {}", e.what());
                auto errMessage = fmt::format("Globed failed to parse server list sent by the central server. This is likely due to a misconfiguration on the server.\n\nError: <cy>{}</c>", e.what());

                g_errMsgQueue.push(errMessage);
            }

            log::info("Successfully updated game servers from the central server");
        }
    }

    namespace ui {
        void addBackground(CCNode* layer) {
            auto windowSize = CCDirector::get()->getWinSize();

            auto bg = CCSprite::create("GJ_gradientBG.png");
            auto bgSize = bg->getTextureRect().size;

            bg->setAnchorPoint({ 0.0f, 0.0f });
            bg->setScaleX((windowSize.width + 10.0f) / bgSize.width);
            bg->setScaleY((windowSize.height + 10.0f) / bgSize.height);
            bg->setPosition({ -5.0f, -5.0f });
            bg->setColor({ 0, 102, 255 });
            bg->setZOrder(-1);

            layer->addChild(bg);
        }

        void addBackButton(CCNode* parent, CCMenu* menu, SEL_MenuHandler callback) {
            auto windowSize = CCDirector::get()->getWinSize();
            auto backBtn = CCMenuItemSpriteExtra::create(
                CCSprite::createWithSpriteFrameName("GJ_arrow_01_001.png"),
                parent,
                callback
            );

            backBtn->setPosition(-windowSize.width / 2 + 25.0f, windowSize.height / 2 - 25.0f);

            menu->addChild(backBtn);
        }
    }
}