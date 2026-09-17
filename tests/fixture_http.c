#include "http_client.h"
#include "exit_codes.h"
#include "test_support.h"

#include <string.h>

int http_get_json(const char *url, char **body, size_t *length, long *status,
                  char *error, size_t error_size)
{
    assert(strcmp(url, "https://ipwho.is/") == 0);
    *body = NULL;
    *length = 0;
    *status = 200;
    const char *failure = getenv("GEOME_TEST_FAILURE");
    if (failure != NULL) {
        (void)snprintf(error, error_size, "injected test transport failure");
        return atoi(failure);
    }
    const char *fixture = getenv("GEOME_TEST_FIXTURE");
    assert(fixture != NULL);
    *body = read_fixture(fixture, length);
    return GEOME_OK;
}
