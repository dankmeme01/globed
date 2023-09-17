#pragma once
#include "../net/bytebuffer.hpp"

struct PlayerIconsData {
    int cube, ship, ball, ufo, wave, robot, spider, color1, color2;
};

void encodeIconData(const PlayerIconsData& data, ByteBuffer& buffer);
PlayerIconsData decodeIconData(ByteBuffer& buffer);