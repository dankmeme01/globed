#include "game_socket.hpp"
#include "../global_data.hpp"
#include <bitset>

uint8_t ptToNumber(PacketType pt) {
    switch (pt) {
        case PacketType::CheckIn:
            return 100;
        case PacketType::Keepalive:
            return 101;
        case PacketType::Disconnect:
            return 102;
        case PacketType::UserLevelEntry:
            return 110;
        case PacketType::UserLevelExit:
            return 111;
        case PacketType::UserLevelData:
            return 112;
        case PacketType::CheckedIn:
            return 200;
        case PacketType::KeepaliveResponse:
            return 201;
        case PacketType::ServerDisconnect:
            return 202;
        case PacketType::LevelData:
            return 210;
    }
}

PacketType numberToPt(uint8_t number) {
    switch (number) {
        case 100:
            return PacketType::CheckIn;
        case 101:
            return PacketType::Keepalive;
        case 102:
            return PacketType::Disconnect;
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
        case 210:
            return PacketType::LevelData;
        default:
            // Handle the case where an unknown number is passed
            throw std::invalid_argument("Invalid number for conversion to PacketType");
    }
}

void encodePlayerData(const PlayerData& data, ByteBuffer& buffer) {
    buffer.writeF32(data.x);
    buffer.writeF32(data.y);

    std::bitset<8> p1data;
    if (data.isUpsideDown) p1data.set(0);
    if (data.isDashing) p1data.set(1);

    uint8_t p1byte = static_cast<uint8_t>(p1data.to_ulong());
    buffer.writeU8(p1byte);

    std::bitset<8> totalData;
    if (data.isPractice) totalData.set(0);
    if (data.isHidden) totalData.set(1);

    uint8_t totalByte = static_cast<uint8_t>(totalData.to_ulong());
    buffer.writeU8(totalByte);
}

PlayerData decodePlayerData(ByteBuffer& buffer) {
    auto p1x = buffer.readF32();
    auto p1y = buffer.readF32();

    auto p1byte = buffer.readU8();
    std::bitset<8> p1data(p1byte);

    auto totalByte = buffer.readU8();
    std::bitset<8> totalData(totalByte);

    return PlayerData {
        .isPractice = totalData.test(0),
        .x = p1x,
        .y = p1y,
        .isHidden = totalData.test(1),
        .isUpsideDown = p1data.test(0),
        .isDashing = p1data.test(1),
    };
}

RecvPacket GameSocket::recvPacket() {
    char lenbuf[4];
    if (receive(lenbuf, 4) != 4) {
        throw std::exception("failed to read 4 bytes");
    }

    auto len = byteswapU32(*reinterpret_cast<uint64_t*>(lenbuf));

    auto msgbuf = new char[len];
    receiveExact(msgbuf, len);

    ByteBuffer buf(msgbuf, len);
    
    RecvPacket pkt;

    auto packetType = numberToPt(buf.readU8());
    switch (packetType) {
        case PacketType::CheckedIn:
            pkt = PacketCheckedIn {};
            break;
        case PacketType::KeepaliveResponse: {
            auto now = std::chrono::high_resolution_clock::now();
            auto pingMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - keepAliveTime).count();
            pkt = PacketKeepaliveResponse { buf.readU32(), pingMs };
            break;
        }
        case PacketType::ServerDisconnect:
            pkt = PacketServerDisconnect { buf.readString() };
            break;
        case PacketType::LevelData: {
            auto playerCount = buf.readU16();
            std::unordered_map<int, PlayerData> players;
            for (uint16_t i = 0; i < playerCount; i++) {
                auto playerId = buf.readI32();
                auto playerData = decodePlayerData(buf);
                players.insert(std::make_pair(playerId, playerData));
            }
            pkt = PacketLevelData { players };
            break;
        }
        default:
            throw std::exception("server sent invalid packet");
    }

    return pkt;
}

void GameSocket::sendMessage(const Message& message) {
    ByteBuffer buf;

    if (std::holds_alternative<PlayerEnterLevelData>(message)) {
        buf.writeI8(ptToNumber(PacketType::UserLevelEntry));
        buf.writeI32(g_accountID);
        buf.writeI32(g_secretKey);
        buf.writeI32(std::get<PlayerEnterLevelData>(message).levelID);
    } else if (std::holds_alternative<PlayerLeaveLevelData>(message)) {
        buf.writeI8(ptToNumber(PacketType::UserLevelExit));
        buf.writeI32(g_accountID);
        buf.writeI32(g_secretKey);
    } else if (std::holds_alternative<PlayerDeadData>(message)) {
        geode::log::debug("player died, unhandled in {}:{}", __FILE__, __LINE__);
        return;
    } else if (std::holds_alternative<PlayerData>(message)) {
        auto data = std::get<PlayerData>(message);
        buf.writeI8(ptToNumber(PacketType::UserLevelData));
        buf.writeI32(g_accountID);
        buf.writeI32(g_secretKey);

        encodePlayerData(data, buf);
    }

    sendAll(reinterpret_cast<char*>(buf.getData().data()), buf.size());
}

void GameSocket::sendHeartbeat() {
    ByteBuffer buf;
    buf.writeI8(ptToNumber(PacketType::Keepalive));
    buf.writeI32(g_accountID);
    buf.writeI32(g_secretKey);

    keepAliveTime = std::chrono::high_resolution_clock::now();

    sendAll(reinterpret_cast<char*>(buf.getData().data()), buf.size());
}

void GameSocket::sendCheckIn() {
    ByteBuffer buf;
    buf.writeI8(ptToNumber(PacketType::CheckIn));
    buf.writeI32(g_accountID);
    buf.writeI32(g_secretKey);

    sendAll(reinterpret_cast<char*>(buf.getData().data()), buf.size());
}

void GameSocket::sendDisconnect() {
    ByteBuffer buf;
    buf.writeI8(ptToNumber(PacketType::Disconnect));
    buf.writeI32(g_accountID);
    buf.writeI32(g_secretKey);

    sendAll(reinterpret_cast<char*>(buf.getData().data()), buf.size());
}

bool GameSocket::close() {
    disconnect();
    return UdpSocket::close();
}

void GameSocket::disconnect() {
    connected = false;
}

bool GameSocket::connect(const std::string& serverIp, unsigned short port) {
    auto val = UdpSocket::connect(serverIp, port);
    connected = val;
    
    return val;
}