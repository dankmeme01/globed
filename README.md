# Globed

Globed is an open-source, highly customizable multiplayer mod for Geometry Dash, powered by [Geode](https://geode-sdk.org/).

## Architecture

The server consists of two parts: a single central server and many game servers. The central server has two endpoints: `/version` (which simply returns the version in Cargo.toml), and `/servers` (which sends a JSON file with a list of all game servers, [see below](#central-server-configuration)).

Upon connection, the client tries to send a request to `/version` and compare if the versions are compatible. Then it fetches all the game servers and sends pings to them every 5 seconds, to show three things: whether the server is online, how many players it has, and what the latency is to that server.

The player can pick the server they want and press "Join", which would initiate the connection with the game server (which is where the magic happens). If you want to learn more about the game server without reading the code, you can peek into [this document](server/game/protocol.md).

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

* unable to compile with MSVC, only can compile with clang on Linux. This should be resolved when a Geode update removes winsock.h from includes.
* Mac is supported but I don't do Mac testing since I don't own one.
* death effects do not work on Android
* auto music sync when spectating does not work on Mac
* spectating does not work on Android

## Special thanks

Thanks to [ca7x3](https://twitter.com/ca7x3) for making the logo for the mod <3 (and for helping me test some things out)

Thank you [Geode](https://geode-sdk.org/) and everybody in [Geode discord server](https://discord.gg/9e43WMKzhp) who helped me whenever I had issues. This is my first ever mod for Geometry Dash, and I am generally not very experienced in C++, so I've had a lot of difficulty with it.

Also, thanks a lot to Cvolton for [BetterInfo](https://github.com/Cvolton/betterinfo-geode), some classes like `GlobedListView` and `GlobedLevelsListView` are borrowed from there.