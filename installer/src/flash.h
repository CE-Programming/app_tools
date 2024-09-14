#include <stddef.h>
#include <stdint.h>

#define ADDR_TO_PAGE(addr) ((uint24_t)(addr) >> 16)
#define PAGE_TO_ADDR(page) ((void*)((page) << 16))

void flash_erase(uint8_t page);
void flash_write(void *dst, const void *src, size_t len);