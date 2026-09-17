#ifndef GEOME_HTTP_CLIENT_H
#define GEOME_HTTP_CLIENT_H

#include <stddef.h>

#define GEOME_CONNECT_TIMEOUT 5L
#define GEOME_TOTAL_TIMEOUT 10L

int http_get_json(const char *url, char **response_body, size_t *response_size,
                  long *http_status, char *error_message, size_t error_message_size);

#endif
