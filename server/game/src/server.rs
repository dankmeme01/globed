use std::{collections::HashMap, net::SocketAddr, time::{SystemTime, Duration}, sync::Arc};

use anyhow::{Result, anyhow};
use bytebuffer::{ByteReader, ByteBuffer};
use log::{info, debug, warn};
use tokio::{sync::RwLock, net::UdpSocket};

#[derive(Debug)]
enum PacketType {
    /* client */
    
    CheckIn = 100,
    Keepalive = 101,
    Disconnect = 102,
    /* level related */
    UserLevelEntry = 110,
    UserLevelExit = 111,
    UserLevelData = 112,

    /* server */

    CheckedIn = 200,
    KeepaliveResponse = 201, // player count
    ServerDisconnect = 202, // message (string)
    LevelData = 210,
}

impl PacketType {
    fn from_number(num: u8) -> Option<Self> {
        match num {
            100 => Some(PacketType::CheckIn),
            101 => Some(PacketType::Keepalive),
            102 => Some(PacketType::Disconnect),
            110 => Some(PacketType::UserLevelEntry),
            111 => Some(PacketType::UserLevelExit),
            112 => Some(PacketType::UserLevelData),
            200 => Some(PacketType::CheckedIn),
            201 => Some(PacketType::KeepaliveResponse),
            202 => Some(PacketType::ServerDisconnect),
            210 => Some(PacketType::LevelData),
            _ => None,
        }
    }

    fn to_number(&self) -> u8 {
        match self {
            PacketType::CheckIn => 100,
            PacketType::Keepalive => 101,
            PacketType::Disconnect => 102,
            PacketType::UserLevelEntry => 110,
            PacketType::UserLevelExit => 111,
            PacketType::UserLevelData => 112,
            PacketType::CheckedIn => 200,
            PacketType::KeepaliveResponse => 201,
            PacketType::ServerDisconnect => 202,
            PacketType::LevelData => 210,
        }
    }
}

#[derive(Default)]
pub struct PlayerData {
    pub p1_pos: (f32, f32),
    pub p1_flipped: bool,
    pub p1_dashing: bool,

    pub practice: bool,
    pub hidden: bool,
}

impl PlayerData {
    pub fn empty() -> Self {
        PlayerData {..Default::default()}
    }

    pub fn encode(&self, buf: &mut ByteBuffer) {
        buf.write_f32(self.p1_pos.0);
        buf.write_f32(self.p1_pos.1);

        buf.write_bit(self.p1_flipped);
        buf.write_bit(self.p1_dashing);

        buf.reset_bits_cursors();

        buf.write_bit(self.practice);
        buf.write_bit(self.hidden);
    }

    pub fn decode(buf: &mut ByteReader) -> Result<Self> {
        let p1x = buf.read_f32()?;
        let p1y = buf.read_f32()?;

        let flipped = buf.read_bit()?;
        let dashing = buf.read_bit()?;

        buf.reset_bits_cursors();
        
        let practice = buf.read_bit()?;
        let hidden = buf.read_bit()?;

        Ok(PlayerData {
            p1_pos: (p1x, p1y),
            p1_flipped: flipped,
            p1_dashing: dashing,

            practice,
            hidden
        })
    }
}

type LevelData = HashMap<i32, PlayerData>;
type ClientData = (SocketAddr, i32, i32, SystemTime);

pub struct State {
    levels: RwLock<HashMap<i32, RwLock<LevelData>>>,
    server_socket: Arc<UdpSocket>,
    connected_clients: RwLock<HashMap<i32, ClientData>>,
}

impl State {
    pub fn new(socket: Arc<UdpSocket>) -> Self {
        State { levels: RwLock::new(HashMap::new()), server_socket: socket, connected_clients: RwLock::new(HashMap::new()) }
    }

