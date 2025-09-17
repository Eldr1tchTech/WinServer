#include "router.h"

#include "response_constructor.h"

#include <stdio.h>

#define BASE_PATH "assets"

char **segment_raw_path(char *raw_path);
int is_segment_dynamic(char *segment_path);

int router_add_route(router *router, enum METHOD method, char *path, void (*handler)(SOCKET client_socket, char **arguments[16][64]))
{
    route *current_route = router->routes[router->route_count];
    router->route_count++;

    char **path_segments = segment_raw_path(path);
    route_segment *current_segment = {0};

    current_route->method = method;
    int seg_count = 0;
    while (path_segments[seg_count] != 0)
    {
        int is_dynamic = is_segment_dynamic(path_segments[seg_count]);
        current_segment->seg_type = is_dynamic ? SEGMENT_TYPE_DYNAMIC : SEGMENT_TYPE_STATIC;
        current_segment->seg_path = is_dynamic ? path_segments[seg_count] + 1 : path_segments[seg_count];
        current_route->segments[seg_count] = current_segment;
        current_segment = 0;
        seg_count++;
    }
    current_route->segment_count = seg_count;
    current_route->handler = handler;

    return 1;
}

void router_handle_request(router *router, request* req, SOCKET client_socket)
{
    if (req->base.method == METHOD_GET && strcmp(req->base.path, "/") == 0)
    {
        req->base.path = "/index.html";
        router_handle_request(router, req, client_socket);
    }

    route *current_route;
    route_segment *current_segment;
    char **request_path_segments;
    request_path_segments = segment_raw_path(req->base.path);
    int current_argument = 0;
    char **arguments;
    int is_route_handled = 0;
    for (int i = 0; i < router->route_count && !is_route_handled; ++i)
    {
        current_route = router->routes[i];
        if (current_route->method == req->base.method)
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
}

char **segment_raw_path(char *raw_path)
{
    char **out_buffer[16][64] = {0};
    char *temp_buffer;
    sprintf(temp_buffer, "/%s", raw_path);
    strtok(temp_buffer, "/");
    char *current_segment;
    int i = 0;
    while (current_segment = strtok(NULL, "/") != NULL && i < 16)
    {
        snprintf(out_buffer[i], 64, current_segment);
        ++i;
    }
    return out_buffer;
}

int is_segment_dynamic(char *segment_path)
{
    if (segment_path[0] == ":")
    {
        return 1;
    }

    return 0;
}

void router_send_response(char *path, SOCKET client_socket)
{
    char *path_buffer;
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
            if (strcmp(path, "404.html") == 0)
            {
                response_buffer = construct_response("HTTP/1.1", STATUS_CODE_404, "text/html", file_size, file_content);
            }
            else if (strcmp(path, "500.html") == 0)
            {
                response_buffer = construct_response("HTTP/1.1", STATUS_CODE_500, "text/html", file_size, file_content);
            }
            else
            {
                response_buffer = construct_response("HTTP/1.1", STATUS_CODE_200, "text/html", file_size, file_content);
            }

            send(client_socket, response_buffer, strlen(response_buffer), 0);

            printf("server_run - Response: \n%s\n", response_buffer);

            free(file_content);
        }

        fclose(f);
    }
    else
    {
        router_send_response("404.html", client_socket);
    }
}
