use roa::{
    preload::*,
    router::{get, Router},
    Context,
};
use tokio::{fs::File, /* io::AsyncReadExt */};
// use serde::{Serialize, Deserialize};

pub async fn version(context: &mut Context) -> roa::Result {
    let version = env!("CARGO_PKG_VERSION");
    context.write(version);
    Ok(())
}

// #[derive(Deserialize, Serialize, Debug)]
// struct ServerEntry<'a> {
//     name: &'a str,
//     id: &'a str,
//     region: &'a str,
//     address: &'a str,
// }

pub async fn servers(context: &mut Context) -> roa::Result {
    let server_path = std::env::var("GLOBED_SERVER_FILE_PATH")?;
    let file = File::open(server_path).await?;
    context.write_reader(file);

    // this way uses less bandwidth but 0.5ms more runtime (!!)
    // let mut contents = String::new();
    // file.read_to_string(&mut contents).await?;
    // let data: Vec<ServerEntry> = serde_json::from_str(&contents)?;
    // let back = serde_json::to_string(&data)?;
    // context.write(back);

    Ok(())
}

pub fn build_router() -> Router<()> {
    Router::new()
        .gate(roa::query::query_parser)
        .on("/version", get(version))
        .on("/servers", get(servers))
}
