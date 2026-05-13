#ifndef SESSION_H
#define SESSION_H

#include "config.h"
#include "types.h"
#include <stdbool.h>
#include <stdint.h>
#include <time.h>

// Persistent identity of a connected implant.
// Lives in the session table, owned by the session module.
// Does NOT contain fd or recv buffer — those live in client_t in listener.c,
// because they're connection-state, not session-state.
typedef struct {
    uint64_t id;
    char hostname[MAX_HOSTNAME_LEN];
    session_state_t state;
    time_t created_at;
    time_t last_seen;
    bool in_use;
} session_t;

// AllocaTe a new session in the table. Returns NULL if table is full
// Copies hostname into the sesion's own buffer.
session_t *session_create(uint64_t id, const char *hostname);

// Look up an existing session by ID. Return Null if not found
session_t *session_find(uint64_t id);

// Update last_seen to current time. Call whenever the implant sends everything
void session_touch(session_t *sess);

// Free a session solt. Marks in_use = False. Does not close any fd
// the caller is responsible for the connection clean up
void session_destroy(session_t *sess);

#endif