#pragma once

// Add an array for more headers.
typedef struct request
{
    base_header base;
    header* headers[32];
} request;

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

enum METHOD {
    METHOD_GET,
    METHOD_PUT,
    METHOD_PUSH,
    METHOD_DELETE,
    // TODO: Add the rest
    METHOD_UNKNOWN
} METHOD;

request parse_request(char* raw_request[1024]);

enum METHOD parse_method(char* raw_method);