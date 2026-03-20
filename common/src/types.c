#include "types.h"

const char *msg_type_str(msg_type_t t) {
    switch (t) {
    case MSG_CHECKIN:
        return "CHECKIN";
    case MSG_CHECKIN_ACK:
        return "CHECKIN_ACK";
    case MSG_TASK:
        return "TASK";
    case MSG_RESULT:
        return "RESULT";
    case MSG_HEARTBEAT:
        return "HEARTBEAT";
    case MSG_SHUTDOWN:
        return "SHUTDOWN";
    default:
        return "UNKNOWN";
    }
}

const char *task_type_str(task_type_t t) {
    switch (t) {
    case TASK_SHELL:
        return "SHELL";
    case TASK_WHOAMI:
        return "WHOAMI";
    case TASK_PWD:
        return "PWD";
    case TASK_LS:
        return "LS";
    case TASK_CD:
        return "CD";
    case TASK_UPLOAD:
        return "UPLOAD";
    case TASK_DOWNLOAD:
        return "DOWNLOAD";
    case TASK_SLEEP:
        return "SLEEP";
    case TASK_EXIT:
        return "EXIT";
    default:
        return "UNKNOWN";
    }
}

const char *session_state_str(session_state_t s) {
    switch (s) {
    case SESSION_NEW:
        return "NEW";
    case SESSION_ACTIVE:
        return "ACTIVE";
    case SESSION_DISCONNECTED:
        return "DISCONNECTED";
    case SESSION_DORMANT:
        return "DORMANT";
    case SESSION_KILLED:
        return "KILLED";
    default:
        return "UNKNOWN";
    }
}

const char *task_state_str(task_state_t s) {
    switch (s) {
    case TASK_QUEUED:
        return "QUEUED";
    case TASK_SENT:
        return "SENT";
    case TASK_COMPLETED:
        return "COMPLETED";
    case TASK_FAILED:
        return "FAILED";
    default:
        return "UNKNOWN";
    }
}