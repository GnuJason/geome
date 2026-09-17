#include "http_client.h"
#include "exit_codes.h"
#include "location.h"

#include <curl/curl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char *data;
    size_t length;
    size_t capacity;
    int failure;
} Response;

static size_t receive_body(char *data, size_t size, size_t count, void *context)
{
    Response *response = context;
    if (size != 0 && count > SIZE_MAX / size) {
        response->failure = GEOME_DATA;
        return 0;
    }
    size_t bytes = size * count;
    if (bytes > GEOME_RESPONSE_LIMIT - response->length) {
        response->failure = GEOME_DATA;
        return 0;
    }
    size_t needed = response->length + bytes + 1;
    if (needed > response->capacity) {
        size_t capacity = response->capacity;
        while (capacity < needed) {
            if (capacity > (GEOME_RESPONSE_LIMIT + 1) / 2) {
                capacity = GEOME_RESPONSE_LIMIT + 1;
            } else {
                capacity *= 2;
            }
        }
        char *grown = realloc(response->data, capacity);
        if (grown == NULL) {
            response->failure = GEOME_INTERNAL;
            return 0;
        }
        response->data = grown;
        response->capacity = capacity;
    }
    if (bytes != 0) {
        memcpy(response->data + response->length, data, bytes);
    }
    response->length += bytes;
    response->data[response->length] = '\0';
    return bytes;
}

int http_get_json(const char *url, char **response_body, size_t *response_size,
                  long *http_status, char *error_message, size_t error_message_size)
{
    *response_body = NULL;
    *response_size = 0;
    *http_status = 0;
    int result = GEOME_INTERNAL;
    const char *reason = "could not initialize HTTPS client";
    CURL *curl = NULL;
    struct curl_slist *headers = NULL;
    Response response = {NULL, 0, 1, GEOME_OK};
    char detail[128];
    if (url == NULL || strncmp(url, "https://", 8) != 0) {
        reason = "only HTTPS provider URLs are permitted";
        goto done;
    }
    response.data = malloc(1);
    if (response.data == NULL) {
        reason = "out of memory receiving location response";
        goto done;
    }
    response.data[0] = '\0';
    curl = curl_easy_init();
    if (curl == NULL) {
        goto done;
    }
    headers = curl_slist_append(NULL, "Accept: application/json");
    if (headers == NULL) {
        reason = "out of memory creating request headers";
        goto done;
    }
    CURLcode status;
#define SET_OPTION(option, value) do { \
    status = curl_easy_setopt(curl, option, value); \
    if (status != CURLE_OK) { \
        reason = "could not configure HTTPS client"; \
        goto done; \
    } \
} while (0)
    SET_OPTION(CURLOPT_URL, url);
    SET_OPTION(CURLOPT_USERAGENT, GEOME_PROGRAM "/" GEOME_VERSION);
    SET_OPTION(CURLOPT_HTTPHEADER, headers);
    SET_OPTION(CURLOPT_WRITEFUNCTION, receive_body);
    SET_OPTION(CURLOPT_WRITEDATA, &response);
    SET_OPTION(CURLOPT_SSL_VERIFYPEER, 1L);
    SET_OPTION(CURLOPT_SSL_VERIFYHOST, 2L);
    SET_OPTION(CURLOPT_FOLLOWLOCATION, 0L);
#if LIBCURL_VERSION_NUM >= 0x075500
    SET_OPTION(CURLOPT_PROTOCOLS_STR, "https");
#else
    SET_OPTION(CURLOPT_PROTOCOLS, (long)CURLPROTO_HTTPS);
#endif
    SET_OPTION(CURLOPT_CONNECTTIMEOUT, GEOME_CONNECT_TIMEOUT);
    SET_OPTION(CURLOPT_TIMEOUT, GEOME_TOTAL_TIMEOUT);
    SET_OPTION(CURLOPT_NOSIGNAL, 1L);
    SET_OPTION(CURLOPT_ACCEPT_ENCODING, "");
#undef SET_OPTION
    status = curl_easy_perform(curl);
    CURLcode info_status = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, http_status);
    if (info_status != CURLE_OK) {
        reason = "could not obtain HTTP status";
        goto done;
    }
    if (*http_status == 429) {
        result = GEOME_SERVICE;
        reason = "location service rate limit exceeded; try again later";
        goto done;
    }
    if (*http_status != 0 && (*http_status < 200 || *http_status >= 300)) {
        result = GEOME_SERVICE;
        (void)snprintf(detail, sizeof(detail), "location service returned HTTP %ld", *http_status);
        reason = detail;
        goto done;
    }
    if (response.failure != GEOME_OK) {
        result = response.failure;
        reason = result == GEOME_DATA ? "location response exceeds the 1 MiB limit" :
                                      "out of memory receiving location response";
        goto done;
    }
    if (status != CURLE_OK) {
        result = status == CURLE_OUT_OF_MEMORY ? GEOME_INTERNAL : GEOME_NETWORK;
        (void)snprintf(detail, sizeof(detail), "HTTPS request failed: %s", curl_easy_strerror(status));
        reason = detail;
        goto done;
    }
    if (*http_status == 0) {
        reason = "location service did not return an HTTP status";
        result = GEOME_SERVICE;
        goto done;
    }
    *response_body = response.data;
    *response_size = response.length;
    response.data = NULL;
    result = GEOME_OK;
done:
    if (result != GEOME_OK && error_message_size != 0) {
        (void)snprintf(error_message, error_message_size, "%s", reason);
    }
    free(response.data);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    return result;
}
