#include "exit_codes.h"
#include "location.h"
#include "test_support.h"
#include "fault_alloc.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    Location location;
    location_init(&location);
    char error[256];
    const char *zero = "{\"success\":true,\"city\":\"Null Island\","
                       "\"latitude\":0,\"longitude\":0}";
    assert(location_parse_json(zero, strlen(zero), &location, error, sizeof(error)) == GEOME_OK);
    assert(location.has_latitude && location.has_longitude);
    assert(location.latitude == 0 && location.longitude == 0);
    const char *invalid = "{\"success\":true,\"city\":\"City\",\"latitude\":91}";
    assert(location_parse_json(invalid, strlen(invalid), &location, error, sizeof(error)) == GEOME_DATA);
    assert(location.city == NULL && !location.success);
    location_free(&location);
    location_free(&location);
    struct parse_case {
        const char *json;
        int status;
    } cases[] = {
        {"{\"success\":true,\"country_code\":\"US\",\"latitude\":-90,\"longitude\":180}", GEOME_OK},
        {"{\"success\":true,\"city\":\"City\",\"latitude\":90,\"longitude\":-180}", GEOME_OK},
        {"{\"success\":true,\"city\":\"City\",\"latitude\":-91}", GEOME_DATA},
        {"{\"success\":true,\"city\":\"City\",\"longitude\":181}", GEOME_DATA},
        {"{\"success\":true,\"city\":\"City\",\"longitude\":-181}", GEOME_DATA},
        {"{\"success\":true,\"city\":\"City\",\"longitude\":1e999}", GEOME_DATA},
        {"{\"success\":true,\"city\":\"City\",\"latitude\":\"0\"}", GEOME_DATA},
        {"{\"success\":true,\"city\":\"City\",\"longitude\":false}", GEOME_DATA},
        {"{\"success\":true,\"city\":3}", GEOME_DATA},
        {"{\"success\":true,\"city\":\"City\",\"connection\":[]}", GEOME_DATA},
        {"{\"success\":true,\"city\":\"City\",\"connection\":{\"isp\":5}}", GEOME_DATA},
        {"{\"success\":true,\"city\":\"City\",\"latitude\":null}", GEOME_OK},
        {"{\"success\":false}", GEOME_SERVICE},
        {"{\"success\":false,\"message\":9}", GEOME_DATA},
        {"{\"city\":\"City\"}", GEOME_DATA},
        {"{\"success\":1,\"city\":\"City\"}", GEOME_DATA},
        {"{\"success\":true,\"latitude\":0,\"longitude\":0}", GEOME_DATA},
        {"{\"success\":true,\"city\":\" \"}", GEOME_DATA},
        {"{\"success\":true,\"city\":null}", GEOME_DATA},
        {"{\"success\":true,\"city\":\"bad\\ncity\"}", GEOME_DATA},
        {"{\"success\":true,\"city\":\"bad\\u0000city\"}", GEOME_DATA},
        {"{\"success\":true,\"city\":\"\\\\u0000\"}", GEOME_OK},
        {"{\"success\":true,\"city\":\"\xc0\xaf\"}", GEOME_DATA},
        {"{\"success\":true,\"city\":\"Montr\xc3\xa9" "al\"}", GEOME_OK},
        {"{\"success\":true,\"city\":\"City\"} trailing", GEOME_DATA},
        {"[]", GEOME_DATA}, {"null", GEOME_DATA}, {"", GEOME_DATA},
        {"{", GEOME_DATA}
    };
    for (size_t index = 0; index < sizeof(cases) / sizeof(cases[0]); ++index) {
        assert(location_parse_json(cases[index].json, strlen(cases[index].json),
                                  &location, error, sizeof(error)) == cases[index].status);
        location_free(&location);
    }
    const char *fixtures[] = {"success", "success_zero_coords", "missing_city",
                             "missing_optional_fields", "api_failure", "malformed"};
    for (size_t index = 0; index < sizeof(fixtures) / sizeof(fixtures[0]); ++index) {
        char path[128];
        assert(snprintf(path, sizeof(path), "tests/fixtures/%s.json", fixtures[index]) > 0);
        size_t length;
        char *json = read_fixture(path, &length);
        int expected = index == 4 ? GEOME_SERVICE : index == 5 ? GEOME_DATA : GEOME_OK;
        assert(location_parse_json(json, length, &location, error, sizeof(error)) == expected);
        if (index == 0) {
            assert(strcmp(location.city, "Charlotte") == 0);
            assert(strcmp(location.isp, "Example ISP") == 0);
            assert(location.longitude < 0);
        } else if (index == 2) {
            assert(location.city == NULL);
        } else if (index == 3) {
            assert(location.isp == NULL && location.region_code == NULL);
            assert(!location.has_latitude && !location.has_longitude);
        } else if (index == 4) {
            assert(strcmp(location.provider_message, "Reserved IP address") == 0);
            assert(strcmp(error, "Reserved IP address") == 0);
        }
        free(json);
        location_free(&location);
    }
    char *large = malloc(GEOME_RESPONSE_LIMIT + 1);
    assert(large != NULL);
    memset(large, ' ', GEOME_RESPONSE_LIMIT + 1);
    assert(location_parse_json(large, GEOME_RESPONSE_LIMIT + 1, &location,
                               error, sizeof(error)) == GEOME_DATA);
    free(large);
    size_t length;
    char *json = read_fixture("tests/fixtures/success.json", &length);
    bool reached_success = false;
    for (long budget = 0; budget < 100; ++budget) {
        fault_after = budget;
        int status = location_parse_json(json, length, &location, error, sizeof(error));
        fault_after = -1;
        assert(status == GEOME_INTERNAL || status == GEOME_OK);
        location_free(&location);
        if (status == GEOME_OK) {
            reached_success = true;
            break;
        }
    }
    assert(reached_success);
    free(json);
    puts("location tests passed");
    return 0;
}
