#include "game_socket.hpp"
#include <global_data.hpp>

#include <bitset>
#include <random>

// idk if that's necessary
char GameSocket::buffer[65536];

GameSocket::GameSocket(int _accountId, int _secretKey) : accountId(_accountId), secretKey(_secretKey) {}

RecvPacket GameSocket::recvPacket() {
    auto received = receive(buffer, 65536);
    if (received <= 0) {
        geode::log::warn("received {} bytes", received);
        geode::log::warn("Last network error: {}", getLastNetErrorPretty());
        throw std::runtime_error("failed to receive()");
    }

    ByteBuffer buf(buffer, received);
    
    RecvPacket pkt;

    auto packetType = numberToPt(buf.readU8());
    switch (packetType) {
        case PacketType::CheckedIn:
            pkt = PacketCheckedIn { buf.readU16() };
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
                if (!g_debug && playerId == accountId) {
                    continue;
                }
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
        case PacketType::PlayerAccountDataResponse: {
            auto playerId = buf.readI32();
            auto playerData = decodeAccountData(buf);

            pkt = PacketAccountDataResponse {
                .playerId = playerId,
                .data = playerData
            };
            break;
        }
        case PacketType::LevelListResponse: {
            auto levelCount = buf.readU32();
            std::unordered_map<int, unsigned short> levels;
            for (int i = 0; i < levelCount; i++) {
                auto levelId = buf.readI32();
                auto playerCount = buf.readU16();
                levels.insert(std::make_pair(levelId, playerCount));
            }

            pkt = PacketLevelListResponse {
                .levels = levels
            };
            break;
        }
        case PacketType::TextMessageSent: {
            auto sender = buf.readI32();
            auto message = buf.readString();

            if (message.size() > 80) {
                message = message.substr(0, 80);
            }

            log::debug("received msg \"{}\" from {}", message, sender);

            pkt = PacketTextMessage {
                .sender = sender,
                .message = message
            };
            break;
        }
        case PacketType::ServerBroadcast: {
            auto message = buf.readString();
            auto color = ccColor3B {
                .r = buf.readU8(),
                .g = buf.readU8(),
                .b = buf.readU8(),
            };

            if (message.size() > 80) {
                message = message.substr(0, 80);
            }

            log::debug("received msg \"{}\" from server", message);

            pkt = PacketServerMessage {
                .message = message,
                .color = color
            };

            break;
        }
        default:
            throw std::runtime_error("server sent invalid packet");
    }

    return pkt;
}

void GameSocket::sendMessage(const NetworkThreadMessage& message) {
    ByteBuffer buf;

    if (std::holds_alternative<NMPlayerLevelEntry>(message)) {
        buf.writeI8(ptToNumber(PacketType::UserLevelEntry));
        writeAuth(buf);
        buf.writeI32(std::get<NMPlayerLevelEntry>(message).levelID);
    } else if (std::holds_alternative<NMPlayerLevelExit>(message)) {
        buf.writeI8(ptToNumber(PacketType::UserLevelExit));
        writeAuth(buf);
    } else if (std::holds_alternative<PlayerData>(message)) {
        auto data = std::get<PlayerData>(message);
        buf.writeI8(ptToNumber(PacketType::UserLevelData));
        writeAuth(buf);
        encodePlayerData(data, buf);
    } else if (std::holds_alternative<NMRequestPlayerAccount>(message)) {
        buf.writeI8(ptToNumber(PacketType::PlayerAccountDataRequest));
        writeAuth(buf);
        buf.writeI32(std::get<NMRequestPlayerAccount>(message).playerId);
    } else if (std::holds_alternative<NMRequestLevelList>(message)) {
        buf.writeI8(ptToNumber(PacketType::LevelListRequest));
        writeAuth(buf);
    } else if (std::holds_alternative<NMSpectatingNoData>(message)) {
        buf.writeI8(ptToNumber(PacketType::SpectateNoData));
        writeAuth(buf);
    } else if (std::holds_alternative<NMSendTextMessage>(message)) {
        buf.writeI8(ptToNumber(PacketType::SendTextMessage));
        writeAuth(buf);
        buf.writeString(std::get<NMSendTextMessage>(message).message);
    } else {
        log::debug("what packet did you try to send silly..");
        throw std::invalid_argument("tried to send invalid packet");
    }

    sendBuf(buf);
}

void GameSocket::sendHeartbeat() {    
    ByteBuffer buf;
    buf.writeI8(ptToNumber(PacketType::Keepalive));
    writeAuth(buf);

    keepAliveTime = std::chrono::high_resolution_clock::now();

    sendBuf(buf);
}

void GameSocket::sendCheckIn() {
    ByteBuffer buf;
    buf.writeI8(ptToNumber(PacketType::CheckIn));
    writeAuth(buf);

    encodeAccountData(*g_accountData.lock(), buf);

    sendBuf(buf);
}

void GameSocket::sendAccountDataRequest(int playerId) {
    ByteBuffer buf;
    buf.writeI8(ptToNumber(PacketType::PlayerAccountDataRequest));
    writeAuth(buf);
    buf.writeI32(playerId);

    sendBuf(buf);
}

void GameSocket::sendDisconnect() {
    ByteBuffer buf;
    buf.writeI8(ptToNumber(PacketType::Disconnect));
    writeAuth(buf);

    sendBuf(buf);
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

void GameSocket::sendBuf(const ByteBuffer& buf) {
    std::lock_guard lock(sendMutex);
    sendAll(reinterpret_cast<char*>(buf.getData().data()), buf.size());
}

void GameSocket::writeAuth(ByteBuffer& buf) {
    buf.writeI32(accountId);
    buf.writeI32(secretKey);
}