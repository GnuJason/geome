#include "geome.h"
#include "exit_codes.h"

#include <getopt.h>
#include <stdio.h>

int cli_parse(int argc, char **argv, CliConfig *config,
              char *error, size_t error_size)
{
    static const struct option options[] = {
        {"city", no_argument, NULL, 'c'},
        {"coords", no_argument, NULL, 256},
        {"json", no_argument, NULL, 'j'},
        {"full", no_argument, NULL, 'f'},
        {"help", no_argument, NULL, 'h'},
        {"version", no_argument, NULL, 'V'},
        {NULL, 0, NULL, 0}
    };
    *config = (CliConfig){MODE_DEFAULT, false, false};
    opterr = 0;
    optind = 0;
    bool selected = false;
    int option;
    const char *reason = NULL;
    while ((option = getopt_long(argc, argv, "+cjfhV", options, NULL)) != -1) {
        OutputMode mode;
        switch (option) {
        case 'c': mode = MODE_CITY; break;
        case 256: mode = MODE_COORDS; break;
        case 'j': mode = MODE_JSON; break;
        case 'f': mode = MODE_FULL; break;
        case 'h': config->help = true; continue;
        case 'V': config->version = true; continue;
        default: reason = "unknown option or unexpected option argument"; goto invalid;
        }
        if (selected && config->mode != mode) {
            reason = "output modes are mutually exclusive";
            goto invalid;
        }
        selected = true;
        config->mode = mode;
    }
    if (optind != argc) {
        reason = "positional arguments are not accepted";
        goto invalid;
    }
    return GEOME_OK;
invalid:
    if (error_size != 0) {
        (void)snprintf(error, error_size, "%s; run --help for usage", reason);
    }
    return GEOME_USAGE;
}
