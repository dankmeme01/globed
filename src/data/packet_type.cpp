#include "packet_type.hpp"
#include "../util.hpp"

uint8_t ptToNumber(PacketType pt) {
    return globed_util::toUnderlying(pt);
}

PacketType numberToPt(uint8_t number) {
    switch (number) {
        case 100:
            return PacketType::CheckIn;
        case 101:
            return PacketType::Keepalive;
        case 102:
            return PacketType::Disconnect;
        case 103:
            return PacketType::Ping;
        case 104:
            return PacketType::PlayerIconsRequest;
        case 110:
            return PacketType::UserLevelEntry;
        case 111:
            return PacketType::UserLevelExit;
        case 112:
            return PacketType::UserLevelData;
        case 200:
            return PacketType::CheckedIn;
        case 201:
            return PacketType::KeepaliveResponse;
        case 202:
            return PacketType::ServerDisconnect;
        case 203:
            return PacketType::PingResponse;
        case 204:
            return PacketType::PlayerIconsResponse;
        case 210:
            return PacketType::LevelData;
        default:
            throw std::invalid_argument(fmt::format("Invalid number for conversion to PacketType: {}", static_cast<int>(number)));
    }
}