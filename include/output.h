#ifndef GEOME_OUTPUT_H
#define GEOME_OUTPUT_H

#include "location.h"
#include <stdio.h>

int output_default(FILE *stream, const Location *location);
int output_city(FILE *stream, const Location *location);
int output_coords(FILE *stream, const Location *location);
int output_json(FILE *stream, const Location *location);
int output_full(FILE *stream, const Location *location);
int output_help(FILE *stream);
int output_version(FILE *stream);

#endif
