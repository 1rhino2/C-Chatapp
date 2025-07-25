// Build: gcc server.c -o server -lws2_32
// Usage: server.exe <port>
// This is a simple chat server for Windows, written in C.
// Up to 10 people can connect (though let's be real, it's usually just you and a friend).
// When someone sends a message, everyone else sees it.
// The server will print out your local IP addresses so people know how to connect.
// If you run into trouble, check your firewall settings.

#define _WIN32_WINNT 0x0601
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <process.h>

#define MAX_CLIENTS 10
#define BUFFER_SIZE 2048

SOCKET client_sockets[MAX_CLIENTS] = {0}; // Holds sockets for everyone who's connected
CRITICAL_SECTION clients_mutex;           // Used so threads don't step on each other

// Print all the local IPv4 addresses your computer has,
// so you know what to tell your friends to connect to.
void print_local_ip_and_port(int port)
{
    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) == SOCKET_ERROR)
    {
        printf("Couldn't get computer name. Sorry!\n");
        return;
    }

    struct addrinfo hints = {0}, *info = NULL, *p = NULL;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(hostname, NULL, &hints, &info) != 0 || info == NULL)
    {
        printf("Couldn't figure out local IP addresses.\n");
        return;
    }

    printf("\nTell your friends to use one of these IPs with port %d:\n", port);
    for (p = info; p != NULL; p = p->ai_next)
    {
        struct sockaddr_in *addr = (struct sockaddr_in *)p->ai_addr;
        printf("  %s:%d\n", inet_ntoa(addr->sin_addr), port);
    }
    freeaddrinfo(info);
    printf("\n");
}

// This sends a message to everyone except the person who sent it.
void broadcast_message(const char *message, SOCKET sender)
{
    EnterCriticalSection(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; ++i)
    {
        if (client_sockets[i] != 0 && client_sockets[i] != sender)
        {
            send(client_sockets[i], message, (int)strlen(message), 0);
        }
    }
    LeaveCriticalSection(&clients_mutex);
}

// Every time someone connects, this runs in a new thread just for them.
// It listens for their messages and broadcasts them to everyone else.
unsigned __stdcall client_handler(void *arg)
{
    SOCKET client = *(SOCKET *)arg;
    free(arg);

    char buffer[BUFFER_SIZE];
    int bytes;
    while ((bytes = recv(client, buffer, sizeof(buffer) - 1, 0)) > 0)
    {
        buffer[bytes] = '\0';
        broadcast_message(buffer, client);
    }

    // If we get here, the client disconnected.
    // Take them out of the list so their slot is free for someone else.
    EnterCriticalSection(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; ++i)
    {
        if (client_sockets[i] == client)
        {
            client_sockets[i] = 0;
            break;
        }
    }
    LeaveCriticalSection(&clients_mutex);

    closesocket(client);
    _endthreadex(0);
    return 0;
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Usage: %s <port>\n", argv[0]);
        printf("Example: %s 12345\n", argv[0]);
        return 1;
    }

    // Windows Sockets startup stuff
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
    {
        printf("Couldn't start WinSock. Error: %d\n", WSAGetLastError());
        return 1;
    }

    // Set up our mutex so threads play nice
    InitializeCriticalSection(&clients_mutex);

    int port = atoi(argv[1]);
    SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_sock == INVALID_SOCKET)
    {
        printf("Couldn't create socket. Error: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    // Fill in the server details
    struct sockaddr_in server_addr = {0};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    // Bind the socket to this address (so the OS knows where to send stuff)
    if (bind(listen_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == SOCKET_ERROR)
    {
        printf("Bind failed. Error: %d\n", WSAGetLastError());
        closesocket(listen_sock);
        WSACleanup();
        return 1;
    }

    // Start listening for people who want to join the chat
    if (listen(listen_sock, MAX_CLIENTS) == SOCKET_ERROR)
    {
        printf("Listen failed. Error: %d\n", WSAGetLastError());
        closesocket(listen_sock);
        WSACleanup();
        return 1;
    }

    printf("=====================================\n");
    printf("  Chat server is up and running.\n");
    printf("  Port: %d\n", port);
    print_local_ip_and_port(port);
    printf("=====================================\n");

    while (1)
    {
        struct sockaddr_in client_addr;
        int addr_len = sizeof(client_addr);
        SOCKET *client_sock = (SOCKET *)malloc(sizeof(SOCKET)); // Give each client its own socket variable
        if (!client_sock)
        {
            printf("Out of memory! (This shouldn't happen)\n");
            continue;
        }
        *client_sock = accept(listen_sock, (struct sockaddr *)&client_addr, &addr_len);
        if (*client_sock == INVALID_SOCKET)
        {
            printf("Accept failed: %d\n", WSAGetLastError());
            free(client_sock);
            continue;
        }

        // Stick the new client in the first empty spot in our array
        EnterCriticalSection(&clients_mutex);
        int added = 0;
        for (int i = 0; i < MAX_CLIENTS; ++i)
        {
            if (client_sockets[i] == 0)
            {
                client_sockets[i] = *client_sock;
                added = 1;
                break;
            }
        }
        LeaveCriticalSection(&clients_mutex);

        if (!added)
        {
            // No room, so let them know and close the connection
            const char *msg = "Chat room is full. Try again later.\n";
            send(*client_sock, msg, (int)strlen(msg), 0);
            closesocket(*client_sock);
            free(client_sock);
        }
        else
        {
            // Start a new thread for this client
            uintptr_t th = _beginthreadex(NULL, 0, client_handler, client_sock, 0, NULL);
            if (th)
                CloseHandle((HANDLE)th);
            printf("New client connected: %s:%d\n",
                   inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        }
    }

    // Cleanup (not that you'll ever get here, but just in case)
    closesocket(listen_sock);
    DeleteCriticalSection(&clients_mutex);
    WSACleanup();
    return 0;
}

/*
-------------------------
USAGE EXAMPLE:

1. Start the server in a terminal:

   server.exe 12345

   (You can use any port number you want.)

   The server will print out the IP addresses and port that clients should use to connect.
   For example, if it prints:
       Tell your friends to use one of these IPs with port 12345:
         127.0.0.1:12345

   Then clients on your network should use one of those.

2. Connect clients from other terminals or computers:

   client.exe 192.168.1.50 12345

   (Replace 192.168.1.50 with your server's actual IP address.)

3. Anyone connected can type messages and all other clients will see them.
   Just type and hit Enter!

-------------------------
*/