#include "server.h"

#include "request_parser.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>  // For malloc/free

#define THREADING_ENABLED 1
#define THREAD_COUNT 16
#define PENDING_ACCEPTS 64  // Change to something larger probably

static server *server_ptr = 0;
static HANDLE iocp = NULL;
static LPFN_ACCEPTEX lpfnAcceptEx = NULL;

typedef enum {
    OP_ACCEPT,
    OP_PROCESS
} operation_type;

typedef struct {
    OVERLAPPED overlapped;
    operation_type op_type;
    SOCKET socket;
    SOCKET accept_socket;
    char buffer[2048];
    DWORD bytes_received;
} io_context;

// Initialize AcceptEx function pointer
BOOL init_acceptex(SOCKET listen_socket) {
    GUID guid_acceptex = WSAID_ACCEPTEX;
    DWORD bytes;
    
    int result = WSAIoctl(
        listen_socket,
        SIO_GET_EXTENSION_FUNCTION_POINTER,
        &guid_acceptex,
        sizeof(guid_acceptex),
        &lpfnAcceptEx,
        sizeof(lpfnAcceptEx),
        &bytes,
        NULL,
        NULL
    );
    
    return (result == 0);
}

// Post an async accept operation
void post_accept(SOCKET listen_socket) {
    io_context* ctx = (io_context*)calloc(1, sizeof(io_context));
    ctx->op_type = OP_ACCEPT;
    ctx->socket = listen_socket;
    
    // Create socket for accepting
    ctx->accept_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (ctx->accept_socket == INVALID_SOCKET) {
        free(ctx);
        return;
    }
    
    // Post AcceptEx
    DWORD bytes_received = 0;
    BOOL result = lpfnAcceptEx(
        listen_socket,
        ctx->accept_socket,
        ctx->buffer,
        sizeof(ctx->buffer) - ((sizeof(struct sockaddr_in) + 16) * 2),
        sizeof(struct sockaddr_in) + 16,
        sizeof(struct sockaddr_in) + 16,
        &bytes_received,
        &ctx->overlapped
    );
    
    if (!result && WSAGetLastError() != ERROR_IO_PENDING) {
        closesocket(ctx->accept_socket);
        free(ctx);
    }
}

DWORD WINAPI async_worker_thread(LPVOID param) {
    DWORD bytes_transferred;
    ULONG_PTR completion_key;
    LPOVERLAPPED overlapped;
    
    printf("Async worker thread %ld started\n", (unsigned long)GetCurrentThreadId());
    
    while (1) {
        BOOL result = GetQueuedCompletionStatus(
            iocp,
            &bytes_transferred,
            &completion_key,
            &overlapped,
            INFINITE
        );
        
        if (completion_key == 0 && overlapped == NULL) {
            // Shutdown signal
            break;
        }
        
        io_context* ctx = CONTAINING_RECORD(overlapped, io_context, overlapped);
        
        if (ctx->op_type == OP_ACCEPT) {
            if (result) {
                // Accept completed successfully
                // Update accept socket context
                setsockopt(ctx->accept_socket, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT,
                          (char*)&ctx->socket, sizeof(ctx->socket));
                
                // Post another accept to replace this one
                post_accept(ctx->socket);
                
                if (bytes_transferred > 0) {
                    // Data was received with accept, process it
                    ctx->buffer[bytes_transferred] = '\0';
                    
                    request *req = (request *)malloc(sizeof(request));
                    parse_request(req, ctx->buffer);
                    router_handle_request(server_ptr->router, req, ctx->accept_socket);
                    free(req->base);
                    free(req);
                    
                    closesocket(ctx->accept_socket);
                } else {
                    // No data yet, need to receive
                    io_context* process_ctx = (io_context*)calloc(1, sizeof(io_context));
                    process_ctx->op_type = OP_PROCESS;
                    process_ctx->socket = ctx->accept_socket;
                    
                    // Associate with IOCP for further processing
                    CreateIoCompletionPort((HANDLE)ctx->accept_socket, iocp, 0, 0);
                    
                    // Post a receive operation
                    WSABUF wsabuf;
                    wsabuf.buf = process_ctx->buffer;
                    wsabuf.len = sizeof(process_ctx->buffer);
                    DWORD flags = 0;
                    
                    WSARecv(ctx->accept_socket, &wsabuf, 1, &bytes_transferred,
                           &flags, &process_ctx->overlapped, NULL);
                }
            } else {
                // Accept failed, post another one
                closesocket(ctx->accept_socket);
                post_accept(ctx->socket);
            }
            
            free(ctx);
            
        } else if (ctx->op_type == OP_PROCESS) {
            // Process received data
            if (result && bytes_transferred > 0) {
                ctx->buffer[bytes_transferred] = '\0';
                
                request *req = (request *)malloc(sizeof(request));
                parse_request(req, ctx->buffer);
                router_handle_request(server_ptr->router, req, ctx->socket);
                free(req->base);
                free(req);
            }
            
            closesocket(ctx->socket);
            free(ctx);
        }
    }
    
    return 0;
}

