// ugh, putting it before enums doesn't work
#![allow(non_upper_case_globals)]

use std::{
    collections::HashMap,
    net::SocketAddr,
    sync::Arc,
    time::{Duration, SystemTime},
};

use crate::{
    data::*,
    handlers,
    packets::{ClientPacketKind, ServerPacketKind},
};
use anyhow::{anyhow, Result};
use bytebuffer::{ByteBuffer, ByteReader};
use log::{debug, info, trace, warn};
use tokio::{net::UdpSocket, sync::RwLock, task::spawn_blocking};

type LevelData = HashMap<i32, PlayerData>;

pub struct ClientData {
    pub address: SocketAddr,
    pub secret_key: i32,
    pub level_id: i32,
    pub last_keepalive_time: SystemTime,
    pub player_data: PlayerAccountData,
}

pub const DAILY_LEVEL_ID: i32 = -2_000_000_000;
pub const WEEKLY_LEVEL_ID: i32 = -2_000_000_001;

pub struct State {
    pub levels: RwLock<HashMap<i32, RwLock<LevelData>>>,
    pub server_socket: Arc<UdpSocket>,
    pub connected_clients: RwLock<HashMap<i32, ClientData>>,
    // send_lock: Mutex<()>,
    pub max_clients: i32,
    pub tps: usize,
    pub tick_based: bool,
    pub kick_timeout: Duration,
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

    pub async fn broadcast_user_message(
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
                    data.write_u8(ServerPacketKind::TextMessageSent as u8);
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

    pub async fn broadcast_message(&'static self, message: &str, color: [u8; 3], level_id: Option<i32>) -> Result<()> {
        let clients = self.connected_clients.write().await;
        for (_, client) in clients.iter() {
            if level_id.is_none() || client.level_id == level_id.unwrap() {
                let mut data = ByteBuffer::new();
                data.write_u8(ServerPacketKind::ServerBroadcast as u8);
                data.write_string(message);
                color.iter().for_each(|c| data.write_u8(*c));

                if message.len() > 80 {
                    warn!("Message over 80 chars!");
                }

                self.send_buf_to(client.address, data.as_bytes()).await?;
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
                buf.write_u8(ServerPacketKind::ServerDisconnect as u8);
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

        let ptype = ClientPacketKind::try_from(bytebuffer.read_u8()?)?;

        // ping is special, requires no client id or secret key
        if ptype == ClientPacketKind::Ping {
            let ping_id = bytebuffer.read_i32()?;
            trace!("Got ping from {peer} with ping id {ping_id}");

            let mut buf = ByteBuffer::new();
            buf.write_u8(ServerPacketKind::PingResponse as u8);
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
            buf.write_u8(ServerPacketKind::ServerDisconnect as u8);
            buf.write_string("You have to sign into a Geometry Dash account before connecting. If you already are, please restart your game.");
            self.send_buf_to(peer, buf.as_bytes()).await?;
            return Ok(());
        }

        if ptype != ClientPacketKind::CheckIn {
            self.verify_secret_key_or_disconnect(client_id, secret_key, peer)
                .await?;
        } else {
            let mut buf = ByteBuffer::new();
            match self.add_client(client_id, peer, secret_key).await {
                Ok(_) => {
                    // add icons
                    let data = PlayerAccountData::decode(&mut bytebuffer)?;
                    if !data.is_valid() {
                        buf.write_u8(ServerPacketKind::ServerDisconnect as u8);
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

                    buf.write_u8(ServerPacketKind::CheckedIn as u8);
                    buf.write_u16(self.tps as u16);
                    self.send_to(client_id, buf.as_bytes()).await?;
                }
                Err(e) => {
                    buf.write_u8(ServerPacketKind::ServerDisconnect as u8);
                    buf.write_string(&e.to_string());
                    self.send_buf_to(peer, buf.as_bytes()).await?;
                }
            }

            return Ok(());
        }

        match ptype {
            ClientPacketKind::Keepalive => handlers::generic::on_keepalive(self, client_id).await,
            ClientPacketKind::Disconnect => handlers::generic::on_disconnect(self, client_id).await,
            ClientPacketKind::UserLevelEntry => {
                handlers::level::on_userlevelentry(self, client_id, &mut bytebuffer).await
            }
            ClientPacketKind::UserLevelExit => {
                handlers::level::on_userlevelexit(self, client_id).await
            }
            ClientPacketKind::UserLevelData | ClientPacketKind::SpectateNoData => {
                handlers::level::on_updatedata(
                    self,
                    client_id,
                    ptype == ClientPacketKind::SpectateNoData,
                    &mut bytebuffer,
                )
                .await
            }
            ClientPacketKind::PlayerAccountDataRequest => {
                handlers::level::on_playeraccountdatarequest(self, client_id, &mut bytebuffer).await
            }
            ClientPacketKind::LevelListRequest => {
                handlers::level::on_levellistrequest(self, client_id).await
            }
            ClientPacketKind::SendTextMessage => {
                handlers::level::on_sendtextmessage(self, client_id, &mut bytebuffer).await
            }

            _ => Err(anyhow!("invalid PacketType: {ptype:?}")),
        }
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
                    buf.write_u8(ServerPacketKind::LevelData as u8);
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
