# Game server protocol

At its core, the game server is a single UDP socket. In the main thread, it constantly tries to receive a packet, and spawn a tokio task to handle it. Additionally, it runs two background tasks: one every 5 seconds that removes clients that haven't sent a `Keepalive` packet in over a minute, and another one that runs every 1/30th of a second (can be changed with environment variables, look in [readme](../../README.md) for more info) which I call a "tick" and will explain [later](#ticks) what it does.

All packets are handled in `State::handle_packet`, the order of operations is as follows:

1. Read the packet type (one unsigned byte)
2. If equal to `Ping`, read the ping ID (explained [later](#pings)), and send back a `PingResponse` packet with the ping ID, and amount of players connected currently.
3. Otherwise, read the client ID (basically account ID) and the secret key (explained [later](#secret-key))
4. Then, match on the packet type

## Packet types

At the beginning of packet handling, every packet except for `CheckIn` checks if the secret key matches with the stored secret key. If there's a mismatch, or the client ID doesn't exist in memory, then we send a `ServerDisconnect` with the appropriate error message.

### CheckIn

The first packet that is sent after establishing a connection. Adds the client into an internal server list, stores its address, client ID and secret key. Sends back a `CheckedIn` packet. If the client already existed in the internal server list (for example you tried connecting from 2 devices), then sends back a `ServerDisconnect` packet with an error message saying that the client already exists.

### Keepalive

Keepalives are similar to [pings](#pings), except that pings are for unconnected servers (so, updating the server list) while keepalives are for actually keeping the connection alive. They do not carry a ping ID, but still carry the player count and are also sent every 5 seconds.

### Disconnect

Just removes the client from the internal server list. That's it

### UserLevelEntry

Reads the level ID (signed 32-bit integer) and adds the user to the list of players on that level.

### UserLevelExit

Removes the user from the level they were playing on.

### UserLevelData

Reads the `PlayerData` structure (described [later](#playerdata)) and stores it in the level.

## Other things

### Secret key

Secret key is a simple signed 32-bit integer that is randomly generated every game startup, and it stays the same until you restart. It is sent with every single packet, and is made to make it harder to impersonate a player, though it's not impossible.

### Pings

Since the client has only a single thread that receives data from a socket and many that send, we can't just send a ping to a server and wait for response, that is extremely wasteful. Since I was lazy, I decided to differentiate between the pings with ping IDs instead of comparing socket addresses. Every time the client sends a ping to a server, it sends a randomly generated ping ID (signed 32-bit integer) and stores it with the appropriate server ID. Then when a ping is received, the client reads the ping ID, matches it with a server ID, and updates the variable that stores the ping of that server.

### PlayerData

`PlayerData` contains two `SpecificIconData` structs, for players 1 and 2 (2 is for dual mode) and a `practice` boolean value which indicates if the player is in practice mode.

`SpecificIconData` consists of two floats for position, two floats for rotation, an `IconGameMode` enum (which simply indicates the gamemode: cube, ball, etc.), and three boolean values: whether the plyer is invisible (hidden), is dashing, or is upside down.

### Ticks

A tick looks at every connected player, checks if they are currently in a level, and then gathers every other player on the same level and sends the `LevelData`  packet. The packet contains the amount of players (unsigned 16-bit integer), and for each player, their client ID (signed 32-bit integer) and the `PlayerData` struct.