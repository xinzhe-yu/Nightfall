#include "taskqueue.h"
#include "types.h"

#define min(a, b) (((a) < (b)) ? (a) : (b))

int taskqueue_enqueue(session_t *sess, task_type_t type, const char *args, uint32_t args_len) {
    // if full return
    if (sess->task_count == MAX_TASKS_PER_SESSION) {
        return -1;
    }

    task_t *task = &sess->tasks[sess->task_count];
    task->type = type;
    task->state = TASK_QUEUED;
    memcpy(task->args, args, sizeof(args));
    task->args_len = args_len;
    sess->task_count++;
    // LOG
}

int taskqueue_pop(session_t *sess, task_t out[], int max) {
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