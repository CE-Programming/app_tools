#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <debug.h>
#include <ti/vars.h>
#include <ti/screen.h>
#include <ti/getcsc.h>

#include "app.h"
#include "flash.h"
#include "ports.h"

#define MAX_APPVARS 99
#define MAX_DESCRIPTION 64

#define APPS_START ((uint8_t *)0x3b0000)
#define APP_VALID 0x81
#define APP_NAME_OFFSET (256 + 3)
#define APP_CODE_OFFSET 0x112

enum
{
    SUCCESS,
    CANCELLED,
    MISSING_VAR,
    PORT_SETUP_FAILED,
    NO_SPACE,
};

struct appvar
{
    uint8_t *data;
    uint24_t size;
    uintptr_t app_offset;
};

static struct appvar appvars[MAX_APPVARS];
static struct appvar *last_appvar;
static uint24_t app_size;
static char app_name[10];

static int load_appvars(void)
{
    struct appvar *appvar = &appvars[0];

    app_size = 0;

    for (uint8_t i = 0; i < MAX_APPVARS; ++i)
    {
        char name[10];
        void *data;
        const int namelen = sprintf(name, APPVAR_PREFIX "%u", i);

        if (!os_ChkFindSym(OS_TYPE_APPVAR, name, NULL, &data))
            return MISSING_VAR;

        uint8_t *d = data;

        if (d < os_RamStart)
            d += 10 + namelen;

        appvar->size = *(uint16_t*)d;
        appvar->data = d + 2;
        appvar->app_offset = app_size;

        /* first appvar contains app name */
        if (i == 0)
        {
            strncpy(app_name, (const char *)(appvar->data + APP_NAME_OFFSET), sizeof app_name - 1);
            dbg_printf("App %s\n", app_name);
        }

        dbg_printf("AppVar %u (%s, %u bytes)\n", i, name, (uint24_t)appvar->size);

        app_size += appvar->size;

        if (appvar->size != APPVAR_SPLIT_SIZE)
            break;

        appvar++;
    }

    last_appvar = appvar;

    return SUCCESS;
}

/* Returns the start of the installed app with the given name, skipping deleted apps. */
static uint8_t *find_app(const char *name)
{
    uint8_t *app = APPS_START;

    for (;;)
    {
        const uint24_t size = *(uint24_t *)(app - sizeof(uint24_t));

        if (size == 0xFFFFFF)
            return NULL;

        app -= sizeof(uint24_t) + size;

        if (*app == APP_VALID && strncmp((const char *)app + APP_NAME_OFFSET, name, 8) == 0)
            return app;
    }
}

static const uint8_t *installed_app;

static uint8_t installed_byte(uint24_t offset)
{
    return installed_app[offset];
}

static uint8_t new_byte(uint24_t offset)
{
    for (struct appvar *appvar = &appvars[0]; appvar <= last_appvar; appvar++)
    {
        if (offset < appvar->app_offset + appvar->size)
            return appvar->data[offset - appvar->app_offset];
    }

    return 0;
}

/* Copies the description that follows the C app header, or an empty string if there is none. */
static void read_description(char *description, uint8_t (*byte)(uint24_t))
{
    uint24_t code = 256 + (byte(APP_CODE_OFFSET) | byte(APP_CODE_OFFSET + 1) << 8 | (uint24_t)byte(APP_CODE_OFFSET + 2) << 16);
    uint24_t offset = code + 6;

    description[0] = '\0';

    if (byte(code) != 0x00 || byte(code + 1) != 0xC3)
        return;

    switch (byte(code + 5))
    {
        case 1:
            offset += 2 + byte(offset) * byte(offset + 1);
            break;
        case 2:
            break;
        default:
            return;
    }

    for (uint8_t i = 0; i < MAX_DESCRIPTION - 1; ++i)
    {
        if (!(description[i] = byte(offset + i)))
            return;
    }

    description[MAX_DESCRIPTION - 1] = '\0';
}

static bool check_variable_overlaps(const uint8_t *app_start)
{
    uint8_t first_erased_page = ADDR_TO_PAGE(app_start) - 1;

    void *entry = os_GetSymTablePtr();
    uint24_t type;
    uint24_t length;
    char name[9];
    void *data;

    while ((entry = os_NextSymEntry(entry, &type, &length, name, &data)) != NULL)
    {
        if (data >= (void*)os_RamStart)
        {
            continue;
        }

        const uint8_t page = ADDR_TO_PAGE(data);
        if (page < first_erased_page)
        {
            continue;
        }

        bool ours = false;
        for (uint8_t i = 0; i < MAX_APPVARS; ++i)
        {
            char appvar_name[10];

            sprintf(appvar_name, APPVAR_PREFIX "%u", i);

            if (strncmp(name, appvar_name, length) == 0)
            {
                const uint24_t addr = (uintptr_t)app_start + (APPVAR_SPLIT_SIZE * i);

                if (page >= ADDR_TO_PAGE(addr))
                {
                    dbg_printf("Variable %s (page 0x%x) would be erased before being copied to 0x%x\n",
                        name, page, addr);

                    return false;
                }
                ours = true;
                break;
            }
        }

        if (!ours)
        {
            dbg_printf("Non-installer variable %s @ %p overlaps app\n", name, data);
            return false;
        }
    }

    return true;
}

static void *pmax(void *a, void *b)
{
    return a > b ? a : b;
}

static void *pmin(void *a, void *b)
{
    return a < b ? a : b;
}