    pub async fn add_client(&self, client_id: i32, address: SocketAddr, secret_key: i32) -> Result<()> {
        // optimizing for edge cases is bad ig
        // let clients = self.connected_clients.read().await;
        // if clients.contains_key(&client_id) {
        //     return Err(anyhow!("Client already exists, please wait a minute before reconnecting."));
        // }

        // drop(clients);
        // let mut clients = self.connected_clients.write().await;

        let mut clients = self.connected_clients.write().await;
        if clients.contains_key(&client_id) {
            return Err(anyhow!("Client already exists, please wait a minute before reconnecting."));
        }
        clients.insert(client_id, (address, secret_key, -1i32, SystemTime::now()));

        Ok(())
    }

    pub async fn remove_client(&self, client_id: i32) {
        let mut clients = self.connected_clients.write().await;
        let client = clients.remove(&client_id);

        if client.is_some() {
            let (_, _, level_id, _) = client.unwrap();
            if level_id != -1 {
                self.remove_client_from_level(client_id, level_id).await;
            }
        }
    }

    pub async fn remove_dead_clients(&self) {
        let now = SystemTime::now();
        let connected_clients = self.connected_clients.read().await;
        let mut to_remove = Vec::new();

        for client in connected_clients.iter() {
            let elapsed = now.duration_since(client.1.3).unwrap_or_else(|_| Duration::from_secs(0));
            if elapsed > Duration::from_secs(60) {
                to_remove.push(*client.0);
            }
        }

        drop(connected_clients);

        for id in to_remove {
            self.remove_client(id).await;
        }
    }


    pub async fn add_client_to_level(&self, client_id: i32, level_id: i32) -> Result<()> {
        let mut levels = self.levels.write().await;
        levels.entry(level_id).or_insert_with(|| RwLock::new(HashMap::new()));

        let mut level = levels.get(&level_id).ok_or(anyhow!("edge case 101"))?.write().await;
        level.insert(client_id, PlayerData::empty());

        drop(level);
        drop(levels);

        let mut clients = self.connected_clients.write().await;
        if let Some(client) = clients.get_mut(&client_id) {
            client.2 = level_id;
        }

        Ok(())
    }

