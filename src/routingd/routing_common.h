#ifndef ROUTING_COMMON_H
#define ROUTING_COMMON_H

#include <sys/types.h>

extern int debug_flag; // Global variable that represents if the demon runs in debug-mode.

void global_debug(const char *format, ...);

#endif //ROUTING_COMMON_H
