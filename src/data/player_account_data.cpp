#include "player_account_data.hpp"

void encodeAccountData(const PlayerAccountData& data, ByteBuffer& buffer) {
    buffer.writeI32(data.cube);
    buffer.writeI32(data.ship);
    buffer.writeI32(data.ball);
    buffer.writeI32(data.ufo);
    buffer.writeI32(data.wave);
    buffer.writeI32(data.robot);
    buffer.writeI32(data.spider);
    buffer.writeI32(data.color1);
    buffer.writeI32(data.color2);
    buffer.writeI32(data.deathEffect);
    buffer.writeU8(data.glow ? 1 : 0);
    buffer.writeString(data.name);
}

PlayerAccountData decodeAccountData(ByteBuffer& buffer) {
    auto cube = buffer.readI32();
    auto ship = buffer.readI32();
    auto ball = buffer.readI32();
    auto ufo = buffer.readI32();
    auto wave = buffer.readI32();
    auto robot = buffer.readI32();
    auto spider = buffer.readI32();
    auto color1 = buffer.readI32();
    auto color2 = buffer.readI32();
    auto deathEffect = buffer.readI32();
    auto glow = buffer.readU8() == 1;
    auto name = buffer.readString();

    geode::log::debug("decoded death effect: {}", deathEffect);

    return PlayerAccountData {
        .cube = cube,
        .ship = ship,
        .ball = ball,
        .ufo = ufo,
        .wave = wave,
        .robot = robot,
        .spider = spider,
        .color1 = color1,
        .color2 = color2,
        .deathEffect = deathEffect,
        .glow = glow,
        .name = name
    };
}