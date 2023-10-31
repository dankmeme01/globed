// ugh, putting it before enums doesn't work
#![allow(non_upper_case_globals)]

use std::{
    collections::HashMap,
    net::SocketAddr,
    sync::Arc,
    time::{Duration, SystemTime},
};

use crate::data::*;
use anyhow::{anyhow, Result};
use bytebuffer::{ByteBuffer, ByteReader};
use log::{debug, info, trace, warn};
use num_enum::TryFromPrimitive;
use tokio::{net::UdpSocket, sync::RwLock, task::spawn_blocking};

#[derive(Debug, Eq, PartialEq, TryFromPrimitive)]
#[repr(u8)]
enum PacketType {
    /* client */
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

    /* server */
    CheckedIn = 200,
    KeepaliveResponse = 201, // player count
    ServerDisconnect = 202,  // message (string)
    PingResponse = 203,
    PlayerAccountDataResponse = 204,
    LevelListResponse = 205,
    LevelData = 210,
    TextMessageSent = 211,
}

type LevelData = HashMap<i32, PlayerData>;

pub struct ClientData {
    address: SocketAddr,
    secret_key: i32,
    level_id: i32,
    last_keepalive_time: SystemTime,
    player_data: PlayerAccountData,
}

const DAILY_LEVEL_ID: i32 = -2_000_000_000;
const WEEKLY_LEVEL_ID: i32 = -2_000_000_001;

pub struct State {
    levels: RwLock<HashMap<i32, RwLock<LevelData>>>,
    server_socket: Arc<UdpSocket>,
    connected_clients: RwLock<HashMap<i32, ClientData>>,
    // send_lock: Mutex<()>,
    max_clients: i32,
    tps: usize,
    tick_based: bool,
    kick_timeout: Duration,
}

impl State {
    pub fn new(socket: Arc<UdpSocket>, settings: &ServerSettings<'_>) -> Self {
        State {
            levels: RwLock::new(HashMap::new()),
            server_socket: socket,
            // send_lock: Mutex::new(()),
            connected_clients: RwLock::new(HashMap::new()),
            max_clients: settings.max_clients,
            tps: settings.tps,
            tick_based: settings.tick_based,
            kick_timeout: Duration::from_secs(settings.kick_timeout as u64),
        }
    }

    pub async fn add_client(
        &'static self,
        client_id: i32,
        address: SocketAddr,
        secret_key: i32,
    ) -> Result<()> {
        let mut clients = self.connected_clients.write().await;
        if clients.contains_key(&client_id) {
            return Err(anyhow!(
                "Client already exists, please wait a minute before reconnecting."
            ));
        }

        if self.max_clients != 0 && (clients.len() as i32) >= self.max_clients {
            return Err(anyhow!(
                "This server is configured to handle a maximum of {} clients, and cannot take any more.", clients.len()
            ));
        }

        clients.insert(
            client_id,
            ClientData {
                address,
                secret_key,
                level_id: -1i32,
                last_keepalive_time: SystemTime::now(),
                player_data: PlayerAccountData::empty(),
            },
        );

        Ok(())
    }

    pub async fn remove_client(&'static self, client_id: i32) -> Option<ClientData> {
        let mut clients = self.connected_clients.write().await;
        let client = clients.remove(&client_id);

        drop(clients);

        if let Some(client) = client {
            if client.level_id != -1 {
                self.remove_client_from_level(client_id, client.level_id)
                    .await;
            }

            return Some(client);
        }

        None
    }

    pub async fn remove_dead_clients(&'static self) {
        let now = SystemTime::now();
        let connected_clients = self.connected_clients.read().await;
        let mut to_remove = Vec::new();

        for client in connected_clients.iter() {
            let elapsed = now
                .duration_since(client.1.last_keepalive_time)
                .unwrap_or_else(|_| self.kick_timeout + Duration::from_secs(1));
            if elapsed > self.kick_timeout {
                to_remove.push(*client.0);
            }
        }

        drop(connected_clients);

        for id in to_remove {
            debug!("attempting to remove dead client {id}");
            self.remove_client(id).await;
        }
    }

