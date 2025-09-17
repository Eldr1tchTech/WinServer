#include "networking/request_parser.h"

#include <string.h>
#include <stdio.h>

enum METHOD parse_method(char* raw_method);

void parse_request(request* req, char* raw_request) {
    base_header* b_header = (base_header*)malloc(sizeof(base_header));
    req->base = b_header;
    char* raw_base_header = strtok(raw_request, "\n");

    req->base->method = parse_method(strtok(raw_base_header, " "));
    req->base->path = strtok(NULL, " ");
    req->base->version = strtok(NULL, " ");

    // TODO: Parse the rest of the headers
}

enum METHOD parse_method(char* raw_method) {
    if (strcmp(raw_method, "GET") == 0)
    {
        return METHOD_GET;
    } else if (strcmp(raw_method, "PUT") == 0)
    {
        return METHOD_PUT;
    } else if (strcmp(raw_method, "PUSH") == 0)
    {
        return METHOD_PUSH;
    } else if (strcmp(raw_method, "DELETE") == 0)
    {
        return METHOD_DELETE;
    } else
    {
        printf("parse_method - Unknown method parsed, returning UNKNOWN");
        return METHOD_UNKNOWN;
        // TODO: Cause this to return a 500 or some similar descriptive response
    }
}