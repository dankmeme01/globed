// ugh, putting it before enums doesn't work
#![allow(non_upper_case_globals)]

use std::{
    collections::HashMap,
    net::SocketAddr,
    sync::Arc,
    time::{Duration, SystemTime},
};

use crate::data::player_data::{PlayerAccountData, PlayerData};
use anyhow::{anyhow, Result};
use bytebuffer::{ByteBuffer, ByteReader};
use log::{debug, info, trace, warn};
use num_enum::TryFromPrimitive;
use tokio::{
    net::UdpSocket,
    sync::{Mutex, RwLock},
    task::spawn_blocking,
};

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
    /* level related */
    UserLevelEntry = 110,
    UserLevelExit = 111,
    UserLevelData = 112,

    /* server */
    CheckedIn = 200,
    KeepaliveResponse = 201, // player count
    ServerDisconnect = 202,  // message (string)
    PingResponse = 203,
    PlayerAccountDataResponse = 204,
    LevelListResponse = 205,
    LevelData = 210,
}

type LevelData = HashMap<i32, PlayerData>;
type ClientData = (SocketAddr, i32, i32, SystemTime, PlayerAccountData); // address, secret key, level ID, last keepalive time, player icons

const DAILY_LEVEL_ID: i32 = -2_000_000_000;
const WEEKLY_LEVEL_ID: i32 = -2_000_000_001;

pub struct State {
    levels: RwLock<HashMap<i32, RwLock<LevelData>>>,
    server_socket: Arc<UdpSocket>,
    connected_clients: RwLock<HashMap<i32, ClientData>>,
    send_lock: Mutex<()>,
    max_clients: i32,
    tps: usize,
}

impl State {
    pub fn new(socket: Arc<UdpSocket>, settings: &ServerSettings<'_>) -> Self {
        State {
            levels: RwLock::new(HashMap::new()),
            server_socket: socket,
            send_lock: Mutex::new(()),
            connected_clients: RwLock::new(HashMap::new()),
            max_clients: settings.max_clients,
            tps: settings.tps,
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
            (
                address,
                secret_key,
                -1i32,
                SystemTime::now(),
                PlayerAccountData::empty(),
            ),
        );

        Ok(())
    }

    pub async fn remove_client(&'static self, client_id: i32) -> Option<ClientData> {
        let mut clients = self.connected_clients.write().await;
        let client = clients.remove(&client_id);

        drop(clients);

        if client.is_some() {
            let client = client.unwrap();
            if client.2 != -1 {
                // client.2 is level id
                self.remove_client_from_level(client_id, client.2).await;
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
                .duration_since(client.1 .3)
                .unwrap_or_else(|_| Duration::from_secs(70));
            if elapsed > Duration::from_secs(60) {
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
            client.2 = level_id;
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
        let client = clients
            .get(&client_id)
            .ok_or(anyhow!("Client not found by id {client_id}"))?;

        self.send_buf_to(client.0, data).await
    }

    pub async fn verify_secret_key(&'static self, client_id: i32, key: i32) -> Result<()> {
        let clients = self.connected_clients.read().await;
        let client = clients
            .get(&client_id)
            .ok_or(anyhow!("Client not found by id {client_id}"))?;
        if client.1 != key {
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
        let len_buf = buf.len() as u32;

        let _ = self.send_lock.lock().await;
        self.server_socket
            .send_to(&len_buf.to_be_bytes()[..], addr)
            .await?;
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

        match ptype {
            PacketType::CheckIn => {
                let mut buf = ByteBuffer::new();
                match self.add_client(client_id, peer, secret_key).await {
                    Ok(_) => {
                        // add icons
                        let data = PlayerAccountData::decode(&mut bytebuffer)?;
                        info!("{} ({}) signed in", data.name, client_id);
                        let mut clients = self.connected_clients.write().await;
                        clients
                            .get_mut(&client_id)
                            .ok_or(anyhow!("this should never happen, {}:{}", file!(), line!()))?
                            .4 = data;

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
                    client.3 = SystemTime::now();
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
                    info!("{} ({}) disconnected", data.4.name, client_id);
                }
            }

            PacketType::UserLevelEntry => {
                self.verify_secret_key_or_disconnect(client_id, secret_key, peer)
                    .await?;

                /* check if already in level */
                let clients = self.connected_clients.read().await;
                let (level_id, name) = clients
                    .get(&client_id)
                    .map(|client| (client.2, client.4.name.clone()))
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
                        let level_id = client.2;
                        client.2 = -1;
                        (client.4.name.clone(), level_id)
                    })
                    .ok_or(anyhow!(
                        "this happens with malformed user input, {}:{}",
                        file!(),
                        line!()
                    ))?;

                info!("{name} ({}) left level {level_id}", client_id);

                self.remove_client_from_level(client_id, level_id).await;
            }

            PacketType::UserLevelData => {
                self.verify_secret_key_or_disconnect(client_id, secret_key, peer)
                    .await?;
                let data = PlayerData::decode(&mut bytebuffer)?;

                let clients = self.connected_clients.read().await;
                let level_id = clients
                    .get(&client_id)
                    .map(|client| client.2)
                    .ok_or(anyhow!(
                        "this happens with malformed user input, {}:{}",
                        file!(),
                        line!()
                    ))?;

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
                    .4;

                let mut buf = ByteBuffer::new();
                buf.write_u8(PacketType::PlayerAccountDataResponse as u8);
                buf.write_i32(player_id);
                icons.encode(&mut buf);

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

        for (id, (_, _, level_id, _, _)) in clients.iter() {
            if *level_id != -1 {
                level_to_players
                    .entry(*level_id)
                    .or_insert_with(Vec::new)
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
                            player.2 = -1;
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
}

pub async fn start_server(settings: ServerSettings<'_>) -> Result<()> {
    let addr = format!("{}:{}", settings.address, settings.port);
    let socket = Arc::new(UdpSocket::bind(&addr).await?);

    let state: &'static State = Box::leak(Box::new(State::new(socket.clone(), &settings)));

    info!("Globed game server running on: {addr}");

    let mut buf = [0u8; 1024];

    let _handle = tokio::spawn(async {
        let mut interval = tokio::time::interval(Duration::from_secs(5));
        interval.tick().await;
        loop {
            interval.tick().await;
            state.remove_dead_clients().await;
        }
    });

    let _handle2 = tokio::spawn(async move {
        let mut interval =
            tokio::time::interval(Duration::from_secs_f32(1f32 / (settings.tps as f32)));
        interval.tick().await;
        loop {
            interval.tick().await;
            state.tick().await;
        }
    });

    loop {
        let (len, peer) = socket.recv_from(&mut buf).await?;

        tokio::spawn(async move {
            if let Err(e) = state.handle_packet(&buf[..len], peer).await {
                warn!("error from {peer}: {e}");
            }
        });
    }
}