    pub async fn add_client_to_level(&'static self, client_id: i32, level_id: i32) -> Result<()> {
        let mut levels = self.levels.write().await;
        levels
            .entry(level_id)
            .or_insert_with(|| RwLock::new(HashMap::new()));

        let mut level = levels
            .get(&level_id)
            .ok_or(anyhow!("this should never happen, {}:{}", file!(), line!()))?
            .write()
            .await;
        level.insert(client_id, PlayerData::empty());

        drop(level);
        drop(levels);

        let mut clients = self.connected_clients.write().await;
        if let Some(client) = clients.get_mut(&client_id) {
            client.level_id = level_id;
        }

        Ok(())
    }

    pub async fn remove_client_from_level(&'static self, client_id: i32, level_id: i32) {
        let levels = self.levels.read().await;

        match levels.get(&level_id) {
            Some(level) => {
                let mut level = level.write().await;
                level.remove(&client_id);
            }
            None => {
                return;
            }
        }

        drop(levels);
        self.remove_empty_levels().await;
    }

    pub async fn remove_empty_levels(&'static self) {
        spawn_blocking(|| {
            let mut levels = self.levels.blocking_write();
            levels.retain(|_, level| {
                let level_ro = level.blocking_read();
                !level_ro.is_empty()
            });
        });
    }

    pub async fn send_to(&'static self, client_id: i32, data: &[u8]) -> Result<usize> {
        let clients = self.connected_clients.read().await;
        let client_addr = clients
            .get(&client_id)
            .ok_or(anyhow!("Client not found by id {client_id}"))?
            .address;

        drop(clients);

        self.send_buf_to(client_addr, data).await
    }

    pub async fn broadcast_text_message(
        &'static self,
        sender_client_id: i32,
        message: &str,
    ) -> Result<()> {
        let clients = self.connected_clients.write().await;
        if let Some(sender_level_id) = clients.get(&sender_client_id).map(|client| client.level_id)
        {
            for (_, client) in clients.iter() {
                if client.level_id == sender_level_id {
                    let mut data = ByteBuffer::new();
                    data.write_u8(PacketType::TextMessageSent as u8);
                    data.write_i32(sender_client_id);
                    let message = if message.len() > 80 {
                        &message[0..80]
                    } else {
                        message
                    };
                    data.write_string(message);

                    self.send_buf_to(client.address, data.as_bytes()).await?;
                }
            }
        }

        Ok(())
    }

    pub async fn verify_secret_key(&'static self, client_id: i32, key: i32) -> Result<()> {
        let clients = self.connected_clients.read().await;
        let client = clients
            .get(&client_id)
            .ok_or(anyhow!("Client not found by id {client_id}"))?;
        if client.secret_key != key {
            return Err(anyhow!("Secret key does not match"));
        }

        Ok(())
    }

    pub async fn verify_secret_key_or_disconnect(
        &'static self,
        client_id: i32,
        key: i32,
        peer: SocketAddr,
    ) -> Result<()> {
        // Returns Err() if verification failed so error handling can abort the packet handling
        match self.verify_secret_key(client_id, key).await {
            Ok(_) => Ok(()),
            Err(err) => {
                let mut buf = ByteBuffer::new();
                buf.write_u8(PacketType::ServerDisconnect as u8);
                buf.write_string(&format!("Failed to verify secret key: {}", err));

                self.send_buf_to(peer, buf.as_bytes()).await?;

                Err(anyhow!(
                    "disconnected client because of secret key mismatch"
                ))
            }
        }
    }

    pub async fn send_buf_to(&'static self, addr: SocketAddr, buf: &[u8]) -> Result<usize> {
        Ok(self.server_socket.send_to(buf, addr).await?)
    }

