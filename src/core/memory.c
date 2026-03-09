#include "core/memory.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

bool Memory_Init() {
    return true;
}

void Memory_Shutdown() {
}

void *Memory_Allocate(size_t size) {
    void *ptr = malloc(size);
    memset(ptr, 0, size);
    return ptr;
}

void *Memory_Reallocate(void *ptr, size_t size) {
    void *result = realloc(ptr, size);
    return result;
}

void *Memory_AllocateArray(unsigned long count, size_t size) {
    void *ptr = calloc(count, size);
    return ptr;
}

void *Memory_ReallocateArray(void *ptr, unsigned long count, size_t size) {
    void *result = realloc(ptr, count * size);
    return result;
}

void Memory_Free(void *ptr) {
    assert(ptr != NULL);
    free(ptr);
}
