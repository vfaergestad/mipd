#include "routing_common.h"
#include <stdio.h>
#include <time.h>
#include <stdarg.h>

int debug_flag = 0; // Global variable that represents if the demon runs in debug-mode.


/**
 * Prints a debug message if the program is running in debug mode.
 * Accepts a format string and a variable number of arguments. Can therefore be used as printf.
 * Also prints a timestamp.
 * Uses the global variable debug_flag to check if debug-mode is active.
 *
 * @param format The message to print, accepts format string.
 *
 * @return void
 */
void global_debug(const char *format, ...) {
    if (debug_flag) {
        // Get the current time
        time_t now;
        struct tm newtime;
        char buf[80];

        time(&now);
        newtime = *localtime_r(&now, &newtime);

        // Format time, "YYYY-MM-DD HH:MM:SS"
        strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &newtime);

        // Prints [TIMESTAMP][DEBUG]
        printf("[%s][ROUT-DEBUG] ", buf);

        // Prints the debug-message
        va_list args;
        va_start(args, format);
        vprintf(format, args);
        va_end(args);
        printf("\n");
    }
}