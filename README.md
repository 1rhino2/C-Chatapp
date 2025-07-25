# Windows C Chat App

A simple terminal-based chat server and client for Windows, written in C. Supports up to 10 simultaneous clients. Messages are broadcast to all connected users except the sender.

## Features

- Simple group chat over TCP/IP
- Windows native (WinSock2)
- Multi-client support (up to 10)
- Easy to build with MinGW/gcc or g++

## Files

- `server.c` — Chat server source code
- `client.c` — Chat client source code
- `SERVER.exe` — Pre-built server binary (Windows x64)
- `CLIENT.exe` — Pre-built client binary (Windows x64)

## Building

You need MinGW-w64 or a similar Windows C/C++ compiler.

```sh
g++ server.c -o SERVER.exe -lws2_32
g++ client.c -o CLIENT.exe -lws2_32
```

Or with gcc:

```sh
gcc server.c -o SERVER.exe -lws2_32
gcc client.c -o CLIENT.exe -lws2_32
```

## Usage

### Start the Server

```sh
SERVER.exe <port>
```

Example:

```sh
SERVER.exe 12345
```

The server will print your local IP addresses and the port. Share these with your friends so they can connect.

### Start the Client

```sh
CLIENT.exe <server_ip> <port>
```

Example:

```sh
CLIENT.exe 192.168.1.50 12345
```

Type messages and press Enter to send. All connected users (except yourself) will see your messages.

## Notes

- You must run the server before connecting clients.
- The server only broadcasts messages to other clients, not back to the sender.
- Make sure your firewall allows the chosen port.
- For best results, use a port above 1024 (e.g., 12345).

## License

MIT License. See `LICENSE` file.

---

## Binaries

Pre-built `SERVER.exe` and `CLIENT.exe` are included in this repository for convenience. If you want to rebuild them, see the instructions above.

**Important:** You must run `SERVER.exe` and `CLIENT.exe` from a terminal (such as Command Prompt or PowerShell). Do not double-click the .exe files, as they require command-line arguments and interactive input.
