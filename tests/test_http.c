#include "http_client.h"
#include "exit_codes.h"
#include "location.h"
#include "test_support.h"
#include "fault_alloc.h"

#include <curl/curl.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>

typedef size_t (*WriteCallback)(char *, size_t, size_t, void *);
static WriteCallback write_body;
static void *write_context;
static CURLcode transfer_result;
static long response_status;
static int scenario;
static unsigned int configured;
static bool fail_option;
static bool fail_info;
static bool fail_init;
static bool fail_header;

CURLcode __wrap_curl_easy_setopt(CURL *curl, CURLoption option, ...);
CURLcode __wrap_curl_easy_perform(CURL *curl);
CURLcode __wrap_curl_easy_getinfo(CURL *curl, CURLINFO info, ...);
CURL *__wrap_curl_easy_init(void);
CURL *__real_curl_easy_init(void);
struct curl_slist *__wrap_curl_slist_append(struct curl_slist *list, const char *text);
struct curl_slist *__real_curl_slist_append(struct curl_slist *list, const char *text);

CURL *__wrap_curl_easy_init(void)
{
    return fail_init ? NULL : __real_curl_easy_init();
}

struct curl_slist *__wrap_curl_slist_append(struct curl_slist *list, const char *text)
{
    return fail_header ? NULL : __real_curl_slist_append(list, text);
}

CURLcode __wrap_curl_easy_setopt(CURL *curl, CURLoption option, ...)
{
    (void)curl;
    if (fail_option) {
        return CURLE_UNKNOWN_OPTION;
    }
    va_list arguments;
    va_start(arguments, option);
    switch (option) {
    case CURLOPT_WRITEFUNCTION:
        write_body = va_arg(arguments, WriteCallback); configured |= 1u; break;
    case CURLOPT_WRITEDATA:
        write_context = va_arg(arguments, void *); configured |= 2u; break;
    case CURLOPT_SSL_VERIFYPEER:
        assert(va_arg(arguments, long) == 1); configured |= 4u; break;
    case CURLOPT_SSL_VERIFYHOST:
        assert(va_arg(arguments, long) == 2); configured |= 8u; break;
    case CURLOPT_FOLLOWLOCATION:
        assert(va_arg(arguments, long) == 0); configured |= 16u; break;
    case CURLOPT_CONNECTTIMEOUT:
        assert(va_arg(arguments, long) == GEOME_CONNECT_TIMEOUT); configured |= 32u; break;
    case CURLOPT_TIMEOUT:
        assert(va_arg(arguments, long) == GEOME_TOTAL_TIMEOUT); configured |= 64u; break;
    case CURLOPT_NOSIGNAL:
        assert(va_arg(arguments, long) == 1); configured |= 128u; break;
    case CURLOPT_USERAGENT:
        assert(strcmp(va_arg(arguments, const char *), GEOME_PROGRAM "/" GEOME_VERSION) == 0);
        configured |= 256u; break;
    case CURLOPT_HTTPHEADER: {
        struct curl_slist *headers = va_arg(arguments, struct curl_slist *);
        assert(strcmp(headers->data, "Accept: application/json") == 0);
        configured |= 512u; break;
    }
    case CURLOPT_URL:
        assert(strcmp(va_arg(arguments, const char *), "https://ipwho.is/") == 0);
        configured |= 1024u; break;
    case CURLOPT_ACCEPT_ENCODING:
        assert(strcmp(va_arg(arguments, const char *), "") == 0); configured |= 2048u; break;
#if LIBCURL_VERSION_NUM >= 0x075500
    case CURLOPT_PROTOCOLS_STR:
        assert(strcmp(va_arg(arguments, const char *), "https") == 0); configured |= 4096u; break;
#else
    case CURLOPT_PROTOCOLS:
        assert(va_arg(arguments, long) == CURLPROTO_HTTPS); configured |= 4096u; break;
#endif
    default: assert(false);
    }
    va_end(arguments);
    return CURLE_OK;
}

