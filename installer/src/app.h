#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

void *find_last_app();
void fix_relocations(void *app);
bool confirm(void);
void delete_var(const char *name, uint8_t type);
void delete_app(void *app);
void garbage_collect(void);
