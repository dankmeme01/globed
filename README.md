# Globed

Globed is a multiplayer mod for Geometry Dash

## Central server configuration

* `GLOBED_SERVER_FILE_PATH` - must be passed, the server reads game servers from this file
* `GLOBED_ADDRESS` - default `0.0.0.0`, bind address for the HTTP server
* `GLOBED_PORT` - default `41000`, bind port for the HTTP server
* `GLOBED_MOUNT_POINT` - default `/`, mount point for the server. if set to `/globed/` it will mean all requests must be made to `http://your.server/globed/...`

The JSON file passed in `GLOBED_SERVER_FILE_PATH` should have a format like this:

```
[
    {
        "name": "Server name",
        "id": "server-id", // can be any string, preferrably a unique kebab-case string
        "region": "Europe", // can be any region, if you want you can put a specific country
        "address": "127.0.0.1:8080"
    }
]
```

## Game server configuration

* `GLOBED_GS_ADDRESS` - default `0.0.0.0`, bind address for the Globed game server
* `GLOBED_GS_PORT` - default `41001`, bind port for the Globed game server
* `GLOBED_GS_TPS` - default `30`, amount of times the server sends data to the clients