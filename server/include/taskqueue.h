#ifndef TASKQUEUE_H
#define TASKQUEUE_H

#include "session.h"

/* returns 0 on success, -1 if the queue is full*/
int taskqueue_enqueue(session_t *sess, task_type_t type, const char *args, uint32_t args_len);

/* copies up to max tasks into out, removes them from queue
    returns count copied */
int taskqueue_pop(session_t *sess, task_t out[], int max);
#endif