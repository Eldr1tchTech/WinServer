#pragma once

#include <string.h>

enum STATUS_CODE
{
    STATUS_CODE_200,
    STATUS_CODE_404,
    STATUS_CODE_500
};

void construct_response(char* response_buffer, char *version, enum STATUS_CODE status_code, char *content_type, long file_size, char *file_content);