CURLcode __wrap_curl_easy_perform(CURL *curl)
{
    (void)curl;
    assert(configured == 8191u);
    if (transfer_result != CURLE_OK) {
        return transfer_result;
    }
    if (scenario == 1) {
        assert(write_body(NULL, SIZE_MAX, 2, write_context) == 0);
        return CURLE_WRITE_ERROR;
    }
    if (scenario == 2 || scenario == 3) {
        size_t length;
        char *seed = read_fixture("tests/fixtures/oversized.json", &length);
        size_t sent = 0;
        while (sent < GEOME_RESPONSE_LIMIT) {
            size_t count = GEOME_RESPONSE_LIMIT - sent;
            if (count > length) { count = length; }
            assert(write_body(seed, 1, count, write_context) == count);
            sent += count;
        }
        free(seed);
        if (scenario == 2) {
            assert(write_body("x", 1, 1, write_context) == 0);
            return CURLE_WRITE_ERROR;
        }
        return CURLE_OK;
    }
    if (scenario == 4) {
        fault_after = 0;
        assert(write_body("x", 1, 1, write_context) == 0);
        fault_after = -1;
        return CURLE_WRITE_ERROR;
    }
    assert(write_body(NULL, 0, 1, write_context) == 0);
    assert(write_body("{", 1, 1, write_context) == 1);
    assert(write_body("}", 1, 1, write_context) == 1);
    return CURLE_OK;
}

CURLcode __wrap_curl_easy_getinfo(CURL *curl, CURLINFO info, ...)
{
    (void)curl;
    assert(info == CURLINFO_RESPONSE_CODE);
    if (fail_info) { return CURLE_BAD_FUNCTION_ARGUMENT; }
    va_list arguments;
    va_start(arguments, info);
    *va_arg(arguments, long *) = response_status;
    va_end(arguments);
    return CURLE_OK;
}

static void request(int expected)
{
    configured = 0;
    char *body = NULL;
    size_t length = 99;
    long status;
    char error[256] = "";
    assert(http_get_json("https://ipwho.is/", &body, &length, &status,
                         error, sizeof(error)) == expected);
    if (expected == GEOME_OK) {
        assert(body != NULL && body[length] == '\0');
        assert(length == (scenario == 3 ? GEOME_RESPONSE_LIMIT : 2));
        free(body);
    } else {
        assert(body == NULL && length == 0 && error[0] != '\0');
        if (response_status == 429 && !fail_init && !fail_header) {
            assert(strstr(error, "rate limit exceeded") != NULL);
        }
    }
}

int main(void)
{
    assert(curl_global_init(CURL_GLOBAL_DEFAULT) == CURLE_OK);
    response_status = 200;
    request(GEOME_OK);
    scenario = 1; request(GEOME_DATA);
    scenario = 2; request(GEOME_DATA);
    scenario = 3; request(GEOME_OK);
    scenario = 4; request(GEOME_INTERNAL);
    scenario = 0;
    long statuses[] = {0, 301, 400, 403, 404, 429, 500, 503};
    for (size_t index = 0; index < sizeof(statuses) / sizeof(statuses[0]); ++index) {
        response_status = statuses[index]; request(GEOME_SERVICE);
    }
    response_status = 0;
    CURLcode errors[] = {CURLE_COULDNT_RESOLVE_HOST, CURLE_COULDNT_RESOLVE_PROXY,
                        CURLE_COULDNT_CONNECT, CURLE_SSL_CONNECT_ERROR,
                        CURLE_PEER_FAILED_VERIFICATION, CURLE_OPERATION_TIMEDOUT,
                        CURLE_PARTIAL_FILE, CURLE_RECV_ERROR, CURLE_OUT_OF_MEMORY};
    for (size_t index = 0; index < sizeof(errors) / sizeof(errors[0]); ++index) {
        transfer_result = errors[index];
        request(errors[index] == CURLE_OUT_OF_MEMORY ? GEOME_INTERNAL : GEOME_NETWORK);
    }
    transfer_result = CURLE_WRITE_ERROR;
    response_status = 429;
    request(GEOME_SERVICE);
    transfer_result = CURLE_OK;
    response_status = 200;
    fail_option = true; request(GEOME_INTERNAL); fail_option = false;
    fail_info = true; request(GEOME_INTERNAL); fail_info = false;
    fail_init = true; request(GEOME_INTERNAL); fail_init = false;
    fail_header = true; request(GEOME_INTERNAL); fail_header = false;
    fault_after = 0; request(GEOME_INTERNAL); fault_after = -1;
    char *body;
    size_t length;
    long status;
    char error[256];
    assert(http_get_json("http://ipwho.is/", &body, &length, &status,
                         error, sizeof(error)) == GEOME_INTERNAL);
    curl_global_cleanup();
    puts("HTTP tests passed");
    return 0;
}
