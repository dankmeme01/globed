#include "player_data.hpp"
#include <util.hpp>

#include <bitset>
#include <cstdint>

uint8_t gmToNumber(IconGameMode gm) {
    return globed_util::toUnderlying(gm);
}

IconGameMode numberToGm(uint8_t number) {
    switch (number) {
        case 0:
            return IconGameMode::CUBE;
        case 1:
            return IconGameMode::SHIP;
        case 2:
            return IconGameMode::BALL;
        case 3:
            return IconGameMode::UFO;
        case 4:
            return IconGameMode::WAVE;
        case 5:
            return IconGameMode::ROBOT;
        case 6:
            return IconGameMode::SPIDER;
        default:
            throw std::invalid_argument(fmt::format("Invalid number for conversion to IconGameMode: {}", static_cast<int>(number)));
    }
}

void encodeSpecificIcon(const SpecificIconData &data, ByteBuffer &buffer) {
    buffer.writeF32(data.x);
    buffer.writeF32(data.y);
    buffer.writeF32(data.rot);;
    buffer.writeU8(gmToNumber(data.gameMode));

    std::bitset<8> flags;
    if (data.isHidden) flags.set(7);
    if (data.isDashing) flags.set(6);
    if (data.isUpsideDown) flags.set(5);
    if (data.isMini) flags.set(4);
    if (data.isGrounded) flags.set(3);

    uint8_t flagByte = static_cast<uint8_t>(flags.to_ulong());
    buffer.writeU8(flagByte);
}

SpecificIconData decodeSpecificIcon(ByteBuffer &buffer) {
    auto x = buffer.readF32();
    auto y = buffer.readF32();
    auto rot = buffer.readF32();
    auto gameMode = numberToGm(buffer.readU8());

    std::bitset<8> flags(buffer.readU8());

    return SpecificIconData {
        .x = x,
        .y = y,
        .rot = rot,
        .gameMode = gameMode,
        .isHidden = flags.test(7),
        .isDashing = flags.test(6),
        .isUpsideDown = flags.test(5),
        .isMini = flags.test(4),
        .isGrounded = flags.test(3),
    };
}

void encodePlayerData(const PlayerData& data, ByteBuffer& buffer) {
    buffer.writeF32(data.timestamp);

    encodeSpecificIcon(data.player1, buffer);
    encodeSpecificIcon(data.player2, buffer);

    buffer.writeF32(data.camX);
    buffer.writeF32(data.camY);

    std::bitset<8> flags;
    if (data.isPractice) flags.set(7);
    if (data.isDead) flags.set(6);
    if (data.isPaused) flags.set(5);

    uint8_t flagByte = static_cast<uint8_t>(flags.to_ulong());
    buffer.writeU8(flagByte);
}

PlayerData decodePlayerData(ByteBuffer& buffer) {
    auto timestamp = buffer.readF32();

    auto player1 = decodeSpecificIcon(buffer);
    auto player2 = decodeSpecificIcon(buffer);

    float camX = buffer.readF32();
    float camY = buffer.readF32();

    std::bitset<8> flags(buffer.readU8());

    return PlayerData {
        .timestamp = timestamp,
        .player1 = player1,
        .player2 = player2,
        .camX = camX,
        .camY = camY,
        .isPractice = flags.test(7),
        .isDead = flags.test(6),
        .isPaused = flags.test(5),
    };
}