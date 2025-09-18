#include "networking/request_parser.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

enum METHOD parse_method(char* raw_method);

void parse_request(request* req, char* raw_request) {
    base_header* b_header = (base_header*)malloc(sizeof(base_header));
    req->base = b_header;
    
    // Initialize with defaults in case parsing fails
    req->base->method = METHOD_UNKNOWN;
    req->base->path = NULL;
    req->base->version = NULL;
    
    // Check if raw_request is NULL or empty
    if (!raw_request || strlen(raw_request) == 0) {
        printf("parse_request - Empty or NULL request received\n");
        return;
    }
    
    char* raw_base_header = strtok(raw_request, "\n");
    
    // Check if we got a valid first line
    if (!raw_base_header) {
        printf("parse_request - No newline found in request\n");
        return;
    }
    
    // Parse method
    char* method_str = strtok(raw_base_header, " ");
    if (method_str) {
        req->base->method = parse_method(method_str);
    } else {
        printf("parse_request - No method found in request\n");
        return;
    }
    
    // Parse path
    req->base->path = strtok(NULL, " ");
    if (!req->base->path) {
        printf("parse_request - No path found in request\n");
        req->base->method = METHOD_UNKNOWN;
        return;
    }
    
    // Parse version
    req->base->version = strtok(NULL, " ");
    if (!req->base->version) {
        printf("parse_request - No HTTP version found in request\n");
        // This is less critical, some clients might not send it
    }

    // TODO: Parse the rest of the headers
}

enum METHOD parse_method(char* raw_method) {
    // Add NULL check
    if (!raw_method) {
        printf("parse_method - NULL method received\n");
        return METHOD_UNKNOWN;
    }
    
    if (strcmp(raw_method, "GET") == 0)
    {
        return METHOD_GET;
    } else if (strcmp(raw_method, "PUT") == 0)
    {
        return METHOD_PUT;
    } else if (strcmp(raw_method, "POST") == 0)  // Fixed: was "PUSH"
    {
        return METHOD_PUSH;  // You might want to rename this to METHOD_POST
    } else if (strcmp(raw_method, "DELETE") == 0)
    {
        return METHOD_DELETE;
    } else
    {
        printf("parse_method - Unknown method parsed: %s, returning UNKNOWN\n", raw_method);
        return METHOD_UNKNOWN;
        // TODO: Cause this to return a 500 or some similar descriptive response
    }
}