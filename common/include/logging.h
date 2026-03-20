#ifndef LOGGING_H
#define LOGGING_H

// Reports based on severity level
typedef enum {
    LOG_LEVEL_DEBUG = 0,
    LOG_LEVEL_INFO = 1,
    LOG_LEVEL_WARN = 2,
    LOG_LEVEL_ERROR = 3,
} log_level_t;

// Public logging macros - Use thsee in your code.
//
// do { ... } while(0) makes the macro safe to use after
// if-statements without dangling semicolon issues/terminating the statement.
//
// ## before __VA_ARGS__ swallows the trailing comma when
// there are zero variadic args, so LOG_INFO("Hello") works
//
// Backslash continues the macro across multiple lines

// Internal operation working correctly
#define LOG_DEBUG(fmt, ...)                                                    \
    do {                                                                       \
        log_write(LOG_LEVEL_DEBUG, __FILE__, __LINE__, fmt, ##__VA_ARGS__);    \
    } while (0)

// Significant events at human relevant cadence
// "Listener started on port 9090." "New session registered: hostname=target1."
#define LOG_INFO(fmt, ...)                                                     \
    do {                                                                       \
        log_write(LOG_LEVEL_INFO, __FILE__, __LINE__, fmt, ##__VA_ARGS__);     \
    } while (0)

// Something unexpected but recoverable.
// "Implant session 3 missed expected checkin, marking dormant."
// "Task queue full for session 7, dropping task."
// "Received unknown msg_type=99, ignoring."
#define LOG_WARN(fmt, ...)                                                     \
    do {                                                                       \
        log_write(LOG_LEVEL_WARN, __FILE__, __LINE__, fmt, ##__VA_ARGS__);     \
    } while (0)

// Something broke.
// "accept() failed."
// "Magic mismatch: expected 0x4E463031 got 0xDEADBEEF."
// "recv() returned -1, dropping connection."
#define LOG_ERROR(fmt, ...)                                                    \
    do {                                                                       \
        log_write(LOG_LEVEL_ERROR, __FILE__, __LINE__, fmt, ##__VA_ARGS__);    \
    } while (0)

// Set minimum log level
// messages below this threshold are silenced.
void log_init(log_level_t level);

// Internal function — do NOT call directly, use the macros above.
void log_write(log_level_t level, const char *file, int line, const char *fmt,
               ...);

#endif