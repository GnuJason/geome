#ifndef GEOME_H
#define GEOME_H

#include <stdbool.h>
#include <stddef.h>

#define GEOME_ENDPOINT "https://ipwho.is/"

typedef enum {
    MODE_DEFAULT, MODE_CITY, MODE_COORDS, MODE_JSON, MODE_FULL
} OutputMode;

typedef struct {
    OutputMode mode;
    bool help;
    bool version;
} CliConfig;

int cli_parse(int argc, char **argv, CliConfig *config,
              char *error, size_t error_size);

#endif
