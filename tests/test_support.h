#ifndef GEOME_TEST_SUPPORT_H
#define GEOME_TEST_SUPPORT_H

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

static inline char *read_fixture(const char *path, size_t *length)
{
    FILE *stream = fopen(path, "rb");
    assert(stream != NULL);
    assert(fseek(stream, 0, SEEK_END) == 0);
    long size = ftell(stream);
    assert(size >= 0);
    rewind(stream);
    *length = (size_t)size;
    char *data = malloc(*length + 1);
    assert(data != NULL);
    assert(fread(data, 1, *length, stream) == *length);
    data[*length] = '\0';
    assert(fclose(stream) == 0);
    return data;
}

#endif
