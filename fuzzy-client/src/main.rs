use std::{
    net::{Ipv4Addr, UdpSocket},
    sync::{
        atomic::{AtomicBool, AtomicI32, AtomicU16, Ordering},
        Mutex,
    },
    time::Duration,
};

use anyhow::{anyhow, Result};
use bytebuffer::{ByteBuffer, ByteReader};
use log::{debug, info, warn, LevelFilter};
use player_data::PlayerData;
use rand::{rngs::OsRng, Rng};
use util::Logger;

mod player_data;
mod util;

static LOGGER: Logger = Logger;

struct State {
    pub level_id: AtomicI32,
    pub socket: UdpSocket,
    checked_in: AtomicBool,
    tps: AtomicU16,
    joined_level: AtomicBool,
    disconnect: AtomicBool,
    data: Mutex<PlayerData>,
}

impl State {
    pub fn new(level_id: i32, socket: UdpSocket) -> Self {
        Self {
            level_id: AtomicI32::new(level_id),
            socket,
            checked_in: AtomicBool::new(false),
            tps: AtomicU16::new(1),
            joined_level: AtomicBool::new(false),
            disconnect: AtomicBool::new(false),
            data: Mutex::new(PlayerData::empty()),
        }
    }
    pub fn t_main(&'static self) {
        let mut buf = ByteBuffer::new();
        buf.write_u8(100);
        buf.write_i32(999999999);
        buf.write_i32(999999999);
        for _ in 0..9 {
            buf.write_i32(0);
        }
        buf.write_string("fuzzy-bot");
        self.socket.send(buf.as_bytes()).unwrap();
        loop {
            if self.disconnect.load(Ordering::Relaxed) {
                break;
            };

            if !self.checked_in.load(Ordering::Relaxed) {
                std::thread::sleep(Duration::from_millis(250));
                continue;
            }

            if !self.joined_level.load(Ordering::Relaxed) {
                let mut buf = ByteBuffer::new();
                buf.write_u8(110);
                buf.write_i32(999999999);
                buf.write_i32(999999999);
                buf.write_i32(self.level_id.load(Ordering::Relaxed));
                self.socket.send(buf.as_bytes()).unwrap();
                self.joined_level.store(true, Ordering::Relaxed);
                continue;
            }

            // packet loss
            let plval = OsRng.gen_range(0..100);
            if plval < 1 {
                debug!("losing a packet");
                let delay: u64 = 1_000_000u64 / (self.tps.load(Ordering::Relaxed) as u64);
                std::thread::sleep(Duration::from_micros(delay));
                continue;
            }

            let data = self.data.lock().unwrap();
            let mut buf = ByteBuffer::new();
            buf.write_u8(112);
            buf.write_i32(999999999);
            buf.write_i32(999999999);
            data.encode(&mut buf);
            drop(data);
            if let Err(x) = self.socket.send(buf.as_bytes()) {
                warn!("send level data err: {x}");
            }

            let delay: u64 = 1_000_000u64 / (self.tps.load(Ordering::Relaxed) as u64);
            std::thread::sleep(Duration::from_micros(delay));
        }
    }
    pub fn t_keepalive(&'static self) {
        let mut buf = ByteBuffer::new();
        buf.write_u8(101);
        buf.write_i32(999999999);
        buf.write_i32(999999999);
        loop {
            if self.disconnect.load(Ordering::Relaxed) {
                break;
            };

            if !self.checked_in.load(Ordering::Relaxed) {
                std::thread::sleep(Duration::from_secs(1));
                continue;
            }

            let r = self.socket.send(buf.as_bytes());
            if r.is_err() {
                warn!("keepalive err: {}", r.unwrap_err());
            }

            std::thread::sleep(Duration::from_secs(5));
        }
    }
    pub fn t_recv(&'static self) {
        let mut buf = [0u8; 65536];
        loop {
            if self.disconnect.load(Ordering::Relaxed) {
                break;
            };

            let received = match self.socket.recv(&mut buf) {
                Err(e) => {
                    warn!("recv error: {e}");
                    continue;
                }
                Ok(x) => x,
            };
            let mut buf = ByteReader::from_bytes(&buf[..received]);
            if let Err(e) = self.handle_packet(&mut buf) {
                warn!("handlepacket error: {e}");
            }
        }
    }
    pub fn handle_packet(&'static self, bytereader: &mut ByteReader) -> Result<()> {
        let pt = bytereader.read_u8()?;
        match pt {
            200 => {
                // checked in
                self.tps.store(bytereader.read_u16()?, Ordering::Relaxed);
                self.checked_in.store(true, Ordering::Relaxed);
            }
            201 => {} // keepalive
            202 => {
                // server disconnect
                let reason = bytereader.read_string()?;
                info!("server disconnect: {reason}");
                self.disconnect.store(true, Ordering::Relaxed);
            }
            210 => {
                let count = bytereader.read_u16()?;
                for _ in 0..count {
                    let client_id = bytereader.read_i32()?;
                    let pdata = PlayerData::decode(bytereader)?;
                    if client_id == 999999999 {
                        continue;
                    }
                    let mut data = self.data.lock().unwrap();
                    *data = pdata;
                    break;
                }
            }

            _ => {
                info!("unhandled packet: {pt}");
            }
        }
        Ok(())
    }
    pub fn run(&'static self) {
        let h1 = std::thread::spawn(|| {
            self.t_recv();
        });

        let h2 = std::thread::spawn(|| {
            self.t_keepalive();
        });

        let h3 = std::thread::spawn(|| {
            self.t_main();
        });

        h1.join().unwrap();
        h2.join().unwrap();
        h3.join().unwrap();
    }
}

fn main() -> Result<()> {
    let log_level = std::env::var("LOG_LEVEL")
        .ok()
        .and_then(|lvl| match lvl.trim().to_lowercase().as_str() {
            "off" => Some(LevelFilter::Off),
            "trace" => Some(LevelFilter::Trace),
            "debug" => Some(LevelFilter::Debug),
            "info" => Some(LevelFilter::Info),
            "warn" => Some(LevelFilter::Warn),
            "error" => Some(LevelFilter::Error),
            _ => None,
        })
        .unwrap_or(if cfg!(debug_assertions) {
            LevelFilter::Trace
        } else {
            LevelFilter::Info
        });

    log::set_logger(&LOGGER)
        .map(|()| log::set_max_level(log_level))
        .unwrap();

    let addr = std::env::var("ADDRESS")?;
    let socket = UdpSocket::bind((Ipv4Addr::UNSPECIFIED, 0)).unwrap();
    socket.connect(addr)?;

    let level_id = std::env::var("LEVEL_ID")?.parse::<i32>()?;

    let state = Box::leak(Box::new(State::new(level_id, socket)));

    state.run();

    Ok(())
}
