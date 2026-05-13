#include "session.h"
#include "config.h"
#include "logging.h"
#include "types.h"
#include <assert.h>

#include <string.h>
#include <time.h>

static struct {
    session_t sessions[MAX_SESSIONS];
} table;

// ── Public API ─────────────────────────────────────────────────

session_t *session_create(uint64_t id, const char *hostname) {
    for (int i = 0; i < MAX_SESSIONS; i++) {
        if (table.sessions[i].in_use) {
            continue;
        }

        session_t *sess = &table.sessions[i];
        sess->id = id;
        strncpy(sess->hostname, hostname, MAX_HOSTNAME_LEN - 1);
        sess->hostname[MAX_HOSTNAME_LEN - 1] = '\0';
        sess->state = SESSION_NEW;
        sess->created_at = time(NULL);
        sess->last_seen = time(NULL);
        sess->in_use = true;
        LOG_INFO("New session created: ID: %llu Hostname: %s", (unsigned long long)id, hostname);
        return sess;
    }
    LOG_WARN("session_create: table full, rejecting id=%llu", (unsigned long long)id);
    return NULL;
}

session_t *session_find(uint64_t id) {

    for (int i = 0; i < MAX_SESSIONS; i++) {
        if (table.sessions[i].in_use == true && table.sessions[i].id == id) {
            return &table.sessions[i];
        }
    }

    return NULL;
}

void session_touch(session_t *sess) {
    assert(sess);
    sess->last_seen = time(NULL);
}

void session_destroy(session_t *sess) {
    assert(sess);
    LOG_INFO("session destroyed: id=%llu hostname=%s", (unsigned long long)sess->id,
             sess->hostname);
    memset(sess, 0, sizeof(*sess));
}