static int install(uint8_t *replace)
{
    int error = load_appvars();

    if (error)
        return error;

    if (port_setup())
    {
        return PORT_SETUP_FAILED;
    }

    struct appvar *appvar = last_appvar;

    /* size at end of application stored in flash */
    appvar->size += sizeof(uint24_t);

    uint8_t *app_end = find_last_app();
    uint8_t *app_start = app_end - app_size - sizeof(uint24_t);

    dbg_printf("App from %p to %p\n", app_start, app_end);

    if (!check_variable_overlaps(app_start))
    {
        return NO_SPACE;
    }

    if (replace)
    {
        dbg_printf("Deleting old app at %p\n", replace);
        delete_app(replace);
    }

    os_ClrHome();
    os_PutStrFull("Installing: ");
    os_PutStrFull(app_name);
    os_NewLine();
    os_PutStrFull("Please wait...");
    os_NewLine();
    os_NewLine();
    os_PutStrFull("This may take a while.");
    os_NewLine();
    os_PutStrFull("Do not reset!");

    void *written = app_end;
    bool wrote_size = false;

    for (uint8_t page = ADDR_TO_PAGE(app_end); page >= ADDR_TO_PAGE(app_start) && appvar >= &appvars[0]; page--)
    {
        while (written > PAGE_TO_ADDR(page) && appvar >= &appvars[0])
        {
            uint8_t *seg_start = app_start + appvar->app_offset;
            uint8_t *dst_end = pmin(seg_start + appvar->size, PAGE_TO_ADDR(page + 1));
            uint8_t *src_start = appvar->data;
            uint8_t *dst_start = pmax(seg_start, PAGE_TO_ADDR(page));

            src_start += dst_start - seg_start;

            dbg_printf("Writing segment %u (%p-%p) %p-%p -> (page %02x) %p-%p\n",
                appvar - appvars,
                appvar->data,
                appvar->data + appvar->size,
                src_start,
                src_start + (dst_end - dst_start),
                page,
                dst_start,
                dst_end);

            if (!wrote_size)
            {
                flash_write(dst_start, src_start, (dst_end - dst_start) - sizeof(uint24_t));
                flash_write(dst_end - sizeof(uint24_t), &app_size, sizeof(uint24_t));
                wrote_size = true;
            }
            else
            {
                flash_write(dst_start, src_start, (dst_end - dst_start));
            }

            dbg_printf("Written\n");

            written = dst_start;
            if (dst_start == seg_start)
            {
                appvar--;
            }
        }

        dbg_printf("Erasing page %02x\n", page - 1);
        flash_erase(page - 1);
        dbg_printf("Erased\n");
    }

    dbg_printf("Copied app data\n");

    fix_relocations(app_start);

    dbg_printf("Fixed relocations");

    return SUCCESS;
}

void delete_vars(void)
{
    for (uint8_t i = 0; i < MAX_APPVARS; ++i)
    {
        char name[10];

        sprintf(name, APPVAR_PREFIX "%u", i);

        if (os_ChkFindSym(OS_TYPE_APPVAR, name, NULL, NULL))
        {
            delete_var(name, OS_TYPE_APPVAR);
        }
        else
        {
            break;
        }
    }
}

int main(int argc, char **argv)
{
    int error;
    uint8_t *installed = NULL;

    (void)argc;

    os_ClrHome();

    error = load_appvars();

    if (!error && (installed = find_app(app_name)))
    {
		char description[MAX_DESCRIPTION];

		os_PutStrFull(app_name);
		os_PutStrFull(" is installed.");
		os_NewLine();
		os_NewLine();

		installed_app = installed;
		read_description(description, installed_byte);
		os_PutStrFull("Old: ");
		os_PutStrFull(description);
		os_NewLine();

		read_description(description, new_byte);
		os_PutStrFull("New: ");
		os_PutStrFull(description);
		os_NewLine();
		os_NewLine();

		os_PutStrFull("Replace it?");
		os_NewLine();

        if (!confirm())
        {
            os_ClrHome();
            return CANCELLED;
        }
    }

    if (!error)
        error = install(installed);

    if (error == NO_SPACE)
    {
        if (installed)
            delete_app(installed);

        os_ClrHome();
		os_PutStrFull("Not enough free archive.");
		os_NewLine();
		os_PutStrFull("Garbage collecting may");
		os_NewLine();
		os_PutStrFull("free enough space.");
		os_NewLine();
		os_NewLine();
		os_PutStrFull("Press any key.");

		while (os_GetCSC());
		while (!os_GetCSC());

        garbage_collect();
        error = install(NULL);
    }

    os_ClrHome();

    switch (error)
    {
        case SUCCESS:
            os_PutStrFull(installed ? "Successfully updated." : "Successfully installed.");
            os_NewLine();
            os_PutStrFull("App: ");
            os_PutStrFull(app_name);
            os_NewLine();
            os_NewLine();
            os_PutStrFull("Delete installer files?");
            os_NewLine();
            if (confirm())
            {
                delete_var(argv[0], OS_TYPE_PROT_PRGM);
                delete_vars();
            }
            return SUCCESS;
            break;

        case MISSING_VAR:
            os_PutStrFull("Install failed.");
            os_NewLine();
            os_PutStrFull("Missing an appvar.");
            break;

        case PORT_SETUP_FAILED:
            os_PutStrFull("Install failed.");
            os_NewLine();
            os_PutStrFull("Unsupported OS version.");
            break;

        case NO_SPACE:
            os_PutStrFull("Install failed.");
            os_NewLine();
            os_PutStrFull("Out of archive space.");
            if (installed)
            {
                os_NewLine();
                os_NewLine();
                os_PutStrFull("The old version was");
                os_NewLine();
                os_PutStrFull("deleted. Free some space");
                os_NewLine();
                os_PutStrFull("and run the installer");
                os_NewLine();
                os_PutStrFull("again.");
            }
            break;
    }

    while (os_GetCSC());
    while (!os_GetCSC());

    return error;
}
