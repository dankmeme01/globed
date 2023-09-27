#pragma once
#include <Geode/Geode.hpp>
#include <global_data.hpp>

using namespace geode::prelude;

#define DEFAULT_CREATE(className) \
    static className* create();

#define DEFAULT_CREATE_DEF(className) \
    className* className::create() { \
        className* ret = new className(); \
        if (ret && ret->init()) { \
            ret->autorelease(); \
            return ret; \
        } \
        CC_SAFE_DELETE(ret); \
        return nullptr; \
    }

#define DEFAULT_CREATE_SINGLETON_DEF(className) \
    static className* __g_singleton_instance = nullptr; \
    className* className::create() { \
        if (__g_singleton_instance) return __g_singleton_instance; \
        className* ret = new className(); \
        if (ret && ret->init()) { \
            ret->autorelease(); \
            __g_singleton_instance = ret; \
            return ret; \
        } \
        CC_SAFE_DELETE(ret); \
        return nullptr; \
    }

#define DEFAULT_KEYDOWN \
    void keyDown(enumKeyCodes key);

#define DEFAULT_KEYDOWN_DEF(className) \
    void className::keyDown(enumKeyCodes key) { \
        CCLayer::keyDown(key); \
        globed_util::ui::handleDefaultKey(key); \
    }

#define DEFAULT_KEYBACK \
    void keyBackClicked();

#define DEFAULT_KEYBACK_DEF(className) \
    void className::keyBackClicked() { \
        globed_util::ui::navigateBack(); \
    }

#define DEFAULT_GOBACK \
    void goBack(CCObject* _sender);

#define DEFAULT_GOBACK_DEF(className) \
    void className::goBack(CCObject* _sender) { \
        globed_util::ui::navigateBack(); \
    }

namespace globed_util {
    CCScene* sceneWithLayer(CCNode* layer);
    void handleErrors();
    bool isNumeric(const std::string& str);

    inline void errorPopup(const std::string& text) {
        FLAlertLayer::create("Globed Error", text, "Ok")->show();
    }

    // https://stackoverflow.com/questions/8357240/how-to-automatically-convert-strongly-typed-enum-into-int
    template <typename E>
    constexpr typename std::underlying_type<E>::type toUnderlying(E e) noexcept {
        return static_cast<typename std::underlying_type<E>::type>(e);
    }

    template <typename K, typename V>
    std::vector<K> mapKeys(const std::unordered_map<K, V>& map) {
        std::vector<K> out;
        for (const auto &[k, _] : map) {
            out.push_back(k);
        }

        return out;
    }

    template <typename K, typename V>
    std::vector<V> mapValues(const std::unordered_map<K, V>& map) {
        std::vector<V> out;
        for (const auto &[_, v] : map) {
            out.push_back(v);
        }

        return out;
    }

    // converts IconGameMode to geode's IconType
    IconType igmToIconType(IconGameMode mode);

    namespace net {
        bool updateGameServers(const std::string& url);
        std::pair<std::string, unsigned short> splitAddress(const std::string& address);
        void testCentralServer(const std::string& modVersion, std::string url);
    }

    namespace ui {
        void addBackground(CCNode* parent);
        void addBackButton(CCNode* parent, CCMenu* menu, SEL_MenuHandler callback);

        inline void enableKeyboard(CCLayer* layer) {
            layer->setKeyboardEnabled(true);
            layer->setKeypadEnabled(true);
        }

        inline void navigateBack() {
            auto director = CCDirector::get();
            director->popSceneWithTransition(0.5f, PopTransition::kPopTransitionFade);
        }

        inline bool handleDefaultKey(enumKeyCodes key) {
            switch (key) {
                case enumKeyCodes::KEY_Escape: {
                    ui::navigateBack();
                }

                default: {
                    return false;
                }
            }

            return true;
        }
    }

    class Benchmarker {
    public:
        Benchmarker() {}
        inline void start(std::string id) {
            #ifdef GLOBED_BENCHMARK_ENABLED
            _entries[id] = std::chrono::high_resolution_clock::now();
            #endif
        }

        inline void end(std::string id) {
            #ifdef GLOBED_BENCHMARK_ENABLED
            auto taken = std::chrono::high_resolution_clock::now() - _entries[id];
            auto micros = std::chrono::duration_cast<std::chrono::microseconds>(taken).count();
            auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(taken).count();

            if (millis > 0) {
                log::info("[B] [!] {} took {}ms {}micros to run", id, millis, micros);
            } else if (micros > 100) {
                log::info("[B] {} took {}micros to run", id, micros);
            }
            #endif
        }
    private:
        #ifdef GLOBED_BENCHMARK_ENABLED
        std::unordered_map<std::string, std::chrono::high_resolution_clock::time_point> _entries;
        #endif
    };


    // timestamp and timestampMs use highres clock, should be used for benchmarking.
    inline uint64_t timestamp() {
        return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    }
    inline uint64_t timestampMs() {
        return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    }

    // absTimestamp and absTimestampMs use system clock, used for packet timestamping.
    inline uint64_t absTimestamp() {
        return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    }
    inline uint64_t absTimestampMs() {
        return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    }
}