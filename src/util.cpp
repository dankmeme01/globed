#include "util.hpp"
#include "global_data.hpp"
#include <Geode/utils/web.hpp>

namespace web = geode::utils::web;

/* Returns a scene with just this layer */
namespace globed_util {
    CCScene* sceneWithLayer(CCNode* layer) {
        auto scene = CCScene::create();
        
        layer->setPosition({0, 0});
        scene->addChild(layer);

        return scene;
    }

    void handleErrors() {
        {
            std::lock_guard<std::mutex> lock(g_errMsgMutex);
            while (!g_errMsgQueue.empty()) {
                globed_util::errorPopup(g_errMsgQueue.front());
                g_errMsgQueue.pop();
            }
        }

        std::lock_guard<std::mutex> lock(g_warnMsgMutex);
        while (!g_warnMsgQueue.empty())         {
            Notification::create(g_warnMsgQueue.front(), NotificationIcon::Warning, 3.0f)->show();
            g_warnMsgQueue.pop();
        }
    }

    namespace net {
        bool updateGameServers(const std::string& url) {
            auto serverListRes = web::fetch(url);
            if (serverListRes.isErr()) {
                auto error = serverListRes.unwrapErr();
                log::error("failed to fetch game servers: {}: {}", url, error);
                auto errMessage = fmt::format("Globed failed to fetch game servers from the central server. This is either a problem with networking on your system, or a misconfiguration of the server. If you believe this is not a problem on your side, please contact the owner of the server.\n\nError: <cy>{}</c>", error);

                std::lock_guard<std::mutex> lock(g_errMsgMutex);
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

        bool connectToServer(const std::string& id) {
            std::lock_guard<std::mutex> lock(g_gameServerMutex);
            
            auto it = std::find_if(g_gameServers.begin(), g_gameServers.end(), [id](const GameServer& element) {
                return element.id == id;
            });

            if (it != g_gameServers.end()) {
                GameServer server = *it;
                auto colonPos = server.address.find(":");
                if (colonPos != std::string::npos) {
                    auto address = server.address.substr(0, colonPos);
                    auto portString = server.address.substr(colonPos + 1);

                    unsigned short port;
                    std::istringstream portStream(portString);
                    if (!(portStream >> port)) {
                        log::error("port read failed");
                        return false;
                    }

                    if (!g_gameSocket.connect(address, port)) {
                        log::error("GameSocket::connect failed");
                        return false;
                    }

                    g_gameServerId = server.id;
                    g_gameSocket.sendCheckIn();

                    Mod::get()->setSavedValue("last-server-id", id);
                    return true;
                }
            }

            return false;
        }

        void disconnect(bool quiet) {
            {
                std::lock_guard<std::mutex> lock(g_gameServerMutex);
                g_gameSocket.sendDisconnect();
                g_gameSocket.disconnect();
                g_gameServerId = "";
            }

            Mod::get()->setSavedValue("last-server-id", std::string(""));
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