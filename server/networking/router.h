#pragma once

#include "request_parser.h"

#include <winsock.h>

enum SEGMENT_TYPE {
    SEGMENT_TYPE_STATIC,
    SEGMENT_TYPE_DYNAMIC
};

typedef struct route_segment
{
    char* seg_path;
    enum SEGMENT_TYPE seg_type;
} route_segment;


typedef struct route
{
    enum METHOD method;
    int segment_count;
    route_segment* segments[16];    // TODO: Make this configurable as well
    void (*handler)(SOCKET client_socket, char** arguments);    // TODO: Make arguments configurable
} route;

typedef struct router
{
    int route_count;
    route* routes[64];  // TODO: Make number configurable, probably dynamic eventually
} router;

int router_add_route(router* router, enum METHOD method, char* path, void (*handler)(SOCKET client_socket, char** arguments));

// Should gracefully handle internal errors, so no return value is needed
void router_handle_request(router* router, request* req, SOCKET client_socket);

void router_send_response(char *path, SOCKET client_socket);
void router_send_content(char* content, SOCKET client_socket);