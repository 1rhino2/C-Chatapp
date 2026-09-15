# C-Chatapp

Minimal chat over TCP sockets in C, a server and a client. Windows / Winsock,
links against `ws2_32`. Made this to get a feel for sockets.

## Build

```bash
gcc server.c -o server -lws2_32
gcc client.c -o client -lws2_32
```

Run `server` first, then point one or more `client` instances at it. There are
prebuilt exes in `WINDOWS BINARIES`.

## License

MIT
