#include "router.h"

#include "response_constructor.h"

#include <stdio.h>

#define BASE_PATH "assets"

char **segment_raw_path(char *raw_path);
int is_segment_dynamic(char *segment_path);

int router_add_route(router *router, enum METHOD method, char *path, void (*handler)(SOCKET client_socket, char **arguments))
{
    // Allocate memory for the new route
    route *current_route = (route*)malloc(sizeof(route));
    router->routes[router->route_count] = current_route;
    router->route_count++;

    char **path_segments = segment_raw_path(path);
    route_segment *current_segment = {0};

    current_route->method = method;
    int seg_count = 0;
    while (path_segments[seg_count] != 0)
    {
        current_segment = (route_segment*)malloc(sizeof(route_segment));
        int is_dynamic = is_segment_dynamic(path_segments[seg_count]);
        current_segment->seg_type = is_dynamic ? SEGMENT_TYPE_DYNAMIC : SEGMENT_TYPE_STATIC;
        current_segment->seg_path = is_dynamic ? path_segments[seg_count] + 1 : path_segments[seg_count];
        current_route->segments[seg_count] = current_segment;
        seg_count++;
    }
    current_route->segment_count = seg_count;
    current_route->handler = handler;

    return 1;
}

void router_handle_request(router *router, request* req, SOCKET client_socket)
{
    // Handle parsing failures
    if (req->base->method == METHOD_UNKNOWN || !req->base->path) {
        router_send_response("400.html", client_socket);  // Or 500.html
        return;
    }
    
    if (req->base->method == METHOD_GET && strcmp(req->base->path, "/") == 0)
    {
        req->base->path = "index.html";
        router_handle_request(router, req, client_socket);
        return;
    }

    route *current_route;
    route_segment *current_segment;
    char **request_path_segments;
    request_path_segments = segment_raw_path(req->base->path);
    int current_argument = 0;
    char *arguments[16];
    int is_route_handled = 0;
    for (int i = 0; i < router->route_count && !is_route_handled; ++i)
    {
        current_route = router->routes[i];
        if (current_route->method == req->base->method)
        {
            for (int j = 0; j < current_route->segment_count && !is_route_handled; ++j)
            {
                current_segment = current_route->segments[j];
                if (strcmp(current_segment->seg_path, request_path_segments[j]) == 0 || current_segment->seg_type == SEGMENT_TYPE_DYNAMIC)
                {
                    if (current_segment->seg_type == SEGMENT_TYPE_DYNAMIC)
                    {
                        arguments[current_argument] = request_path_segments[j];
                        current_argument++;
                    }
                    if (j == current_route->segment_count - 1)
                    {
                        current_route->handler(client_socket, arguments);
                        is_route_handled = 1;
                        break;
                    }
                }
                else
                {
                    break;
                }
            }
            current_argument = 0;
        }
    }

    if (!is_route_handled)
    {
        router_send_response("404.html", client_socket);
    }
    
}

char **segment_raw_path(char *raw_path)
{
    char **out_buffer = (char**)malloc(16 * sizeof(char*));
    memset(out_buffer, 0, 16 * sizeof(char*)); // Initialize all to NULL
    
    char *temp_buffer = (char*)malloc(256);
    sprintf(temp_buffer, "%s", raw_path);
    
    // Skip leading slash if present
    char *start = temp_buffer;
    if (start[0] == '/') {
        start++;
    }
    
    if (strlen(start) == 0) {
        free(temp_buffer);
        return out_buffer;
    }
    
    char *current_segment = strtok(start, "/");
    int i = 0;
    
    while (current_segment != NULL && i < 16) 
    {
        out_buffer[i] = (char*)malloc(64);
        snprintf(out_buffer[i], 64, "%s", current_segment);
        i++;
        current_segment = strtok(NULL, "/");
    }
    
    free(temp_buffer);
    return out_buffer;
}

int is_segment_dynamic(char *segment_path)
{
    if (segment_path[0] == ':')
    {
        return 1;
    }

    return 0;
}

void router_send_response(char *path, SOCKET client_socket)
{
    char path_buffer[256];
    sprintf(path_buffer, "%s/%s", BASE_PATH, path);
    FILE *f = fopen(path_buffer, "rb"); // Open in binary read mode
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

            char *response_buffer = {0};
            response_buffer = (char*)malloc(2048 + file_size);
            if (strcmp(path, "404.html") == 0)
            {
                construct_response(response_buffer, "HTTP/1.1", STATUS_CODE_404, "text/html", file_size, file_content);
            }
            else if (strcmp(path, "500.html") == 0)
            {
                construct_response(response_buffer, "HTTP/1.1", STATUS_CODE_500, "text/html", file_size, file_content);
            }
            else
            {
                construct_response(response_buffer, "HTTP/1.1", STATUS_CODE_200, "text/html", file_size, file_content);
            }

            send(client_socket, response_buffer, strlen(response_buffer), 0);

            free(response_buffer);
            free(file_content);
        }

        fclose(f);
    }
    else
    {
        router_send_response("404.html", client_socket);
    }
}

void router_send_content(char* content, SOCKET client_socket) {
    int content_size = strlen(content);
    char *response_buffer = (char*)malloc(2048 + content_size);
    construct_response(response_buffer, "HTTP/1.1", STATUS_CODE_200, "text/html", content_size, content);

    send(client_socket, response_buffer, strlen(response_buffer), 0);

    free(response_buffer);
}