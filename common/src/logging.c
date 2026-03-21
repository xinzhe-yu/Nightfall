#include <stdarg.h>
#include <stdio.h>
#include <time.h>

#include "logging.h"

static log_level_t current_level;

void log_init(log_level_t level) {
    current_level = level;
}

void log_write(log_level_t level, const char *file, int line, const char *fmt,
               ...) {

    // Filters callers level by current threshold level
    if (level < current_level)
        return;

    const char *level_str[] = {"DEBUG", "INFO ", "WARN ", "ERROR"};

    // Get current Time
    time_t now = time(NULL);

    // Convert it to local Time
    struct tm *t = localtime(&now);

    char location[64];
    snprintf(location, sizeof(location), "%s:%d", file, line);

    // Print prefix from the tm struct
    fprintf(stderr, "%04d-%02d-%02d %02d:%02d:%02d [%s] %-29s— ",
            t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min,
            t->tm_sec, level_str[level], location);

    // Print message
    // Forward variadic args to vfprintf
    va_list args;
    va_start(args, fmt); // set args to point right after fmt
    vfprintf(stderr, fmt, args);
    va_end(args);

    // newline
    fprintf(stderr, "\n");
}
