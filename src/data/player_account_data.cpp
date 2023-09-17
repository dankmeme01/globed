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
    buffer.writeString(data.name);
}

PlayerAccountData decodeAccountData(ByteBuffer& buffer) {
    return PlayerAccountData {
        .cube = buffer.readI32(),
        .ship = buffer.readI32(),
        .ball = buffer.readI32(),
        .ufo = buffer.readI32(),
        .wave = buffer.readI32(),
        .robot = buffer.readI32(),
        .spider = buffer.readI32(),
        .color1 = buffer.readI32(),
        .color2 = buffer.readI32(),
        .name = buffer.readString(),
    };
}