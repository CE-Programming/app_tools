#include <stddef.h>

void *os_FindAppStart(const char *name);
void *find_last_app();
void fix_relocations(void *app);
bool confirm_delete_vars(void);
void delete_var(const char *name, uint8_t type);
