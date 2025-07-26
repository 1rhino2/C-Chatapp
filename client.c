// Build: gcc client.c -o client -lws2_32
// Usage: client.exe <server_ip> <port>
// You connect to the server and type messages. Other people see what you type, we all know you have no friends and your gonna test it on your terminal.
// and you see what they type. 
// If you run into trouble, check your firewall.

#define _WIN32_WINNT 0x0601
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>
#include <process.h>

#define BUFFER_SIZE 2048

// This function runs in a background thread and just waits for messages from the server.
unsigned __stdcall receive_thread(void *arg)
{
    SOCKET sock = *(SOCKET *)arg;
    char buffer[BUFFER_SIZE];
    int bytes;
    while ((bytes = recv(sock, buffer, sizeof(buffer) - 1, 0)) > 0)
    {
        buffer[bytes] = '\0';
        printf("%s", buffer); // Print whatever came in
        fflush(stdout);
    }
    printf("Disconnected from server.\n");
    exit(0);
    return 0;
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("Usage: %s <server_ip> <port>\n", argv[0]);
        printf("Example: %s 127.0.0.1 12345\n", argv[0]);
        return 1;
    }

    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
    {
        printf("WSAStartup failed. Error: %d\n", WSAGetLastError());
        return 1;
    }

    char *server_ip = argv[1];
    int port = atoi(argv[2]);

    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET)
    {
        printf("Couldn't create socket. Error: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    struct sockaddr_in server_addr = {0};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    // Convert the server IP (like "192.168.1.50") to a number (Windows: use inet_addr)
    server_addr.sin_addr.s_addr = inet_addr(server_ip);
    if (server_addr.sin_addr.s_addr == INADDR_NONE)
    {
        printf("Invalid IP address.\n");
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    // Try to connect to the server
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        printf("Connect failed. Error: %d\n", WSAGetLastError());
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    printf("Connected to chat server at %s:%d\n", server_ip, port);

    // Start a background thread to print messages as they come in
    uintptr_t tid = _beginthreadex(NULL, 0, receive_thread, &sock, 0, NULL);
    if (tid)
        CloseHandle((HANDLE)tid);

    char buffer[BUFFER_SIZE];
    // This loop just reads what you type and sends it to the server
    while (fgets(buffer, sizeof(buffer), stdin))
    {
        send(sock, buffer, (int)strlen(buffer), 0);
    }

    closesocket(sock);
    WSACleanup();
    return 0;
}

/*
-------------------------
USAGE:

1. Make sure the server is running and you know its IP address and port.

2. Run the client from a terminal:

   client.exe 192.168.1.50 12345

   (Replace 192.168.1.50 and 12345 with your server's IP and port.)

3. Type messages and press Enter to send. All connected users will see your messages.

-------------------------
*/