    pub async fn remove_client_from_level(&self, client_id: i32, level_id: i32) {
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

    pub async fn remove_empty_levels(&self) {
        let mut levels = self.levels.write().await;
        levels.retain(|_, level| {
            let level_ro = level.blocking_read();
            !level_ro.is_empty()
        });
    }


    pub async fn send_to(&self, client_id: i32, data: &[u8]) -> Result<usize> {
        let clients = self.connected_clients.read().await;
        let client = clients.get(&client_id).ok_or(anyhow!("Client not found by id {client_id}"))?;
        
        let len_buf = data.len() as u32;
        self.server_socket.send_to(&len_buf.to_be_bytes()[..], client.0).await?;
        Ok(self.server_socket.send_to(data, client.0).await?)
    }

    pub async fn verify_secret_key(&self, client_id: i32, key: i32) -> Result<()> {
        let clients = self.connected_clients.read().await;
        let client = clients.get(&client_id).ok_or(anyhow!("Client not found by id {client_id}"))?;
        if client.1 != key {
            return Err(anyhow!("Secret key does not match"));
        }

        Ok(())
    }

    pub async fn handle_packet(&self, buf: &[u8], peer: SocketAddr) -> Result<()> {
        let mut bytebuffer = ByteReader::from_bytes(buf);

        let ptype = PacketType::from_number(bytebuffer.read_u8()?).ok_or(anyhow!("invalid packet type"))?;

        let client_id = bytebuffer.read_i32()?;
        let secret_key = bytebuffer.read_i32()?;

        match ptype {
            PacketType::CheckIn => {
                debug!("{peer} sent CheckIn with account ID {client_id}");
                let mut buf = ByteBuffer::new();
                match self.add_client(client_id, peer, secret_key).await {
                    Ok(_) => {
                        buf.write_u8(PacketType::CheckedIn.to_number());
                    },
                    Err(e) => {
                        buf.write_u8(PacketType::ServerDisconnect.to_number());
                        buf.write_string(&e.to_string());
                    }
                }

                self.send_to(client_id, buf.as_bytes()).await?;
            }

            PacketType::Keepalive => {
                self.verify_secret_key(client_id, secret_key).await?;

                let mut buf = ByteBuffer::new();
                buf.write_u8(PacketType::KeepaliveResponse.to_number());


                let mut clients = self.connected_clients.write().await;
                if let Some(client) = clients.get_mut(&client_id) {
                    client.3 = SystemTime::now();
                }
                
                buf.write_u32(clients.len() as u32);
                drop(clients);

                self.send_to(client_id, buf.as_bytes()).await?;
            }

            PacketType::Disconnect => {
                self.verify_secret_key(client_id, secret_key).await?;
                self.remove_client(client_id).await;

                debug!("{peer} sent Disconnect");
            }

            PacketType::UserLevelEntry => {
                self.verify_secret_key(client_id, secret_key).await?;

                let level_id = bytebuffer.read_i32()?;
                debug!("{peer} joined level {level_id}");
                
                self.add_client_to_level(client_id, level_id).await?;
            }

            PacketType::UserLevelExit => {
                self.verify_secret_key(client_id, secret_key).await?;

                let mut clients = self.connected_clients.write().await;
                let level_id = clients.get_mut(&client_id).map(|client| {
                    let level_id = client.2;
                    client.2 = -1;
                    level_id
                }).ok_or(anyhow!("edge case 102"))?;

                self.remove_client_from_level(client_id, level_id).await;
            }

            PacketType::UserLevelData => {
                self.verify_secret_key(client_id, secret_key).await?;
                let data = PlayerData::decode(&mut bytebuffer)?;

                let clients = self.connected_clients.read().await;
                let level_id = clients.get(&client_id).map(|client| client.2).ok_or(anyhow!("edge case 103"))?;

                let levels = self.levels.read().await;
                let mut level = levels.get(&level_id).ok_or(anyhow!("edge case 104"))?.write().await;
                level.insert(client_id, data);
            }

            _ => {
                return Err(anyhow!("invalid PacketType: {ptype:?}"));
            }
        }

        Ok(())
    }

    pub async fn tick(&self) {
        /* Send LevelData to all users currently on a level */

        // this is level_id: [client_id]
        let mut level_to_players: HashMap<i32, Vec<i32>> = HashMap::new();
        let clients = self.connected_clients.read().await;

        for (id, (_, _, level_id, _)) in clients.iter() {
            level_to_players.entry(*level_id).or_insert_with(Vec::new).push(*id);
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
                    buf.write_u8(PacketType::LevelData.to_number());
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
}

pub async fn start_server(settings: ServerSettings<'_>) -> Result<()> {
    let addr = format!("{}:{}", settings.address, settings.port);
    let socket = Arc::new(UdpSocket::bind(&addr).await?);

    let state = Arc::new(State::new(socket.clone()));

    info!("Globed game server running on: {addr}");

    let mut buf = [0u8; 1024];

    let state_cloned = state.clone();
    let state_cloned2 = state.clone();
    let _handle = tokio::spawn(async move {
        let mut interval = tokio::time::interval(Duration::from_secs(5));
        interval.tick().await;
        loop {
            interval.tick().await;
            debug!("removing dead clients");
            state_cloned.remove_dead_clients().await;
        }
    });

    let _handle2 = tokio::spawn(async move {
        let mut interval = tokio::time::interval(Duration::from_secs_f32(1f32 / (settings.tps as f32)));
        interval.tick().await;
        loop {
            interval.tick().await;
            state_cloned2.tick().await;
        }
    });

    loop {
        let (len, peer) = socket.recv_from(&mut buf).await?;
        let cloned_state = state.clone();

        tokio::spawn(async move {
            if let Err(e) = cloned_state.handle_packet(&buf[..len], peer).await {
                warn!("error from {peer}: {e}");
            }
        });
    }
}