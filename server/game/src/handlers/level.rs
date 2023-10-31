use anyhow::{anyhow, Result};
use bytebuffer::{ByteBuffer, ByteReader};
use log::{info, warn};

use crate::{
    data::PlayerData,
    packets::ServerPacketKind,
    server::{State, DAILY_LEVEL_ID, WEEKLY_LEVEL_ID},
};

pub async fn on_userlevelentry(
    state: &'static State,
    client_id: i32,
    bbuf: &mut ByteReader<'_>,
) -> Result<()> {
    /* check if already in level */
    let clients = state.connected_clients.read().await;
    let (level_id, name) = clients
        .get(&client_id)
        .map(|client| (client.level_id, client.player_data.name.clone()))
        .ok_or(anyhow!("this should never happen, {}:{}", file!(), line!()))?;

    drop(clients);

    if level_id != -1 {
        state.remove_client_from_level(client_id, level_id).await;
    }

    let level_id = match bbuf.read_i32()? {
        -1 => DAILY_LEVEL_ID,
        -2 => WEEKLY_LEVEL_ID,
        x => x,
    };

    info!("{name} ({}) joined level {level_id}", client_id);
    state.broadcast_message(&format!("{name} joined"), [30, 200, 30], Some(level_id)).await?;

    state.add_client_to_level(client_id, level_id).await?;

    Ok(())
}

pub async fn on_userlevelexit(state: &'static State, client_id: i32) -> Result<()> {
    let mut clients = state.connected_clients.write().await;
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
    state.broadcast_message(&format!("{name} left"), [200, 30, 30], Some(level_id)).await?;

    state.remove_client_from_level(client_id, level_id).await;

    Ok(())
}

pub async fn on_updatedata(
    state: &'static State,
    client_id: i32,
    spectating: bool,
    bbuf: &mut ByteReader<'_>,
) -> Result<()> {
    let clients = state.connected_clients.read().await;
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
    if !spectating {
        let data = PlayerData::decode(bbuf)?;

        if level_id != -1 {
            let levels = state.levels.read().await;
            let mut level = levels
                .get(&level_id)
                .ok_or(anyhow!("this should never happen, {}:{}", file!(), line!()))?
                .write()
                .await;
            level.insert(client_id, data);
        }
    }

    if !state.tick_based {
        let levels = state.levels.read().await;
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
                buf.write_u8(ServerPacketKind::LevelData as u8);
                buf.write_u16(level.len() as u16);
                for (player_id, player_data) in level.iter() {
                    buf.write_i32(*player_id);
                    player_data.encode(&mut buf);
                }

                state.send_to(client_id, buf.as_bytes()).await?;
            }
        }
    }

    Ok(())
}

pub async fn on_levellistrequest(state: &'static State, client_id: i32) -> Result<()> {
    let mut buf = ByteBuffer::new();
    buf.write_u8(ServerPacketKind::LevelListResponse as u8);

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

    let levels = state.levels.read().await;
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

    state.send_to(client_id, buf.as_bytes()).await?;

    Ok(())
}

pub async fn on_playeraccountdatarequest(
    state: &'static State,
    client_id: i32,
    bbuf: &mut ByteReader<'_>,
) -> Result<()> {
    let player_id = bbuf.read_i32()?;

    let clients = state.connected_clients.read().await;
    let icons = &clients
        .get(&player_id)
        .ok_or(anyhow!(
            "user requested account data of non-existing player"
        ))?
        .player_data;

    let mut buf = ByteBuffer::new();
    buf.write_u8(ServerPacketKind::PlayerAccountDataResponse as u8);
    buf.write_i32(player_id);
    icons.encode(&mut buf);
    drop(clients);

    state.send_to(client_id, buf.as_bytes()).await?;

    Ok(())
}

pub async fn on_sendtextmessage(
    state: &'static State,
    client_id: i32,
    bbuf: &mut ByteReader<'_>,
) -> Result<()> {
    match bbuf.read_string() {
        Ok(message) => {
            let (name, level_id) = state
                .connected_clients
                .write()
                .await
                .get(&client_id)
                .map(|client| (client.player_data.name.clone(), client.level_id))
                .unwrap_or((String::from("?????"), -1));
            info!("[{name} @ {level_id}] {message:?}");
            state.broadcast_user_message(client_id, &message).await?;
        }
        Err(_error) => {
            warn!("user {client_id} sent an invalid message");
        }
    }

    Ok(())
}
