#pragma once

enum METHOD {
    METHOD_GET,
    METHOD_PUT,
    METHOD_PUSH,
    METHOD_DELETE,
    // TODO: Add the rest
    METHOD_UNKNOWN
};

typedef struct base_header
{
    enum METHOD method;
    char* path;
    char* version;
} base_header;

typedef struct header
{
    char* name;
    char* content;
} header;
typedef struct request
{
    base_header* base;
    header* headers;
} request;

void parse_request(request* req, char* raw_request);