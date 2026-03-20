#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>

// Message types - Identify what kind of protocol message this is
// Used in the wire header's msg_type field.
typedef enum {
    MSG_CHECKIN = 1,     // implant -> server: Heres my ID
    MSG_CHECKIN_ACK = 2, // server -> implant: Ack, here are tasks
    MSG_TASK = 3,        // server -> implant: Execute this
    MSG_RESULT = 4,      // implant -> server: Here is the output
    MSG_HEARTBEAT = 5,   // either direction: Still alive
    MSG_SHUTDOWN = 6,    // Server -> implant: Exit cleanly
} msg_type_t;

// Task types - what the implants can be told to do
typedef enum {
    TASK_SHELL = 1,    // Run shell command, return output
    TASK_WHOAMI = 2,   // Return current username
    TASK_PWD = 3,      // Return working directory
    TASK_LS = 4,       // List directory contents
    TASK_CD = 5,       // Change working directory
    TASK_UPLOAD = 6,   // Server sends file to implant
    TASK_DOWNLOAD = 7, // Implant sends file to server
    TASK_SLEEP = 8,    // Change beacon interval
    TASK_EXIT = 9,     // Implant shuts down cleanly
} task_type_t;

// Session states — tracks the lifecycle of a session (permanent identity).
// Note: there is no TASKED or RESPONDING state. Task status lives
// on the task struct, not the session. Session state reflects
// whether the implant is reachable, not what it's doing.
typedef enum {
    SESSION_NEW,          // Just created on first checkin
    SESSION_ACTIVE,       // Connected and recently seen
    SESSION_DISCONNECTED, // Connection dropped, session persists
    SESSION_DORMANT,      // Missed multiple expected checkins
    SESSION_KILLED,       // Operator manually terminated
} session_state_t;

// Task states - tracks one task through its lifecycle.
typedef enum {
    TASK_QUEUED,    // Operator submitted, waiting for implant to check in
    TASK_SENT,      // Delivered to implant in CHECKIN_ACK
    TASK_COMPLETED, // Result received back from implant
    TASK_FAILED,    // Implant reported an error executing this task
} task_state_t;

// Used for logging
// converts enum value to string
const char *msg_type_str(msg_type_t t);
const char *task_type_str(task_type_t t);
const char *session_state_str(session_state_t t);
const char *task_state_str(task_state_t t);

#endif