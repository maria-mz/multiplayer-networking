# Pixelators

Pixelators is a small real-time multiplayer arena game built with C++, SDL2, and Asio.

SDL2 is used for managing the game window, keyboard input, and rendering. Asio is used to build a custom networking layer with asynchronous TCP and UDP transports for client-server communication.

## Demo

https://github.com/user-attachments/assets/811b46f2-ce25-4953-976a-2d8c2fabb852

## Features/Roadmap

- [x] Join a shared 2D multiplayer arena as a unique player
- [x] Move around the arena in real time with keyboard controls
- [x] See other connected players’ positions update live
- [x] Aim and shoot projectiles at other players
- [x] Take damage from projectile hits and respawn after being eliminated
- [x] Synchronize gameplay through a client-server networking system that uses reliable events and low-latency position updates
- [ ] Improve smoothness of remote player movement with client-side prediction
- [ ] Add scoring and round-based win conditions

## Why did I decide to make this?

* To practice modern C++
* To learn client-server networking with asynchronous TCP and UDP
* To understand how state is synchronized across multiple clients
* To learn game development fundamentals like delta-time game loops
* To practice breaking down a system with many moving parts into clear modules

## Dependencies

- [SDL2](https://wiki.libsdl.org/SDL2/FrontPage)
- [SDL2_image](https://wiki.libsdl.org/SDL2_image/FrontPage)
- [SDL2_ttf](https://wiki.libsdl.org/SDL2_ttf/FrontPage)
- [Standalone Asio](https://think-async.com/Asio/AsioStandalone.html)
- [GoogleTest](https://google.github.io/googletest/)

## Quick Start

From the project root:

```console
cmake -S . -B build
cmake --build build
```

Run the server:

```console
./build/bin/server
```

Run one or more clients in separate terminals:

```console
./build/bin/client
```

Run the tests:

```console
./build/bin/tests
```

## Controls

| Input | Action |
| ----- | ------ |
| <kbd>W</kbd><kbd>A</kbd><kbd>S</kbd><kbd>D</kbd> | Move |
| <kbd>Space</kbd> | Shoot |
| <kbd>F3</kbd> | Toggle Debug UI |
