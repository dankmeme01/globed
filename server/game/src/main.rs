use std::{env, error::Error};

use log::{error, info, LevelFilter};
use server::{start_server, ServerSettings};
use util::Logger;

mod server;
mod util;

static LOGGER: Logger = Logger;

#[tokio::main]
async fn main() -> Result<(), Box<dyn Error>> {
    let log_level = std::env::var("GLOBED_GS_LOG_LEVEL")
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

    let bind_addr = env::var("GLOBED_GS_ADDRESS").unwrap_or("0.0.0.0".to_string());
    let http_port = env::var("GLOBED_GS_PORT").unwrap_or("41001".to_string());
    let tps = env::var("GLOBED_GS_TPS").unwrap_or("30".to_string());
    // let req_rate = env::var("GLOBED_GS_MAX_REQUEST_RATE").unwrap_or("32".to_string());

    let settings = ServerSettings {
        address: &bind_addr,
        port: &http_port,
        tps: tps.parse::<usize>()?,
        // max_req_rate: req_rate.parse::<usize>()?,
    };

    let result = start_server(settings).await;

    if result.is_err() {
        error!("start_server returned an error");
        error!("{}", result.unwrap_err());
    }

    info!("Shutting down");

    Ok(())
}
