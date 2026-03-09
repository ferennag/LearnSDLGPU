#pragma once

#include <stddef.h>

bool Memory_Init();

void Memory_Shutdown();

void *Memory_Allocate(size_t size);

void *Memory_Reallocate(void *ptr, size_t size);

void *Memory_AllocateArray(unsigned long count, size_t size);

void *Memory_ReallocateArray(void *ptr, unsigned long count, size_t size);

void Memory_Free(void *ptr);
