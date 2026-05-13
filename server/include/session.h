#ifndef SESSION_H
#define SESSION_H

#include <stdbool.h>
#include <time.h>

typedef struct {
    // No fd, fd lives in client_t in listener.c
    uint64_t id;
    char hostname[64];
    session_state_t state;
    time_t created_at;
    time_t last_seen;
    bool in_use;
} session_t;

session_t *session_create(uint64_t id, const char *hostname);

session_t *session_find(uint64_t id);

void session_touch(session_t *sess);

void session_destroy(uint64_t *sess);

#endif