    pub async fn handle_packet(&'static self, buf: &[u8], peer: SocketAddr) -> Result<()> {
        let mut bytebuffer = ByteReader::from_bytes(buf);

        let ptype = PacketType::try_from(bytebuffer.read_u8()?)?;

        // ping is special, requires no client id or secret key
        if ptype == PacketType::Ping {
            let ping_id = bytebuffer.read_i32()?;
            trace!("Got ping from {peer} with ping id {ping_id}");

            let mut buf = ByteBuffer::new();
            buf.write_u8(PacketType::PingResponse as u8);
            buf.write_i32(ping_id);

            let clients = self.connected_clients.read().await;
            buf.write_u32(clients.len() as u32);
            drop(clients);

            self.send_buf_to(peer, buf.as_bytes()).await?;
            return Ok(());
        }

        let client_id = bytebuffer.read_i32()?;
        let secret_key = bytebuffer.read_i32()?;

        if client_id == 0 {
            let mut buf = ByteBuffer::new();
            buf.write_u8(PacketType::ServerDisconnect as u8);
            buf.write_string("You have to sign into a Geometry Dash account before connecting. If you already are, please restart your game.");
            self.send_buf_to(peer, buf.as_bytes()).await?;
            return Ok(());
        }

        match ptype {
            PacketType::CheckIn => {
                let mut buf = ByteBuffer::new();
                match self.add_client(client_id, peer, secret_key).await {
                    Ok(_) => {
                        // add icons
                        let data = PlayerAccountData::decode(&mut bytebuffer)?;
                        if !data.is_valid() {
                            buf.write_u8(PacketType::ServerDisconnect as u8);
                            buf.write_string("Invalid data was passed. Contact the developer if you think this is wrong.");
                            self.send_buf_to(peer, buf.as_bytes()).await?;
                            let mut clients = self.connected_clients.write().await;
                            clients.remove(&client_id);
                            return Ok(());
                        }
                        info!("{} ({}) signed in", data.name, client_id);
                        let mut clients = self.connected_clients.write().await;
                        clients
                            .get_mut(&client_id)
                            .ok_or(anyhow!("this should never happen, {}:{}", file!(), line!()))?
                            .player_data = data;

                        drop(clients);

                        buf.write_u8(PacketType::CheckedIn as u8);
                        buf.write_u16(self.tps as u16);
                        self.send_to(client_id, buf.as_bytes()).await?;
                    }
                    Err(e) => {
                        buf.write_u8(PacketType::ServerDisconnect as u8);
                        buf.write_string(&e.to_string());
                        self.send_buf_to(peer, buf.as_bytes()).await?;
                    }
                }
            }

            PacketType::Keepalive => {
                self.verify_secret_key_or_disconnect(client_id, secret_key, peer)
                    .await?;

                let mut buf = ByteBuffer::new();
                buf.write_u8(PacketType::KeepaliveResponse as u8);

                let mut clients = self.connected_clients.write().await;
                if let Some(client) = clients.get_mut(&client_id) {
                    client.last_keepalive_time = SystemTime::now();
                }

                buf.write_u32(clients.len() as u32);
                drop(clients);

                self.send_to(client_id, buf.as_bytes()).await?;
            }

            PacketType::Disconnect => {
                self.verify_secret_key_or_disconnect(client_id, secret_key, peer)
                    .await?;
                let data = self.remove_client(client_id).await;

                if let Some(data) = data {
                    info!("{} ({}) disconnected", data.player_data.name, client_id);
                }
            }

            PacketType::UserLevelEntry => {
                self.verify_secret_key_or_disconnect(client_id, secret_key, peer)
                    .await?;

                /* check if already in level */
                let clients = self.connected_clients.read().await;
                let (level_id, name) = clients
                    .get(&client_id)
                    .map(|client| (client.level_id, client.player_data.name.clone()))
                    .ok_or(anyhow!("this should never happen, {}:{}", file!(), line!()))?;

                drop(clients);

                if level_id != -1 {
                    self.remove_client_from_level(client_id, level_id).await;
                }

                let level_id = match bytebuffer.read_i32()? {
                    -1 => DAILY_LEVEL_ID,
                    -2 => WEEKLY_LEVEL_ID,
                    x => x,
                };

                info!("{name} ({}) joined level {level_id}", client_id);

                self.add_client_to_level(client_id, level_id).await?;
            }

            PacketType::UserLevelExit => {
                self.verify_secret_key_or_disconnect(client_id, secret_key, peer)
                    .await?;

                let mut clients = self.connected_clients.write().await;
                let (name, level_id) = clients
                    .get_mut(&client_id)
                    .map(|client| {
                        let level_id = client.level_id;
                        client.level_id = -1;
                        (client.player_data.name.clone(), level_id)
                    })
                    .ok_or(anyhow!(
                        "this happens with malformed user input, {}:{}",
                        file!(),
                        line!()
                    ))?;

                drop(clients);

                info!("{name} ({}) left level {level_id}", client_id);

                self.remove_client_from_level(client_id, level_id).await;
            }

            PacketType::UserLevelData | PacketType::SpectateNoData => {
                self.verify_secret_key_or_disconnect(client_id, secret_key, peer)
                    .await?;

                let clients = self.connected_clients.read().await;
                let level_id = clients
                    .get(&client_id)
                    .map(|client| client.level_id)
                    .ok_or(anyhow!(
                        "this happens with malformed user input, {}:{}",
                        file!(),
                        line!()
                    ))?;

                drop(clients);

                // for specating dont try to decode data
                if ptype == PacketType::UserLevelData {
                    let data = PlayerData::decode(&mut bytebuffer)?;

                    if level_id != -1 {
                        let levels = self.levels.read().await;
                        let mut level = levels
                            .get(&level_id)
                            .ok_or(anyhow!("this should never happen, {}:{}", file!(), line!()))?
                            .write()
                            .await;
                        level.insert(client_id, data);
                    }
                }

                if !self.tick_based {
                    let levels = self.levels.read().await;
                    match levels.get(&level_id) {
                        None => {
                            warn!(
                                "player on an invalid level, id: {level_id}, player {}",
                                client_id
                            );
                        }

                        Some(level) => {
                            let level = level.read().await;
                            let mut buf = ByteBuffer::new();
                            buf.write_u8(PacketType::LevelData as u8);
                            buf.write_u16(level.len() as u16);
                            for (player_id, player_data) in level.iter() {
                                buf.write_i32(*player_id);
                                player_data.encode(&mut buf);
                            }

                            self.send_to(client_id, buf.as_bytes()).await?;
                        }
                    }
                }
            }

            PacketType::PlayerAccountDataRequest => {
                self.verify_secret_key_or_disconnect(client_id, secret_key, peer)
                    .await?;

                let player_id = bytebuffer.read_i32()?;

                let clients = self.connected_clients.read().await;
                let icons = &clients
                    .get(&player_id)
                    .ok_or(anyhow!(
                        "user requested account data of non-existing player"
                    ))?
                    .player_data;

                let mut buf = ByteBuffer::new();
                buf.write_u8(PacketType::PlayerAccountDataResponse as u8);
                buf.write_i32(player_id);
                icons.encode(&mut buf);
                drop(clients);

                self.send_to(client_id, buf.as_bytes()).await?;
            }

            PacketType::LevelListRequest => {
                self.verify_secret_key_or_disconnect(client_id, secret_key, peer)
                    .await?;

                let mut buf = ByteBuffer::new();
                buf.write_u8(PacketType::LevelListResponse as u8);

                // for testing uncomment this and comment the next part
                // buf.write_u32(9u32);

                // buf.write_i32(91446932_i32);
                // buf.write_u16(8u16);
                // buf.write_i32(82510517_i32);
                // buf.write_u16(1u16);
                // buf.write_i32(69010770_i32);
                // buf.write_u16(0u16);
                // buf.write_i32(59626284_i32);
                // buf.write_u16(23u16);
                // buf.write_i32(57433866_i32);
                // buf.write_u16(57u16);
                // buf.write_i32(47611766_i32);
                // buf.write_u16(23u16);
                // buf.write_i32(45239692_i32);
                // buf.write_u16(2u16);
                // buf.write_i32(44062068_i32);
                // buf.write_u16(110u16);
                // buf.write_i32(37259527_i32);
                // buf.write_u16(3u16);

                let levels = self.levels.read().await;
                buf.write_u32(levels.len() as u32);

                for level in levels.iter() {
                    let level_id = match *level.0 {
                        DAILY_LEVEL_ID => -1,
                        WEEKLY_LEVEL_ID => -2,
                        x => x,
                    };

                    buf.write_i32(level_id);
                    buf.write_u16(level.1.read().await.len() as u16);
                }

                self.send_to(client_id, buf.as_bytes()).await?;
            }

            PacketType::SendTextMessage => {
                self.verify_secret_key_or_disconnect(client_id, secret_key, peer)
                    .await?;

                match bytebuffer.read_string() {
                    Ok(message) => {
                        let (name, level_id) = self
                            .connected_clients
                            .write()
                            .await
                            .get(&client_id)
                            .map(|client| (client.player_data.name.clone(), client.level_id))
                            .unwrap_or((String::from("?????"), -1));
                        let message = crate::util::escape_bad_words(&message);

                        info!("[{name} @ {level_id}] {message:?}");
                        self.broadcast_text_message(client_id, &message).await?
                    }
                    Err(_error) => {
                        warn!("user {client_id} sent an invalid message");
                    }
                }
            }

            _ => {
                return Err(anyhow!("invalid PacketType: {ptype:?}"));
            }
        }

        Ok(())
    }

