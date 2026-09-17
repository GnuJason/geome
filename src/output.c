#include "output.h"
#include "exit_codes.h"

#include <cjson/cJSON.h>

static int finish(FILE *stream)
{
    return fflush(stream) == EOF || ferror(stream) ? GEOME_OUTPUT : GEOME_OK;
}

int output_default(FILE *stream, const Location *location)
{
    const char *parts[] = {
        location->city,
        location->region_code != NULL ? location->region_code : location->region,
        location->country_code != NULL ? location->country_code : location->country
    };
    bool written = false;
    for (size_t index = 0; index < sizeof(parts) / sizeof(parts[0]); ++index) {
        if (parts[index] != NULL) {
            if (fprintf(stream, "%s%s", written ? ", " : "", parts[index]) < 0) {
                return GEOME_OUTPUT;
            }
            written = true;
        }
    }
    if (!written) {
        return GEOME_DATA;
    }
    if (fputc('\n', stream) == EOF) {
        return GEOME_OUTPUT;
    }
    return finish(stream);
}

int output_city(FILE *stream, const Location *location)
{
    if (location->city == NULL) {
        return GEOME_DATA;
    }
    if (fprintf(stream, "%s\n", location->city) < 0) {
        return GEOME_OUTPUT;
    }
    return finish(stream);
}

int output_coords(FILE *stream, const Location *location)
{
    if (!location->has_latitude || !location->has_longitude) {
        return GEOME_DATA;
    }
    if (fprintf(stream, "%.6f,%.6f\n", location->latitude, location->longitude) < 0) {
        return GEOME_OUTPUT;
    }
    return finish(stream);
}

int output_json(FILE *stream, const Location *location)
{
    cJSON *root = cJSON_CreateObject();
    if (root == NULL) {
        return GEOME_INTERNAL;
    }
    struct string_field {
        const char *key;
        const char *value;
    } fields[] = {
        {"city", location->city}, {"region", location->region},
        {"region_code", location->region_code}, {"country", location->country},
        {"country_code", location->country_code}, {"ip", location->ip},
        {"isp", location->isp}
    };
    for (size_t index = 0; index < sizeof(fields) / sizeof(fields[0]); ++index) {
        cJSON *item = fields[index].value != NULL ?
            cJSON_AddStringToObject(root, fields[index].key, fields[index].value) :
            cJSON_AddNullToObject(root, fields[index].key);
        if (item == NULL) {
            cJSON_Delete(root);
            return GEOME_INTERNAL;
        }
    }
    cJSON *latitude = location->has_latitude ?
        cJSON_AddNumberToObject(root, "lat", location->latitude) :
        cJSON_AddNullToObject(root, "lat");
    cJSON *longitude = location->has_longitude ?
        cJSON_AddNumberToObject(root, "lon", location->longitude) :
        cJSON_AddNullToObject(root, "lon");
    if (latitude == NULL || longitude == NULL) {
        cJSON_Delete(root);
        return GEOME_INTERNAL;
    }
    char *text = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (text == NULL) {
        return GEOME_INTERNAL;
    }
    int result = fprintf(stream, "%s\n", text) < 0 ? GEOME_OUTPUT : finish(stream);
    cJSON_free(text);
    return result;
}

static const char *known(const char *text)
{
    return text != NULL ? text : "Unknown";
}

int output_full(FILE *stream, const Location *location)
{
    if (fprintf(stream, "City: %s\nRegion: %s\nRegion code: %s\nCountry: %s\nCountry code: %s\n",
                known(location->city), known(location->region), known(location->region_code),
                known(location->country), known(location->country_code)) < 0) {
        return GEOME_OUTPUT;
    }
    if (location->has_latitude) {
        if (fprintf(stream, "Latitude: %.6f\n", location->latitude) < 0) {
            return GEOME_OUTPUT;
        }
    } else if (fputs("Latitude: Unknown\n", stream) == EOF) {
        return GEOME_OUTPUT;
    }
    if (location->has_longitude) {
        if (fprintf(stream, "Longitude: %.6f\n", location->longitude) < 0) {
            return GEOME_OUTPUT;
        }
    } else if (fputs("Longitude: Unknown\n", stream) == EOF) {
        return GEOME_OUTPUT;
    }
    if (fprintf(stream, "IP: %s\nISP: %s\n", known(location->ip), known(location->isp)) < 0) {
        return GEOME_OUTPUT;
    }
    return finish(stream);
}

int output_help(FILE *stream)
{
    if (fputs(
        GEOME_PROGRAM " - approximate public IP-based location\n"
        "Usage: " GEOME_PROGRAM " [OPTION]\n"
        "  -c, --city     Print only the city\n"
        "      --coords   Print latitude,longitude (six fractional digits)\n"
        "  -j, --json     Print a stable JSON object\n"
        "  -f, --full     Print all location fields\n"
        "  -h, --help     Show this help without network access\n"
        "  -V, --version  Show version without network access\n"
        "With no mode, print city, region code, country code. Modes are exclusive.\n"
        "Results are approximate and based on the public IP, not GPS.\n"
        "VPNs, proxies, corporate gateways, Tor, mobile routing, and ISP address\n"
        "registration can produce a different location. Never use this for emergency\n"
        "response, safety decisions, legal residency, authentication, or precise tracking.\n"
        "Privacy: a request is sent to https://ipwho.is/ from your public IP.\n"
        "The provider receives that IP. This program does not retain, log, or\n"
        "transmit the result elsewhere. Your selected output is written to stdout.\n"
        "Exit statuses: 0 success; 2 usage; 3 network/DNS/TLS/timeout; 4 service/HTTP;\n"
        "5 location data; 6 memory/internal; 7 output/write failure.\n"
        "See " GEOME_PROGRAM "(1) for details.\n", stream) == EOF) {
        return GEOME_OUTPUT;
    }
    return finish(stream);
}

int output_version(FILE *stream)
{
    if (fputs(GEOME_PROGRAM " " GEOME_VERSION "\n", stream) == EOF) {
        return GEOME_OUTPUT;
    }
    return finish(stream);
}
