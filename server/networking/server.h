#pragma once

// TODO: Create a socket wrapper
#include "router.h"

#include "win_headers.h"

typedef struct server
{
    int port;
    SOCKET listen_socket;
    router* router;
} server;

int server_initialize(server* server);

int server_run(int backlog);

void server_shutdown();