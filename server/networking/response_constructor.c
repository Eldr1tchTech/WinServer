#include "response_constructor.h"

#include <stdio.h>

// TODO: Take a buffer as a parameter to write to
char *construct_response(char *version, enum STATUS_CODE status_code, char *content_type, long file_size, char *file_content) {
    char* out_buffer = (char*)malloc(2048 + file_size);
    char* raw_status_code;

    if (status_code == STATUS_CODE_200)
    {
        raw_status_code = "200 OK";
    } else if (status_code == STATUS_CODE_404)
    {
        raw_status_code = "404 Not Found";
    } else if (status_code == STATUS_CODE_500)
    {
        raw_status_code = "500 Internal Server Error";
    } else
    {
        printf("construct_response - Failed to parse the STATUS_CODE. Setting to 500.");
        raw_status_code = "500 Internal Server Error";
    }

    sprintf(out_buffer,
        "%s %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %ld\r\n"
        "\r\n"
        "%s",
        version, raw_status_code, content_type, file_size, file_content);

    return out_buffer;
}