    pub async fn tick(&'static self) {
        /* Send LevelData to all users currently on a level */

        // this is level_id: [client_id]
        let mut level_to_players: HashMap<i32, Vec<i32>> = HashMap::new();
        let clients = self.connected_clients.read().await;

        for (id, client_data) in clients.iter() {
            if client_data.level_id != -1 {
                level_to_players
                    .entry(client_data.level_id)
                    .or_default()
                    .push(*id);
            }
        }

        drop(clients);

        let levels = self.levels.read().await;
        for (level_id, players) in level_to_players {
            match levels.get(&level_id) {
                None => {
                    warn!("players on an invalid level, id: {level_id}, players: {players:?}");
                    let mut clients_rw = self.connected_clients.write().await;
                    for player in players {
                        if let Some(player) = clients_rw.get_mut(&player) {
                            player.level_id = -1;
                        }
                    }
                }
                Some(level) => {
                    let level = level.read().await;
                    let mut buf = ByteBuffer::new();
                    buf.write_u8(PacketType::LevelData as u8);
                    buf.write_u16(level.len() as u16);
                    for (player_id, player_data) in level.iter() {
                        buf.write_i32(*player_id);
                        player_data.encode(&mut buf);
                    }

                    for player_id in level.keys() {
                        let res = self.send_to(*player_id, buf.as_bytes()).await;
                        if res.is_err() {
                            warn!("tick error sending to {player_id}: {}", res.unwrap_err());
                        }
                    }
                }
            }
        }
    }
}

