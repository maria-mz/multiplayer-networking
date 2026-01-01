# multiplayer-networking

A playground project in C++ for learning about networking in multiplayer games.

## Demo 
https://github.com/user-attachments/assets/09f1a0dc-7485-4b81-8dfe-d5aca8c191d6

## Architecture

The project uses a simple client/server architecture:

- Clients connect to a server
- Each client sends state updates (e.g. position) for its local player to the server
- The server relays these updates to all other clients
- The client updates the game simulation to reflect local and remote player state
- TCP is used for reliable messages (e.g. player joins/leaves)
- TCP or UDP can be used for player state messages (configurable for comparison)

## Metrics

| Metric | Units | Description |
| ------ | ---- | ----------- |
| Ping | ms | Time for data to travel from the client to the server and back. |
| Burstiness (Coefficient of Variation) | None | Roughly measures how choppy movements are for remote players. A higher value means more variation in arrival times for state updates from the server, and therefore choppier movement. Note, this could potentially be mitigated with client-side prediction (see [Improvements](#improvements)). |


## Dependencies

- [SDL2](https://wiki.libsdl.org/SDL2/FrontPage)
- [SDL2_image](https://wiki.libsdl.org/SDL2_image/FrontPage)
- [SDL2_ttf](https://wiki.libsdl.org/SDL2_ttf/FrontPage)
- [Standalone Asio](https://think-async.com/Asio/AsioStandalone.html)

## Quick start

From the project root:
```console
cmake -S . -B build
cmake --build build
```

Run the server:
```console
./build/bin/server
```

Run a client:
```console
./build/bin/client
```

Use <kbd>w</kbd>,  <kbd>a</kbd>, <kbd>s</kbd>, <kbd>d</kbd> to move the local player.

Run the tests:
```console
./build/bin/tests
```

## A small experiment

When running locally, there was little difference between TCP and UDP for player state updates.

I used `pfctl` and `dnctl` to simulate ~200ms ping and ran two clients with both protocols:

### Demos
https://github.com/user-attachments/assets/6bc590e0-d963-4c4c-a7ac-3a56b00f6de7

https://github.com/user-attachments/assets/c69eb146-dd06-4830-ac4d-043c7c299792

Latency was similar for both protocols, but movement appeared much smoother with UDP.

## Improvements

- Remote player movement is fully dependent on server updates, which can appear choppy if multiple updates are missed. Client-side prediction could be added to estimate remote player positions and correct them when server data arrives.
- UDP doesn't guarantee in-order or reliable delivery. Tracking out-of-order and dropped packets could help guide further improvements.
- Text looks fuzzy on macOS. Addressing this with `SDL_WINDOW_ALLOW_HIGHDPI` would require decoupling world coordinates from window pixels.
