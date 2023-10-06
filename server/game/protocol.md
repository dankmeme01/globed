# Game server protocol

At its core, the game server is a single UDP socket. In the main thread, it constantly tries to receive a packet, and spawn a tokio task to handle it. Additionally, it runs another background task every 5 seconds that removes clients that haven't sent a `Keepalive` packet in over a minute (by default, can be changed).

All packets are handled in `State::handle_packet`, the order of operations is as follows:

1. Read the packet type (one unsigned byte)
2. If equal to `Ping`, read the ping ID (explained [later](#pings)), and send back a `PingResponse` packet with the ping ID, and amount of players connected currently.
3. Otherwise, read the client ID (basically account ID) and the secret key (explained [later](#secret-key))
4. Then, match on the packet type

## Packet types

At the beginning of packet handling, every packet except for `CheckIn` checks if the secret key matches with the stored secret key. If there's a mismatch, or the client ID doesn't exist in memory, then we send a `ServerDisconnect` with the appropriate error message.

### CheckIn

The first packet that is sent after establishing a connection. Adds the client into an internal client list, stores its address, client ID and secret key. Sends back a `CheckedIn` packet. If the client already existed in the internal server list (for example you tried connecting from 2 devices), then sends back a `ServerDisconnect` packet with an error message saying that the client already exists. Additionally, if the `GLOBED_GS_MAX_CLIENTS` variable is set, `CheckIn` will also return a `ServerDisconnect` with an appropriate message when the server is full.

Since protocol v2: this also includes a `PlayerAccountData` [structure](#playeraccountdata). 

### Keepalive

Keepalives are similar to [pings](#pings), except that pings are for unconnected servers (so, updating the server list) while keepalives are for actually keeping the connection alive. They do not carry a ping ID, but still carry the player count and are also sent every 5 seconds.

### Disconnect

Just removes the client from the internal client list. That's it

### PlayerAccountDataRequest

One signed 32-bit integer for player ID. Returns a `PlayerAccountDataResponse` with the same player ID and the `PlayerAccountData` [structure](#playeraccountdata)

### LevelListRequest

No data. Returns a `LevelListResponse` with one unsigned 32-bit integer for the amount of levels, and then for each level, a signed 32-bit integer for the level ID and an unsigned 16-bit integer for player count.

### UserLevelEntry

Reads the level ID (signed 32-bit integer) and adds the user to the list of players on that level.

### UserLevelExit

Removes the user from the level they were playing on.

### UserLevelData

Reads the `PlayerData` structure (described [later](#playerdata)) and stores it in the level.

### SpectateNoData

No data, but the server still returns `LevelData`. Used for spectating.

## Other things

### Secret key

Secret key is a simple signed 32-bit integer that is randomly generated every game startup, and it stays the same until you restart. It is sent with every single packet, and is made to make it harder to impersonate a player, though it's not impossible if they connect before you.

### Pings

Since the client has only a single thread that receives data from a socket and many that send, we can't just send a ping to a server and wait for response, that is extremely wasteful. Since I was lazy, I decided to differentiate between the pings with ping IDs instead of comparing socket addresses. Every time the client sends a ping to a server, it sends a randomly generated ping ID (signed 32-bit integer) and stores it with the appropriate server ID. Then when a ping is received, the client reads the ping ID, matches it with a server ID, and updates the variable that stores the ping of that server.

### PlayerData

`PlayerData` contains two `SpecificIconData` structs, for players 1 and 2 (2 is for dual mode), two floats for camera position (used for spectating) and two flags: practice mode, and whether the player is dead (they are stored in a single byte)

`SpecificIconData` consists of two floats for position, a float for rotation, an `IconGameMode` enum (which simply indicates the gamemode: cube, ball, etc.), and one byte for the following boolean values (MSB to LSB):

1. whether the player is invisible
2. whether the player is dashing
3. whether the player is upside down
4. whether the player is mini
5. whether the player is on the ground

### PlayerAccountData 

`PlayerAccountData` consists of the follwing values:

* Seven signed 32-bit integers, for icon ID of each game mode (cube, ship, etc.)
* Two signed 32-bit integers for the primary and secondary color IDs
* One signed 32-bit integer for the death effect ID
* A boolean value for the glow
* A string with player's name