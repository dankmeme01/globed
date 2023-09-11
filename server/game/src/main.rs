use std::{env, error::Error};

use log::{info, LevelFilter, error};
use util::Logger;
use server::{ServerSettings, start_server};

mod util;
mod server;

static LOGGER: Logger = Logger;

#[tokio::main]
async fn main() -> Result<(), Box<dyn Error>> {
    log::set_logger(&LOGGER)
        .map(|()| {
            log::set_max_level(if cfg!(debug_assertions) {
                LevelFilter::Trace
            } else {
                LevelFilter::Info
            })
        })
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
