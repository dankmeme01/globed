# Globed for 2.1

Globed is an open-source, highly customizable multiplayer mod for Geometry Dash, powered by [Geode](https://geode-sdk.org/).

**NOTE**: this repository is now archived and will not receive any updates. The mod has been rewritten from scratch and all of the development has moved to the [new repository](https://github.com/dankmeme01/globed2), and Geometry Dash 2.2 and all future versions will only be supported by the rewrite.

If you are curious why, simply put: I don't like the state of the codebase anymore. It gets harder to maintain as time goes and the only way out is to completely redesign **eveything**. So that's kinda what I did :p.

Expect significantly better performance, cleaner and more maintainable code, easy to self-host servers, smoother player interpolation, and a lot more quality of life features (including voice chat!) when the rewrite will be publicly released.

## Architecture

The server consists of two parts: a single central server and any amount of game servers. The central server has two endpoints: `/version` (returns protocol version), and `/servers` (which sends a JSON file with a list of all game servers, [see below](#central-server-configuration)).

Upon connection, the client tries to send a request to `/version` and compare if the versions are compatible. Then it fetches all the game servers and pings them to know whether the server is online, how many players it has, and what the latency is to that server.

The player can pick the server they want and press "Join", which would initiate the connection with the game server (which is where the magic happens). If you want to learn more about the game server without reading the code, you can peek into [this document](server/game/protocol.md).

## Self-hosting

If you want to self-host a server, you need to install a [Rust toolchain](https://rustup.rs/) and compile the server yourself. If you already have Rust, all you have to do is change your terminal's active directory to the appropriate server's directory (`server/central` or `server/game`) and run

```
cargo run --release
```

## Central server configuration

* `GLOBED_SERVER_FILE_PATH` - must be passed, the server reads game servers from this file. see the format below.
* `GLOBED_ADDRESS` - default `0.0.0.0`, bind address for the HTTP server
* `GLOBED_PORT` - default `41000`, bind port for the HTTP server
* `GLOBED_MOUNT_POINT` - default `/`, mount point for the server. if set to `/globed/` it will mean all requests must be made to `http://your.server/globed/...`
* `GLOBED_LOG_LEVEL` - default `trace` in debug builds, `info` in release builds. Can be one of: `off`, `trace`, `debug`, `info`, `warn`, `error`. Indicates the minimum log priority, anything less important than the specified level will not be logged. For logs made by any other crate than the server itself, the minimum log level is always `warn` (unless you set this environment variable to `error` or `off`, that will impact those logs too)

The JSON file passed in `GLOBED_SERVER_FILE_PATH` should have a format like this:

```json5
[
    {
        "name": "Server name",
        "id": "server-id", // can be any string, preferrably a unique kebab-case string
        "region": "Europe", // can be any string, if you want you can put a specific country
        "address": "127.0.0.1:8080" // port is assumed 41001 on the client if unspecified
    },
    // ...
]
```

## Game server configuration

* `GLOBED_GS_ADDRESS` - default `0.0.0.0`, bind address for the Globed game server
* `GLOBED_GS_PORT` - default `41001`, bind port for the Globed game server
* `GLOBED_GS_TPS` - default `30`, dictates the server tickrate
* `GLOBED_GS_LOG_LEVEL` - same as in central server
* `GLOBED_GS_MAX_CLIENTS` - default `0`, indicates maximum amount of clients connected at once. 0 means infinite
* `GLOBED_GS_TICK_BASED` - default `false`, set `true` to send data to clients strictly `GLOBED_GS_TPS` amount per second. otherwise responds immediately to `PlayerData` packets. when disabled, you will potentially have worse server-side performance but higher client-side accuracy & lower latency.
* `GLOBED_GS_KICK_TIMEOUT` - default `60`, amount of seconds of inactivity before kicking a client

## Issues

Here is a list of known issues or something I cannot test:

* Mod does not compile with MSVC on windows due to winsock issues
* Mac is supported but I don't do Mac testing since I don't own one, so it's not guaranteed to be stable.
* Auto music sync when spectating does not work on Mac

## Special thanks

[ca7x3](https://twitter.com/ca7x3) - thank you for making the logo for the mod and helping me with beta testing <3

maki - wrote most of the code for the chat

Thank you [Geode](https://geode-sdk.org/) and everybody in [Geode discord server](https://discord.gg/9e43WMKzhp) for helping me with my issues. I decided to write a massive project as my first ever mod so it was not easy :)

Also, thanks a lot to Cvolton for [BetterInfo](https://github.com/Cvolton/betterinfo-geode), some of the UI code was heavily inspired from there and is something I wouldn't figure out on my own.
