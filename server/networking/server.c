#include "server.h"

#include <windows.h>
#include <string.h> // For strlen
#include <stdio.h>

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

        while (1)
        {
            SOCKET client_socket = accept(listen_socket, NULL, NULL);
            if (client_socket == INVALID_SOCKET)
            {
                printf("Accept failed.\n");
                continue; // Continue to the next iteration
            }

            char request[1024] = {0};
            recv(client_socket, request, sizeof(request) - 1, 0);

            printf("server_run - Request: \n%s\n", request);

            // Check for a simple GET / request
            if (memcmp(request, "GET / ", 6) == 0)
            {
                FILE *f = fopen("assets/index.html", "rb"); // Open in binary read mode
                if (f)
                {
                    // Find the file size
                    fseek(f, 0, SEEK_END);
                    long file_size = ftell(f);
                    fseek(f, 0, SEEK_SET);

                    // Allocate memory for the file content
                    char *file_content = (char *)malloc(file_size);
                    if (file_content)
                    {
                        fread(file_content, 1, file_size, f);

                        // Create a proper HTTP response
                        char response_buffer[2048] = {0};
                        sprintf(response_buffer,
                                "HTTP/1.1 200 OK\r\n"
                                "Content-Type: text/html\r\n"
                                "Content-Length: %ld\r\n"
                                "\r\n" // Empty line separates headers from body
                                "%s",  // The file content
                                file_size, file_content);

                        send(client_socket, response_buffer, strlen(response_buffer), 0);

                        printf("server_run - Response: \n%s\n", response_buffer);

                        free(file_content);
                    }
                    // *** Cleanup #1: Close the file ***
                    fclose(f);
                }
                else
                {
                    // Send a 404 Not Found response if file doesn't exist
                    const char *not_found_response = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
                    send(client_socket, not_found_response, strlen(not_found_response), 0);
                }
            }

            // *** Cleanup #2: Close the client socket ***
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