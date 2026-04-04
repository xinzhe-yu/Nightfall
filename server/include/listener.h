#ifndef LISTENER_H
#define LISTENER_H

#include <stdint.h>

// Initialize the listener: create socket, bind, listen,
// set up epoll. Call once at startup.
// Returns 0 on success, -1 on error.
int listener_init(uint16_t port);

// Run the event loop. Blocks forever (or until error/shutdown).
// This is the server's main loop — it calls epoll_wait,
// accepts connections, recvs data, dispatches messages.
void listener_run(void);

// Clean shutdown: close all client fds, close listen fd,
// close epoll fd.
void listener_shutdown(void);

#endif