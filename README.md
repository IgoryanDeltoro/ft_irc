# ft_irc

A custom **IRC server** written from scratch in **C++98**, plus a bonus **IRC bot** client. The project implements the core of the IRC protocol (RFC 1459/2812 style) over raw TCP sockets: client registration, channels, operators, and the standard set of commands — all handled through a single-threaded, non-blocking event loop built on `poll()`.

This is a team project (42 school-style `ft_irc`), split into two areas of ownership:

| Part | Owner | Covers |
|---|---|---|
| Server core, sockets, event loop, bot | **Ihor Bondarchuk** | `Server`, `Client`, non-blocking sockets, `poll()`-based I/O multiplexing, the IRC bot (`bot/`) |
| Command parsing, protocol commands, channels | **Dmitry Zasenko** | `Parser`, `Command`, per-command handlers (`src/commands/`), `Channel`, numeric replies |

## What the project is about

`ircserv` is a minimal but standards-adjacent IRC server:

- Accepts multiple simultaneous client connections over TCP using **non-blocking sockets** and a single **`poll()`** loop — no threads, no forking.
- Requires clients to authenticate with a server password (`PASS`) and register (`NICK` + `USER`) before doing anything else.
- Supports **channels**: joining/parting, topics, invite-only mode, keys (passwords), operators, user limits, and kicking.
- Supports private messaging between users and to channels.
- Implements the standard registration and messaging handshake so it can be tested with a real IRC client (e.g. `irssi`, `WeeChat`, `HexChat`) or with `nc`.

Supported commands: `PASS`, `NICK`, `USER`, `JOIN`, `PART`, `TOPIC`, `MODE`, `INVITE`, `KICK`, `PRIVMSG`, `CAP`, `PING`, `PONG`, `AWAY`, `QUIT`, `HELP`.

The **bot** (`bot/`) is a separate executable that connects to the server like a normal client and responds to commands sent to it, useful for demoing/testing the server without a full IRC client.

## Project structure

```
ft_irc/
├── includes/            # Headers: Server, Client, Channel, Command, Parser, numeric replies, macros
├── src/
│   ├── main.cpp         # Entry point: arg validation, launches Server
│   ├── Server.cpp       # poll() event loop, accepting/reading/writing clients
│   ├── Client.cpp       # Per-connection state (nick, user, channels, buffers)
│   ├── Parser.cpp       # Raw line -> Command parsing/validation
│   ├── Command.cpp      # Command object (prefix, params, trailing text)
│   ├── Channel.cpp      # Channel state: members, modes, topic, invite list
│   ├── commands/         # One file per IRC command handler (join, kick, mode, privmsg, ...)
│   └── utils/            # Socket send/receive helpers, reply formatting, cleanup
├── bot/                 # Standalone IRC bot client (Bot.cpp/.hpp, main.cpp)
├── Makefile             # Builds ircserv
└── bot/Makefile          # Builds irc_bot
```

## How to build and run

### Build the server

```bash
make
```

This produces the `ircserv` binary.

### Run the server

```bash
./ircserv <port> <password>
```

Example:

```bash
./ircserv 6667 mypassword
```

- `<port>` must be between 1024 and 65535.
- `<password>` is required for clients to authenticate via `PASS`.

### Connect to it

With `nc`:

```bash
nc 127.0.0.1 6667
PASS mypassword
NICK alice
USER alice 0 * :Alice
JOIN #general
PRIVMSG #general :hello everyone
```

Or with a real IRC client (irssi example):

```bash
irssi -c 127.0.0.1 -p 6667 -w mypassword -n alice
```

### Build and run the bot

```bash
cd bot
make
./irc_bot <ip> <port> <password>
```

Example:

```bash
./irc_bot 127.0.0.1 6667 mypassword
```

### Cleaning up

```bash
make clean   # remove object files
make fclean  # remove object files + binary
make re      # fclean + rebuild
```
(same targets available inside `bot/`)

## Architecture / mind map

```
                              ┌─────────────────────┐
                              │        ircserv        │
                              │  (single-threaded,    │
                              │   poll()-based loop)  │
                              └──────────┬───────────┘
                                         │
              ┌───────────────────────────┼───────────────────────────┐
              │                          │                          │
      ┌───────▼────────┐        ┌────────▼────────┐        ┌────────▼────────┐
      │   Sockets/IO    │        │  Parsing/Commands │        │    Channels     │
      │  (Ihor)         │        │   (Dima)          │        │   (Dima)        │
      ├─────────────────┤        ├───────────────────┤        ├─────────────────┤
      │ Server.cpp       │        │ Parser.cpp         │        │ Channel.cpp      │
      │  - accept()      │        │  - tokenize line   │        │  - members list   │
      │  - non-blocking   │        │  - validate nick/  │        │  - topic          │
      │    fds            │        │    user/channel    │        │  - modes (i,t,k,  │
      │  - poll() loop    │        │  - build Command    │        │    o,l)            │
      │  - read/write      │        │                     │        │  - invite list     │
      │    buffering       │        │ Command.cpp         │        │                     │
      │                    │        │  - prefix/params/   │        └────────┬────────┘
      │ Client.cpp          │        │    trailing text     │                 │
      │  - per-connection    │        │                       │        used by
      │    state (nick,      │        │ src/commands/*.cpp    │◄───────┘
      │    user, buffers)    │        │  - PASS, NICK, USER    │
      │                      │        │  - JOIN, PART, TOPIC    │
      │ utils/                │        │  - MODE, INVITE, KICK   │
      │  - sender/reciver      │        │  - PRIVMSG, PING/PONG   │
      │  - reply formatting    │        │  - AWAY, QUIT, CAP,     │
      │  - close/cleanup        │        │    HELP                  │
      └──────────┬───────────┘        └───────────────────────┘
                 │
         ┌───────▼────────┐
         │   bot/ (Ihor)    │
         │  Bot.cpp/.hpp    │
         │  - connects as    │
         │    a normal client │
         │  - IP/port/pass    │
         │    validation       │
         │  - auto-replies     │
         └───────────────────┘
```

## Team

- **Ihor Bondarchuk** — initial project setup, socket handling, `poll()` event loop, server lifecycle, IRC bot.
- **Dmitry Zasenko** — command parser, protocol command implementations, channel logic, numeric replies.
