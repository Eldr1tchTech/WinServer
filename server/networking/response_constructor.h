#pragma once

#include <string.h>

enum STATUS_CODE
{
    STATUS_CODE_200,
    STATUS_CODE_404,
    STATUS_CODE_500
} STATUS_CODE;

char *construct_response(char *version, enum STATUS_CODE status_code, char *content_type, long file_size, char *file_content);