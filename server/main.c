#include "networking/server.h"
#include "networking/router.h"

#include <winsock.h>
#include <Windows.h>

void ROUTE_HANDLER_404(SOCKET client_socket, char** arguments) {
    router_send_response("404.html", client_socket);
}
void ROUTE_HANDLER_500(SOCKET client_socket, char** arguments) {
    router_send_response("500.html", client_socket);
}
void ROUTE_HANDLER_index(SOCKET client_socket, char** arguments) {
    router_send_response("index.html", client_socket);
}
void ROUTE_HANDLER_test(SOCKET client_socket, char** arguments) {
    router_send_response("test.html", client_socket);
}

int main() {
    // Allocate server on heap instead of using uninitialized pointer
    server* s = (server*)malloc(sizeof(server));
    s->port = 8080;

    int result = server_initialize(s);
    if (result)
    {
        router_add_route(s->router, METHOD_GET, "404.html", ROUTE_HANDLER_404);
        router_add_route(s->router, METHOD_GET, "500.html", ROUTE_HANDLER_500);
        router_add_route(s->router, METHOD_GET, "index.html", ROUTE_HANDLER_index);
        router_add_route(s->router, METHOD_GET, "test.html", ROUTE_HANDLER_test);
        result = server_run(10);
        if (result)
        {
            server_shutdown();
            return 0;
        }
    }

    free(s);
    return 1;
}