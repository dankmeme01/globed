use std::{env, error::Error};

use log::{info, LevelFilter};
use roa::{tcp::Listener, App};
use util::Logger;

mod util;
mod web;

static LOGGER: Logger = Logger;

#[tokio::main]
async fn main() -> Result<(), Box<dyn Error>> {
    let log_level = std::env::var("GLOBED_LOG_LEVEL")
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

    let bind_addr = env::var("GLOBED_ADDRESS").unwrap_or("0.0.0.0".to_string());

    let http_port = env::var("GLOBED_PORT").unwrap_or("41000".to_string());
    let mount_point = env::var("GLOBED_MOUNT_POINT").unwrap_or("/".to_string());
    let mount_point = Box::leak(Box::new(mount_point));

    let gdm_router = web::build_router();
    let routes = gdm_router.routes(mount_point)?;

    let app = App::new().end(routes);

    app.listen(format!("{bind_addr}:{http_port}"), |addr| {
        info!("Central server listening on: {addr}");
    })?
    .await?;

    Ok(())
}
