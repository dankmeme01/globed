use std::time::SystemTime;

use anyhow::Result;
use bytebuffer::ByteBuffer;
use log::info;

use crate::{packets::ServerPacketKind, server::State};

pub async fn on_keepalive(state: &'static State, client_id: i32) -> Result<()> {
    let mut buf = ByteBuffer::new();
    buf.write_u8(ServerPacketKind::KeepaliveResponse as u8);

    let mut clients = state.connected_clients.write().await;
    if let Some(client) = clients.get_mut(&client_id) {
        client.last_keepalive_time = SystemTime::now();
    }

    buf.write_u32(clients.len() as u32);
    drop(clients);

    state.send_to(client_id, buf.as_bytes()).await?;

    Ok(())
}

pub async fn on_disconnect(state: &'static State, client_id: i32) -> Result<()> {
    let data = state.remove_client(client_id).await;

    if let Some(data) = data {
        info!("{} ({}) disconnected", data.player_data.name, client_id);
    }

    Ok(())
}
