#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <debug.h>
#include <ti/vars.h>
#include <ti/screen.h>
#include <ti/getcsc.h>

#include "app.h"
#include "flash.h"
#include "ports.h"
#include "segments.h"

static bool check_variable_overlaps(const void *app_start) {
    // The OS needs a completely FF page between the archive and the first app, for some reason.
    uint8_t first_erased_page = ADDR_TO_PAGE(app_start) - 1;

    void *entry = os_GetSymTablePtr();
    uint24_t type;
    uint24_t nameLength;
    char name[9];
    void *data;
    while ((entry = os_NextSymEntry(entry, &type, &nameLength, name, &data)) != NULL) {
        if (data >= (void*)os_RamStart) {
            // Variables in RAM will not be erased, ignore it
            continue;
        }
        uint8_t page = ADDR_TO_PAGE(data);
        if (page < first_erased_page) {
            // This variable is on a page that will not be erased, ignore it
            continue;
        }
        bool ours = false;
        for (const struct segment *segment = segments; segment->type != 0; segment++) {
            if (strncmp(segment->file, name, nameLength) == 0) {
                if (page >= ADDR_TO_PAGE(app_start + segment->app_offset)) {
                    // This variable would be overwritten before it has a chance to be copied into its desination
                    dbg_printf("Variable %.09s (page 0x%x) would be erased before being copied to %p\n", name, page, app_start + segment->app_offset);
                    return false;
                }
                ours = true;
                break;
            }
        }
        if (!ours) {
            // This is not one of our variables, and would be overwritten
            dbg_printf("Non-installer variable %.09s @ %p overlaps app\n", name, data);
            return false;
        }
    }

    return true;
}

static void *pmax(void *a, void *b) {
    if (a > b) return a;
    else return b;
}

static void *pmin(void *a, void *b) {
    if (a < b) return a;
    else return b;
}

static bool install(void) {
    bool is_installed = os_FindAppStart(app_name) != NULL;

    if (is_installed) {
        // todo: option to delete + reinstall app from here
        os_PutStrFull("Already installed - delete app from Mem Mgmt menu to reinstall");
        return false;
    }

    if (port_setup()) {
        os_PutStrFull("Unsupported OS version");
        return false;
    }

    size_t total_size = 0;

    struct segment *segment;
    for (segment = segments; segment->type != 0; segment++) {
        total_size += segment->size;
        void *file_data;
        if (!os_ChkFindSym(segment->type, segment->file, NULL, &file_data)) {
            os_PutStrFull("Missing variable: ");
            os_PutStrFull(segment->file);
            return false;
        }
        if (file_data < (void*)os_RamStart) {
            file_data += 10 + strlen(segment->file);
        }
        uint16_t file_size = *(const uint16_t*) file_data;
        file_data += 2;
        dbg_printf("Segment %u (%.8s) var size %u (expected >= %u) at %p\n", segment - segments, segment->file, file_size, segment->size, file_data);
        if (file_size < segment->file_offset + segment->size) {
            os_PutStrFull("Invalid variable (too short): ");
            os_PutStrFull(segment->file);
            return false;
        }
        segment->src = file_data + segment->file_offset;
    }

    void *app_end = find_last_app();
    void *app_start = app_end - total_size;

    dbg_printf("App from %p to %p\n", app_start, app_end);

    if (!check_variable_overlaps(app_start)) {
        os_PutStrFull("No space, try deleting vars, then Garbage Collect");
        return false;
    }

    os_PutStrFull("Installing. This takes a  while, do not reset!");
    os_NewLine();

    void *written = app_end;

    segment--; // last segment
    for (uint8_t page = ADDR_TO_PAGE(app_end); page >= ADDR_TO_PAGE(app_start) && segment >= &segments[0]; page--) {
        while (written > PAGE_TO_ADDR(page) && segment >= &segments[0]) {
            void *seg_start = app_start + segment->app_offset;
            void *dst_end = pmin(seg_start + segment->size, PAGE_TO_ADDR(page + 1));
            void *src_start = segment->src;
            void *dst_start = pmax(seg_start, PAGE_TO_ADDR(page));
            src_start += dst_start - seg_start;
            dbg_printf("Writing segment %u (%8.8s) (%p-%p) %p-%p -> (page %02x) %p-%p\n", segment - segments, segment->file, segment->src, segment->src + segment->size, src_start, src_start + (dst_end - dst_start), page, dst_start, dst_end);
            flash_write(dst_start, src_start, (dst_end - dst_start));
            dbg_printf("Written\n");
            written = dst_start;
            if (dst_start == seg_start) {
                segment--;
            }
        }
        dbg_printf("Erasing page %02x\n", page - 1);
        flash_erase(page - 1);
        dbg_printf("Erased\n");
    }

    dbg_printf("Copied data segments\n");

    fix_relocations(app_start);

    dbg_printf("Fixed relocations");

    return true;
}


int main() {
    os_ClrHome();
    bool succeeded = install();
    if (!succeeded) {
        os_NewLine();
        os_PutStrFull("Installation failed");
    } else {
        os_PutStrFull("Installed - open from apps menu");
    }

    while (!os_GetCSC());
}
