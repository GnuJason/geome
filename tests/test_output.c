#define _GNU_SOURCE
#include "output.h"
#include "exit_codes.h"
#include "test_support.h"
#include "fault_alloc.h"

#include <cjson/cJSON.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>

typedef int (*Formatter)(FILE *, const Location *);

static char *render(Formatter formatter, const Location *location, int expected)
{
    char *text = NULL;
    size_t length = 0;
    FILE *stream = open_memstream(&text, &length);
    assert(stream != NULL);
    assert(formatter(stream, location) == expected);
    assert(fclose(stream) == 0);
    if (expected == GEOME_OK) {
        assert(length > 0 && text[length - 1] == '\n');
        assert(length == 1 || text[length - 2] != '\n');
    } else {
        assert(length == 0);
    }
    return text;
}

static void expect(Formatter formatter, const Location *location, const char *expected)
{
    char *text = render(formatter, location, GEOME_OK);
    assert(strcmp(text, expected) == 0);
    free(text);
}

int main(void)
{
    Location location;
    location_init(&location);
    char error[256];
    size_t length;
    char *fixture = read_fixture("tests/fixtures/success.json", &length);
    assert(location_parse_json(fixture, length, &location, error, sizeof(error)) == GEOME_OK);
    free(fixture);
    expect(output_default, &location, "Charlotte, NC, US\n");
    expect(output_city, &location, "Charlotte\n");
    expect(output_coords, &location, "35.227100,-80.843100\n");
    expect(output_full, &location,
           "City: Charlotte\nRegion: North Carolina\nRegion code: NC\n"
           "Country: United States\nCountry code: US\nLatitude: 35.227100\n"
           "Longitude: -80.843100\nIP: 198.51.100.10\nISP: Example ISP\n");
    char *text = render(output_json, &location, GEOME_OK);
    cJSON *json = cJSON_Parse(text);
    assert(json != NULL && cJSON_GetArraySize(json) == 9);
    assert(strcmp(cJSON_GetObjectItemCaseSensitive(json, "city")->valuestring, "Charlotte") == 0);
    assert(cJSON_GetObjectItemCaseSensitive(json, "lon")->valuedouble == location.longitude);
    cJSON_Delete(json);
    free(text);
    free(location.region_code); location.region_code = NULL;
    expect(output_default, &location, "Charlotte, North Carolina, US\n");
    free(location.country_code); location.country_code = NULL;
    expect(output_default, &location, "Charlotte, North Carolina, United States\n");
    free(location.city); location.city = NULL;
    expect(output_default, &location, "North Carolina, United States\n");
    text = render(output_city, &location, GEOME_DATA); free(text);
    location_free(&location);
    const char *escaped = "{\"success\":true,\"city\":\"Quote \\\" and \\\\ slash\",\"latitude\":0,\"longitude\":0}";
    assert(location_parse_json(escaped, strlen(escaped), &location, error, sizeof(error)) == GEOME_OK);
    expect(output_coords, &location, "0.000000,0.000000\n");
    text = render(output_json, &location, GEOME_OK);
    json = cJSON_Parse(text);
    assert(json != NULL);
    assert(strcmp(cJSON_GetObjectItemCaseSensitive(json, "city")->valuestring, location.city) == 0);
    assert(cJSON_IsNull(cJSON_GetObjectItemCaseSensitive(json, "isp")));
    assert(cJSON_IsNumber(cJSON_GetObjectItemCaseSensitive(json, "lat")));
    cJSON_Delete(json); free(text);
    location_free(&location);
    const char *minimal = "{\"success\":true,\"country_code\":\"US\"}";
    assert(location_parse_json(minimal, strlen(minimal), &location, error, sizeof(error)) == GEOME_OK);
    expect(output_default, &location, "US\n");
    expect(output_full, &location, "City: Unknown\nRegion: Unknown\nRegion code: Unknown\n"
           "Country: Unknown\nCountry code: US\nLatitude: Unknown\nLongitude: Unknown\n"
           "IP: Unknown\nISP: Unknown\n");
    text = render(output_coords, &location, GEOME_DATA); free(text);
    text = render(output_json, &location, GEOME_OK);
    json = cJSON_Parse(text);
    assert(json != NULL && cJSON_GetArraySize(json) == 9);
    const char *null_keys[] = {"city", "region", "region_code", "country", "lat", "lon", "ip", "isp"};
    for (size_t index = 0; index < sizeof(null_keys) / sizeof(null_keys[0]); ++index) {
        assert(cJSON_IsNull(cJSON_GetObjectItemCaseSensitive(json, null_keys[index])));
    }
    cJSON_Delete(json); free(text);
    Formatter formatters[] = {output_default, output_city, output_coords, output_json, output_full};
    location_free(&location);
    fixture = read_fixture("tests/fixtures/success.json", &length);
    assert(location_parse_json(fixture, length, &location, error, sizeof(error)) == GEOME_OK);
    free(fixture);
    for (size_t index = 0; index < sizeof(formatters) / sizeof(formatters[0]); ++index) {
        FILE *full = fopen("/dev/full", "w");
        assert(full != NULL);
        assert(formatters[index](full, &location) == GEOME_OUTPUT);
        (void)fclose(full);
    }
    assert(signal(SIGPIPE, SIG_IGN) != SIG_ERR);
    int descriptors[2];
    assert(pipe(descriptors) == 0);
    assert(close(descriptors[0]) == 0);
    FILE *broken = fdopen(descriptors[1], "w");
    assert(broken != NULL);
    assert(output_json(broken, &location) == GEOME_OUTPUT);
    (void)fclose(broken);
    bool reached_success = false;
    for (long budget = 0; budget < 100; ++budget) {
        FILE *stream = fopen("/dev/null", "w");
        assert(stream != NULL);
        cJSON_Hooks hooks = {__wrap_malloc, free};
        cJSON_InitHooks(&hooks);
        fault_after = budget;
        int status = output_json(stream, &location);
        fault_after = -1;
        cJSON_InitHooks(NULL);
        assert(fclose(stream) == 0);
        assert(status == GEOME_INTERNAL || status == GEOME_OK);
        if (status == GEOME_OK) { reached_success = true; break; }
    }
    assert(reached_success);
    location_free(&location);
    puts("output tests passed");
    return 0;
}
