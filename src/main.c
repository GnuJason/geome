#include "geome.h"
#include "exit_codes.h"
#include "http_client.h"
#include "location.h"
#include "output.h"

#include <curl/curl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    char error[512] = "internal failure";
    CliConfig config;
    Location location;
    location_init(&location);
    char *body = NULL;
    size_t body_size = 0;
    long http_status = 0;
    bool curl_initialized = false;
    int result = GEOME_INTERNAL;
    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
        (void)snprintf(error, sizeof(error), "could not configure SIGPIPE handling");
        goto done;
    }
    result = cli_parse(argc, argv, &config, error, sizeof(error));
    if (result != GEOME_OK) {
        goto done;
    }
    if (config.help || config.version) {
        result = config.help ? output_help(stdout) : output_version(stdout);
        goto done;
    }
    if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK) {
        result = GEOME_INTERNAL;
        (void)snprintf(error, sizeof(error), "could not initialize libcurl");
        goto done;
    }
    curl_initialized = true;
    result = http_get_json(GEOME_ENDPOINT, &body, &body_size, &http_status, error, sizeof(error));
    if (result != GEOME_OK) {
        goto done;
    }
    result = location_parse_json(body, body_size, &location, error, sizeof(error));
    if (result != GEOME_OK) {
        goto done;
    }
    switch (config.mode) {
    case MODE_DEFAULT: result = output_default(stdout, &location); break;
    case MODE_CITY:
        result = output_city(stdout, &location);
        (void)snprintf(error, sizeof(error), "provider returned no city; try the default mode or --json");
        break;
    case MODE_COORDS:
        result = output_coords(stdout, &location);
        (void)snprintf(error, sizeof(error), "provider returned incomplete coordinates; try --json");
        break;
    case MODE_JSON: result = output_json(stdout, &location); break;
    case MODE_FULL: result = output_full(stdout, &location); break;
    }
done:
    free(body);
    location_free(&location);
    if (curl_initialized) {
        curl_global_cleanup();
    }
    if (result != GEOME_OK) {
        const char *message = result == GEOME_OUTPUT ? "could not write output" :
                              result == GEOME_INTERNAL ? "local memory or internal failure" : error;
        if (fprintf(stderr, GEOME_PROGRAM ": %s\n", message) < 0 || fflush(stderr) == EOF) {
            return GEOME_OUTPUT;
        }
    }
    return result;
}
