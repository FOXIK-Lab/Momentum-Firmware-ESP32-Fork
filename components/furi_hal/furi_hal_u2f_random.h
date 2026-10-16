#pragma once
#include <stddef.h>
void furi_hal_u2f_random_init(void);
int furi_hal_u2f_random_bytes(void* context, unsigned char* output, size_t length);