int server_initialize(server *server) {
    if (server->port) {
        server_ptr = server;
    } else {
        printf("server_initialize - A value must be passed for server.port.\n");
        return 0;
    }
    
    WSADATA wsa_data;
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
        printf("WSAStartup failed.\n");
        return 0;
    }
    printf("server_initialize - Winsock initialized.\n");
    
    SOCKET listen_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listen_socket == INVALID_SOCKET) {
        printf("Socket creation failed.\n");
        server_shutdown();
        return 0;
    }
    server_ptr->listen_socket = listen_socket;
    printf("server_initialize - Socket initialized.\n");
    
    // Enable SO_REUSEADDR to avoid "Address already in use" errors
    int reuse = 1;
    if (setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR, 
                   (const char*)&reuse, sizeof(reuse)) < 0) {
        printf("setsockopt(SO_REUSEADDR) failed.\n");
    }
    
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(server_ptr->port);
    
    if (bind(listen_socket, (struct sockaddr *)&addr, sizeof(addr)) == SOCKET_ERROR) {
        printf("Bind failed with error: %d\n", WSAGetLastError());
        server_shutdown();
        return 0;
    }
    printf("server_initialize - Socket bound to port %i.\n", server_ptr->port);
    
    router *r = (router *)malloc(sizeof(router));
    r->route_count = 0;
    server_ptr->router = r;
    
    return 1;
}

int server_run(int backlog) {
    if (!server_ptr) {
        printf("Server not initialized.\n");
        return 0;
    }
    
    SOCKET listen_socket = server_ptr->listen_socket;
    
    if (listen(listen_socket, backlog) == SOCKET_ERROR) {
        printf("Listen failed with error: %d\n", WSAGetLastError());
        server_shutdown();
        return 0;
    }
    
    printf("Server listening on port %i...\n", server_ptr->port);
    
    if (THREADING_ENABLED) {
        // Initialize AcceptEx
        if (!init_acceptex(listen_socket)) {
            printf("Failed to initialize AcceptEx\n");
            return 0;
        }
        
        // Create IOCP
        iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
        if (iocp == NULL) {
            printf("Failed to create IOCP\n");
            return 0;
        }
        
        // Associate listen socket with IOCP
        if (CreateIoCompletionPort((HANDLE)listen_socket, iocp, 0, 0) == NULL) {
            printf("Failed to associate listen socket with IOCP\n");
            return 0;
        }
        
        // Create worker threads
        HANDLE threads[THREAD_COUNT];
        for (int i = 0; i < THREAD_COUNT; i++) {
            threads[i] = CreateThread(NULL, 0, async_worker_thread, NULL, 0, NULL);
            if (threads[i] == NULL) {
                printf("Failed to create thread %d\n", i);
            }
        }
        
        printf("Created %d async worker threads\n", THREAD_COUNT);
        
        // Post initial accepts
        for (int i = 0; i < PENDING_ACCEPTS; i++) {
            post_accept(listen_socket);
        }
        
        printf("Posted %d pending accepts\n", PENDING_ACCEPTS);
        
        // Main thread can do other work or just wait
        // The worker threads handle everything
        while (1) {
            Sleep(1000);
            // Could add monitoring/statistics here
        }
        
    } else {
        // Non-threaded fallback
        while (1) {
            SOCKET client_socket = accept(listen_socket, NULL, NULL);
            if (client_socket == INVALID_SOCKET) {
                printf("Accept failed.\n");
                continue;
            }
            
            char request_buffer[2048] = {0};
            recv(client_socket, request_buffer, sizeof(request_buffer) - 1, 0);
            
            request *req = (request *)malloc(sizeof(request));
            parse_request(req, request_buffer);
            router_handle_request(server_ptr->router, req, client_socket);
            free(req->base);
            free(req);
            
            closesocket(client_socket);
        }
    }
    
    return 1;
}

void server_shutdown() {
    if (server_ptr) {
        if (iocp) {
            // Signal threads to shutdown
            for (int i = 0; i < THREAD_COUNT; i++) {
                PostQueuedCompletionStatus(iocp, 0, 0, NULL);
            }
            CloseHandle(iocp);
        }
        
        if (server_ptr->listen_socket) {
            closesocket(server_ptr->listen_socket);
        } else {
            printf("server_shutdown - no listen_socket was defined.\n");
        }
        
        WSACleanup();
    } else {
        printf("server_shutdown - server_ptr was not defined.\n");
    }
}