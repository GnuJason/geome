#ifndef GEOME_FAULT_ALLOC_H
#define GEOME_FAULT_ALLOC_H

#include <stddef.h>

extern long fault_after;
void *__wrap_malloc(size_t size);
void *__wrap_realloc(void *memory, size_t size);
void *__real_malloc(size_t size);
void *__real_realloc(void *memory, size_t size);

#endif
