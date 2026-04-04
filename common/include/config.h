#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>

// Protocol Header "NF01".
#define NIGHTFALL_MAGIC 0x4E463031

// Size in bytes of the fixed protocol header.
#define NIGHTFALL_HDR_SIZE 12

// Maximum total message size (header + body).
#define MAX_MSG_SIZE (64 * 1024) // Growing this stack overflow risk 

// Maximum concurrent sessions the server tracks.
#define MAX_SESSIONS 1024

// How many tasks can be queued per session. 
#define MAX_TASKS_PER_SESSION 16

// Maximum length of task argument string.
#define MAX_TASK_ARGS  4096

// Default beacon sleep interval in seconds.
#define DEFAULT_SLEEP_SEC 5 // longer in real senario 

// Default jitter percentage (0-100).
#define DEFAULT_JITTER_PCT 20 

// Default server listening port. 
#define DEFAULT_PORT 9090

#endif