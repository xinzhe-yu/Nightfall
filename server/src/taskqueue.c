#include "taskqueue.h"
#include "logging.h"
#include "types.h"
#include <assert.h>
#include <stdint.h>
#include <string.h>

#define min(a, b) (((a) < (b)) ? (a) : (b))

int taskqueue_enqueue(session_t *sess, task_type_t type, const char *args, uint32_t args_len) {
    assert(sess);
    // if full return
    if (sess->task_count == MAX_TASKS_PER_SESSION) {
        LOG_WARN("taskqueue full for session=%llu, rejecting task type=%s",
                 (unsigned long long)sess->id, task_type_str(type));
        return -1;
    }

    task_t *task = &sess->tasks[sess->task_count];
    task->type = type;
    task->state = TASK_QUEUED;

    if (args_len > MAX_TASK_ARGS) {
        LOG_WARN("task args too large: %u > %d", args_len, MAX_TASK_ARGS);
        return -1;
    }
    memcpy(task->args, args, args_len);
    task->args_len = args_len;
    sess->task_count++;
    LOG_INFO("task enqueued: session=%llu type=%s args_len=%u count=%d/%d",
             (unsigned long long)sess->id, task_type_str(type), args_len, sess->task_count,
             MAX_TASKS_PER_SESSION);

    return 0;
}

int taskqueue_pop(session_t *sess, task_t out[], int max) {
    assert(sess);
    int n = min(sess->task_count, max);
    for (int i = 0; i < n; i++) {
        out[i] = sess->tasks[i];
    }

    int remaining = sess->task_count - n;
    for (int i = 0; i < remaining; i++) {
        sess->tasks[i] = sess->tasks[n + i];
    }

    sess->task_count = remaining;

    return n;
}