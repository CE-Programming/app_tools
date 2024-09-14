#include <stddef.h>

void *os_FindAppStart(const char *name);
void *find_last_app();
void fix_relocations(void *app);
