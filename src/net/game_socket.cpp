#include "game_socket.hpp"
#include <bitset>
#include <random>

uint8_t ptToNumber(PacketType pt) {
    return toUnderlying(pt);
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
        case 210:
            return PacketType::LevelData;
        default:
            throw std::invalid_argument(fmt::format("Invalid number for conversion to PacketType: {}", static_cast<int>(number)));
    }
}

uint8_t gmToNumber(IconGameMode gm) {
    return toUnderlying(gm);
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

// REMEMBER - bit indexes are reverse of rust, since std::bitset counts them from LSB to MSB, so right to left

void encodeSpecificIcon(const SpecificIconData &data, ByteBuffer &buffer) {
    buffer.writeF32(data.x);
    buffer.writeF32(data.y);
    buffer.writeF32(data.xRot);
    buffer.writeF32(data.yRot);
    buffer.writeU8(gmToNumber(data.gameMode));

    std::bitset<8> flags;
    if (data.isHidden) flags.set(7);
    if (data.isDashing) flags.set(6);
    if (data.isUpsideDown) flags.set(5);

    uint8_t flagByte = static_cast<uint8_t>(flags.to_ulong());
    buffer.writeU8(flagByte);
}

void encodePlayerData(const PlayerData& data, ByteBuffer& buffer) {
    encodeSpecificIcon(data.player1, buffer);
    encodeSpecificIcon(data.player2, buffer);

    std::bitset<8> flags;
    if (data.isPractice) flags.set(7);

    uint8_t flagByte = static_cast<uint8_t>(flags.to_ulong());
    buffer.writeU8(flagByte);
}

SpecificIconData decodeSpecificIcon(ByteBuffer &buffer) {
    auto x = buffer.readF32();
    auto y = buffer.readF32();
    auto xRot = buffer.readF32();
    auto yRot = buffer.readF32();
    auto gameMode = numberToGm(buffer.readU8());

    std::bitset<8> flags(buffer.readU8());

    return SpecificIconData {
        .x = x,
        .y = y,
        .xRot = xRot,
        .yRot = yRot,
        .gameMode = gameMode,
        .isHidden = flags.test(7),
        .isDashing = flags.test(6),
        .isUpsideDown = flags.test(5),
    };
}

PlayerData decodePlayerData(ByteBuffer& buffer) {
    auto player1 = decodeSpecificIcon(buffer);
    auto player2 = decodeSpecificIcon(buffer);

    std::bitset<8> flags(buffer.readU8());

    return PlayerData {
        .player1 = player1,
        .player2 = player2,
        .isPractice = flags.test(7),
    };
}

GameSocket::GameSocket(int _accountId, int _secretKey) : accountId(_accountId), secretKey(_secretKey) {}

RecvPacket GameSocket::recvPacket() {
    char lenbuf[4];
    if (receive(lenbuf, 4) != 4) {
        throw std::exception("failed to read 4 bytes");
    }

    auto len = byteswapU32(*reinterpret_cast<uint64_t*>(lenbuf));

    auto msgbuf = new char[len];
    receiveExact(msgbuf, len);

    ByteBuffer buf(msgbuf, len);
    delete[] msgbuf;
    
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
        case PacketType::PingResponse: {
            auto now = std::chrono::system_clock::now();
            auto pingId = buf.readI32();
            auto [serverId, then] = pingTimes.at(pingId);

            auto pingMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - then).count();
            pkt = PacketPingResponse { serverId, buf.readU32(), pingMs };

            pingTimes.erase(pingId);
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
        buf.writeI32(accountId);
        buf.writeI32(secretKey);
        buf.writeI32(std::get<PlayerEnterLevelData>(message).levelID);
    } else if (std::holds_alternative<PlayerLeaveLevelData>(message)) {
        buf.writeI8(ptToNumber(PacketType::UserLevelExit));
        buf.writeI32(accountId);
        buf.writeI32(secretKey);
    } else if (std::holds_alternative<PlayerDeadData>(message)) {
        geode::log::debug("player died, unhandled in {}:{}", __FILE__, __LINE__);
        return;
    } else if (std::holds_alternative<PlayerData>(message)) {
        auto data = std::get<PlayerData>(message);
        buf.writeI8(ptToNumber(PacketType::UserLevelData));
        buf.writeI32(accountId);
        buf.writeI32(secretKey);

        encodePlayerData(data, buf);
    } else {
        throw std::invalid_argument("tried to send invalid packet");
    }

    std::lock_guard lock(sendMutex);
    sendAll(reinterpret_cast<char*>(buf.getData().data()), buf.size());
}

void GameSocket::sendHeartbeat() {    
    ByteBuffer buf;
    buf.writeI8(ptToNumber(PacketType::Keepalive));
    buf.writeI32(accountId);
    buf.writeI32(secretKey);

    keepAliveTime = std::chrono::high_resolution_clock::now();

    std::lock_guard lock(sendMutex);
    sendAll(reinterpret_cast<char*>(buf.getData().data()), buf.size());
}

void GameSocket::sendCheckIn() {
    ByteBuffer buf;
    buf.writeI8(ptToNumber(PacketType::CheckIn));
    buf.writeI32(accountId);
    buf.writeI32(secretKey);

    std::lock_guard lock(sendMutex);
    sendAll(reinterpret_cast<char*>(buf.getData().data()), buf.size());
}

void GameSocket::sendDisconnect() {
    ByteBuffer buf;
    buf.writeI8(ptToNumber(PacketType::Disconnect));
    buf.writeI32(accountId);
    buf.writeI32(secretKey);

    std::lock_guard lock(sendMutex);
    sendAll(reinterpret_cast<char*>(buf.getData().data()), buf.size());
}

void GameSocket::sendPingTo(const std::string& serverId, const std::string& serverIp, unsigned short port) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<int> distrib(std::numeric_limits<int>::min(), std::numeric_limits<int>::max());

    auto num = distrib(gen);
    
    ByteBuffer buf;
    buf.writeI8(ptToNumber(PacketType::Ping));
    buf.writeI32(num);

    pingTimes.insert(std::make_pair(num, std::make_pair(serverId, std::chrono::system_clock::now())));

    std::lock_guard lock(sendMutex);
    sendAllTo(reinterpret_cast<char*>(buf.getData().data()), buf.size(), serverIp, port);
}

void GameSocket::disconnect() {
    UdpSocket::disconnect();
    established = false;
}
