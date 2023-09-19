# Globed

Globed is a multiplayer mod for Geometry Dash, powered by [Geode](https://geode-sdk.org/)

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
* `GLOBED_GS_TICK_BASED` - default `false`, set `true` to send data to clients strictly `GLOBED_GS_TPS` amount per second. otherwise responds immediately to `PlayerData` packets. potentially less performance but higher client-side accuracy & latency.

## Issues & PRs

If you find any issues or potential improvements to the mod or the server, feel free to make an issue or a pull request. If you are trying to report a fatal error, please make sure to include the logs.

Additionally, if you have other requests (like if you want to host a server), feel free to contact me on discord @dank_meme01

Here is a list of known issues or something I cannot test:

* unable to compile with MSVC, only can compile with clang on Linux. This should be resolved when a Geode update removes winsock.h from includes.
* no idea if it compiles/works on Mac, but I tried to avoid platform-specific code.
* interpolation is a bit glitchy sometimes, no idea why
* PPA has weird teleportation on framerates that aren't a multiple of server's TPS, no matter if DR or interpolation.
* `DRPPAEngine` doesn't do dash rotations because I was too lazy and no one should use DR anyway.

Planned features:

* Level ending animation
* Screen which shows all levels that other players are playing
* Show where the player is even if they're off-screen (percentage)
* Show animation for robot & spider icons

If you can help with any of those, feel free to make a PR. Also, good luck deciphering over 4k lines of source code that even I sometimes cannot read myself!

## Special thanks

Thank you [Geode](https://geode-sdk.org/) and everybody in [Geode discord server](https://discord.gg/9e43WMKzhp) who helped me whenever I had issues. This is my first ever mod for Geometry Dash, and I am generally not very experienced in C++, so I've had a lot of difficulty with it.

Also, thanks a lot to Cvolton for [BetterInfo](https://github.com/Cvolton/betterinfo-geode), some classes like `GlobedListView` and `GlobedLevelsListView` are borrowed from there.