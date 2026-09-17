#ifndef GEOME_LOCATION_H
#define GEOME_LOCATION_H

#include <stdbool.h>
#include <stddef.h>

#define GEOME_RESPONSE_LIMIT ((size_t)1024 * 1024)

typedef struct {
    bool success;
    char *provider_message;
    char *ip;
    char *city;
    char *region;
    char *region_code;
    char *country;
    char *country_code;
    char *isp;
    double latitude;
    double longitude;
    bool has_latitude;
    bool has_longitude;
} Location;

void location_init(Location *location);
void location_free(Location *location);
int location_parse_json(const char *json, size_t json_length,
                        Location *location, char *error, size_t error_size);

#endif
