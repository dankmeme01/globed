#pragma once
#include "settings_remote_player.hpp"

struct GlobedGameSettings {
    bool displayProgress;
    bool newProgress;
    unsigned char playerOpacity;
    float progressScale;
    bool showSelfProgress;
    float progressOffset;
    bool progressAltColor;
    bool hideOverlayCond;
    bool chatWhitelist;

    RemotePlayerSettings rpSettings;
};