#include "server.h"

#include "request_parser.h"

#include <windows.h>
#include <string.h> // For strlen
#include <stdio.h>

#define THREADING_ENABLED 1
#define THREAD_COUNT 8

static server *server_ptr = 0;

int server_initialize(server *server)
{
    if (server->port)
    {
        server_ptr = server;
    }
    else
    {
        printf("server_initialize - A value must be passed for server.port.");
    }

    WSADATA wsa_data;
    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0)
    {
        printf("WSAStartup failed.\n");
        return 0;
    }
    printf("server_initialize - Winsock initialized.");

    // Create a socket
    SOCKET listen_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listen_socket == INVALID_SOCKET)
    {
        printf("Socket creation failed.\n");
        server_shutdown();
        return 0;
    }
    server_ptr->listen_socket = listen_socket;
    printf("server_initialize - Socket initialized.");

    // Prepare the sockaddr_in structure
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY; // Bind to any available IP address
    addr.sin_port = htons(server_ptr->port);

    // Bind the socket
    if (bind(listen_socket, (struct sockaddr *)&addr, sizeof(addr)) == SOCKET_ERROR)
    {
        printf("Bind failed.\n");
        server_shutdown();
        return 0;
    }
    printf("server_initialize - Socket bound to port %i.", server_ptr->port);

    router *r = (router *)malloc(sizeof(router));
    r->route_count = 0;
    server_ptr->router = r;

    return 1;
}

int server_run(int backlog)
{
    if (server_ptr)
    {
        SOCKET listen_socket = server_ptr->listen_socket;
        // Listen for incoming connections
        if (listen(listen_socket, backlog) == SOCKET_ERROR)
        {
            printf("Listen failed.\n");
            server_shutdown();
            return 0;
        }

        printf("Server listening on port %i...\n", server_ptr->port);

        if (THREADING_ENABLED)
        {
            thread_router_parameter* param_array[THREAD_COUNT];
            HANDLE thread_array[THREAD_COUNT];

            // Create threads
            for (int i = 0; i < THREAD_COUNT; ++i)
            {
                param_array[i] = (thread_router_parameter*) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(thread_router_parameter));
                HANDLE thread_handle = CreateThread(NULL, 0, router_handle_request_threaded, NULL, 0, NULL);
            }
        }

        while (1)
        {
            SOCKET client_socket = accept(listen_socket, NULL, NULL);
            if (client_socket == INVALID_SOCKET)
            {
                printf("Accept failed.\n");
                continue; // Continue to the next iteration
            }

            char request_buffer[2048] = {0};
            recv(client_socket, request_buffer, sizeof(request_buffer) - 1, 0);

            request *req = (request *)malloc(sizeof(request));
            parse_request(req, request_buffer);

            thread_router_parameter *param;
            router_handle_request(server_ptr->router, req, client_socket);

            free(req->base);
            free(req);

            closesocket(client_socket);
        }
    }

    return 1;
}

void server_shutdown()
{
    if (server_ptr)
    {
        // *** Cleanup #3: Close the listening socket and clean up Winsock ***
        // (This part is unreachable in the while(1) loop, but is good practice
        // for servers that have a shutdown condition).

        if (server_ptr->listen_socket)
        {
            closesocket(server_ptr->listen_socket);
        }
        else
        {
            printf("server_shutdown - no listen_socket was defined.");
        }

        WSACleanup();
    }
    else
    {
        printf("server_shutdown - server_ptr was not defined.");
    }
}