#include <stdint.h>


struct segment {
    uint8_t type;
    char file[9];
    uint16_t file_offset;
    size_t app_offset;
    size_t size;
    // todo: add checksum to this?
    void *src;
};

extern const char app_name[];
extern struct segment segments[];
