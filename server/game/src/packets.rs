use num_enum::TryFromPrimitive;

#[derive(Debug, Eq, PartialEq, TryFromPrimitive)]
#[repr(u8)]
pub enum ClientPacketKind {
    CheckIn = 100,
    Keepalive = 101,
    Disconnect = 102,
    Ping = 103,
    PlayerAccountDataRequest = 104,
    LevelListRequest = 105,
    SendTextMessage = 106,

    /* level related */
    UserLevelEntry = 110,
    UserLevelExit = 111,
    UserLevelData = 112,
    SpectateNoData = 113,
}

#[derive(Debug, Eq, PartialEq, TryFromPrimitive)]
#[repr(u8)]
pub enum ServerPacketKind {
    CheckedIn = 200,
    KeepaliveResponse = 201, // player count
    ServerDisconnect = 202,  // message (string)
    PingResponse = 203,
    PlayerAccountDataResponse = 204,
    LevelListResponse = 205,
    LevelData = 210,
    TextMessageSent = 211,
}
