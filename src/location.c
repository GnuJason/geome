#include "location.h"
#include "exit_codes.h"

#include <cjson/cJSON.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void location_init(Location *location)
{
    *location = (Location){0};
}

void location_free(Location *location)
{
    if (location == NULL) {
        return;
    }
    free(location->provider_message);
    free(location->ip);
    free(location->city);
    free(location->region);
    free(location->region_code);
    free(location->country);
    free(location->country_code);
    free(location->isp);
    location_init(location);
}

static bool allocation_failed;

static void *parse_allocate(size_t size)
{
    void *memory = malloc(size);
    if (memory == NULL) {
        allocation_failed = true;
    }
    return memory;
}

static int copy_field(const cJSON *object, const char *name, char **destination)
{
    const cJSON *field = cJSON_GetObjectItemCaseSensitive(object, name);
    if (field == NULL || cJSON_IsNull(field)) {
        return GEOME_OK;
    }
    if (!cJSON_IsString(field)) {
        return GEOME_DATA;
    }
    const char *text = field->valuestring;
    size_t length = strlen(text);
    bool meaningful = false;
    for (size_t index = 0; index < length; ++index) {
        unsigned char byte = (unsigned char)text[index];
        if (byte < 0x20 || byte == 0x7f) {
            return GEOME_DATA;
        }
        if (byte != ' ') {
            meaningful = true;
        }
    }
    if (!meaningful) {
        return GEOME_OK;
    }
    *destination = malloc(length + 1);
    if (*destination == NULL) {
        return GEOME_INTERNAL;
    }
    memcpy(*destination, text, length + 1);
    return GEOME_OK;
}

static int coordinate(const cJSON *root, const char *name, double bound,
                      double *value, bool *present)
{
    const cJSON *field = cJSON_GetObjectItemCaseSensitive(root, name);
    if (field == NULL || cJSON_IsNull(field)) {
        return GEOME_OK;
    }
    if (!cJSON_IsNumber(field) || !isfinite(field->valuedouble) ||
        field->valuedouble < -bound || field->valuedouble > bound) {
        return GEOME_DATA;
    }
    *value = field->valuedouble;
    *present = true;
    return GEOME_OK;
}

static bool valid_utf8(const unsigned char *text, size_t length)
{
    for (size_t index = 0; index < length; ++index) {
        unsigned char lead = text[index];
        if (lead == 0) {
            return false;
        }
        if (lead < 0x80) {
            continue;
        }
        size_t extra;
        unsigned int point;
        unsigned int minimum;
        if (lead >= 0xc2 && lead <= 0xdf) {
            extra = 1; point = lead & 0x1fu; minimum = 0x80;
        } else if (lead >= 0xe0 && lead <= 0xef) {
            extra = 2; point = lead & 0x0fu; minimum = 0x800;
        } else if (lead >= 0xf0 && lead <= 0xf4) {
            extra = 3; point = lead & 0x07u; minimum = 0x10000;
        } else {
            return false;
        }
        if (extra >= length - index) {
            return false;
        }
        for (size_t offset = 0; offset < extra; ++offset) {
            unsigned char next = text[++index];
            if ((next & 0xc0u) != 0x80u) {
                return false;
            }
            point = (point << 6) | (next & 0x3fu);
        }
        if (point < minimum || point > 0x10ffff ||
            (point >= 0xd800 && point <= 0xdfff)) {
            return false;
        }
    }
    return true;
}

int location_parse_json(const char *json, size_t json_length,
                        Location *location, char *error, size_t error_size)
{
    Location parsed;
    location_init(&parsed);
    location_free(location);
    int result = GEOME_DATA;
    const char *reason = "invalid location response";
    cJSON *root = NULL;
    if (json == NULL || json_length == 0 || json_length > GEOME_RESPONSE_LIMIT ||
        !valid_utf8((const unsigned char *)json, json_length)) {
        reason = "empty, oversized, or invalid UTF-8 location response";
        goto done;
    }
    for (size_t index = 0; index < json_length; ++index) {
        if (json[index] == '\\') {
            if (json_length - index >= 6 &&
                memcmp(json + index, "\\u0000", 6) == 0) {
                reason = "NUL characters are not permitted in location data";
                goto done;
            }
            ++index;
        }
    }
    allocation_failed = false;
    cJSON_Hooks hooks = {parse_allocate, free};
    cJSON_InitHooks(&hooks);
    const char *end = NULL;
    root = cJSON_ParseWithLengthOpts(json, json_length, &end, 0);
    cJSON_InitHooks(NULL);
    if (root == NULL) {
        result = allocation_failed ? GEOME_INTERNAL : GEOME_DATA;
        reason = allocation_failed ? "out of memory parsing location" :
                                    "malformed or truncated JSON response";
        goto done;
    }
    while (end < json + json_length &&
           (*end == ' ' || *end == '\n' || *end == '\r' || *end == '\t')) {
        ++end;
    }
    if (end != json + json_length || !cJSON_IsObject(root)) {
        reason = "expected exactly one JSON object";
        goto done;
    }
    const cJSON *success = cJSON_GetObjectItemCaseSensitive(root, "success");
    if (!cJSON_IsBool(success)) {
        reason = "location response requires boolean success";
        goto done;
    }
    result = copy_field(root, "message", &parsed.provider_message);
    if (result != GEOME_OK) {
        reason = "invalid provider message or out of memory";
        goto done;
    }
    if (!cJSON_IsTrue(success)) {
        result = GEOME_SERVICE;
        reason = parsed.provider_message != NULL ? parsed.provider_message :
                 "location service rejected the request";
        goto done;
    }
    struct field_mapping {
        const char *name;
        char **target;
    } fields[] = {
        {"ip", &parsed.ip}, {"city", &parsed.city},
        {"region", &parsed.region}, {"region_code", &parsed.region_code},
        {"country", &parsed.country}, {"country_code", &parsed.country_code}
    };
    for (size_t index = 0; index < sizeof(fields) / sizeof(fields[0]); ++index) {
        result = copy_field(root, fields[index].name, fields[index].target);
        if (result != GEOME_OK) {
            reason = "invalid location string or out of memory";
            goto done;
        }
    }
    const cJSON *connection = cJSON_GetObjectItemCaseSensitive(root, "connection");
    if (connection != NULL && !cJSON_IsNull(connection)) {
        if (!cJSON_IsObject(connection)) {
            result = GEOME_DATA;
            reason = "connection must be an object";
            goto done;
        }
        result = copy_field(connection, "isp", &parsed.isp);
        if (result != GEOME_OK) {
            reason = "invalid ISP string or out of memory";
            goto done;
        }
    }
    result = coordinate(root, "latitude", 90, &parsed.latitude, &parsed.has_latitude);
    if (result == GEOME_OK) {
        result = coordinate(root, "longitude", 180, &parsed.longitude,
                            &parsed.has_longitude);
    }
    if (result != GEOME_OK) {
        reason = "coordinates must be finite numbers within geographic bounds";
        goto done;
    }
    if (parsed.city == NULL && parsed.region == NULL && parsed.region_code == NULL &&
        parsed.country == NULL && parsed.country_code == NULL) {
        result = GEOME_DATA;
        reason = "response contains no meaningful location identifiers";
        goto done;
    }
    parsed.success = true;
done:
    if (result != GEOME_OK && error_size != 0) {
        (void)snprintf(error, error_size, "%s", reason);
    }
    cJSON_Delete(root);
    if (result == GEOME_OK || result == GEOME_SERVICE) {
        *location = parsed;
    } else {
        location_free(&parsed);
    }
    return result;
}
