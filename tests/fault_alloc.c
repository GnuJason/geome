#include "fault_alloc.h"

long fault_after = -1;

static int should_fail(void)
{
    if (fault_after < 0) {
        return 0;
    }
    if (fault_after == 0) {
        return 1;
    }
    --fault_after;
    return 0;
}

void *__wrap_malloc(size_t size)
{
    return should_fail() ? NULL : __real_malloc(size);
}

void *__wrap_realloc(void *memory, size_t size)
{
    return should_fail() ? NULL : __real_realloc(memory, size);
}