pub struct ServerSettings<'a> {
    pub address: &'a str,
    pub port: &'a str,
    pub tps: usize,
    pub max_clients: i32,
    pub tick_based: bool,
    pub kick_timeout: u32,
}

pub async fn start_server(settings: ServerSettings<'_>) -> Result<()> {
    let addr = format!("{}:{}", settings.address, settings.port);
    let socket = Arc::new(UdpSocket::bind(&addr).await?);

    let state: &'static State = Box::leak(Box::new(State::new(socket.clone(), &settings)));

    info!("Globed game server running on: {addr}");

    let mut buf = [0u8; 1024];

    // task that runs every 5 seconds to remove clients that havent responed in `kick_timeout` seconds
    let _handle = tokio::spawn(async {
        let mut interval = tokio::time::interval(Duration::from_secs(5));
        interval.tick().await;
        loop {
            interval.tick().await;
            state.remove_dead_clients().await;
        }
    });

    // task to send data if tick based (shouldnt be used generally)
    if settings.tick_based {
        let _handle2 = tokio::spawn(async move {
            let mut interval =
                tokio::time::interval(Duration::from_secs_f32(1f32 / (settings.tps as f32)));
            interval.tick().await;
            loop {
                interval.tick().await;
                state.tick().await;
            }
        });
        Box::leak(Box::new(_handle2));
    }

    loop {
        let (len, peer) = socket.recv_from(&mut buf).await?;

        tokio::spawn(async move {
            if let Err(e) = state.handle_packet(&buf[..len], peer).await {
                warn!("error from {peer}: {e}");
            }
        });
    }
}
