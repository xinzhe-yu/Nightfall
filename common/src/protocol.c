#include "protocol.h"
#include "logging.h"

void recv_buf_reset(recv_buf_t *rbuf) {
    rbuf->len = 0;
}

int protocol_pack_header(uint8_t *buf, msg_type_t type, uint32_t body_len) {
    // Create a header before the front of body

    uint32_t magic = htonl(NIGHTFALL_MAGIC);
    uint32_t mtype = htonl((uint32_t)type);
    uint32_t blen = htonl(body_len);

    memcpy(buf, &magic, 4);
    memcpy(buf + 4, &mtype, 4);
    memcpy(buf + 8, &blen, 4);

    LOG_DEBUG("Pack_header: type=%s(%d) body_len=%u", msg_type_str(type),
              (int)type, body_len);

    return NIGHTFALL_HDR_SIZE;
}

int protocol_pack_checkin(uint8_t *buf, uint64_t session_id,
                          const char *hostname, uint32_t hostname_len) {
    // Pack the body

    // uint64_t sessionid needs to be split into 2 uint32_t for htonl()
    uint32_t sh = (uint32_t)(session_id >> 32);
    uint32_t sl = (uint32_t)session_id;

    uint32_t sessidh = htonl(sh);
    uint32_t sessidl = htonl(sl);
    uint32_t hnlen = htonl(hostname_len);

    size_t off = 0;
    memcpy(buf + off, &sessidh, 4);
    off += 4;
    memcpy(buf + off, &sessidl, 4);
    off += 4;
    memcpy(buf + off, &hnlen, 4);
    off += 4;
    memcpy(buf + off, hostname, hostname_len);

    LOG_DEBUG("Pack_checkin: session_id=%llu, hostname_len=%u, hostname=%s",
              (unsigned long long)session_id, hostname_len, hostname);

    return off + hostname_len;
}

int protocol_pack_checkin_ack(uint8_t *buf, uint32_t task_count) {
    // Pack the body

    uint32_t tcount = htonl(task_count);

    size_t off = 0;
    memcpy(buf + off, &tcount, 4);
    off += 4;

    LOG_DEBUG("Pack_checkin_ack: Task_count=%u", task_count);

    return off;
}

int protocol_pack_task(uint8_t *buf, task_type_t type, const char *args,
                       uint32_t args_len) {
    // Pack MSG_TASK body

    uint32_t btype = htonl((uint32_t)type);
    uint32_t barg_len = htonl(args_len);

    size_t off = 0;
    memcpy(buf + off, &btype, 4);
    off += 4;
    memcpy(buf + off, &barg_len, 4);
    off += 4;
    memcpy(buf + off, args, args_len);

    LOG_DEBUG("Pack_task: type=%s(%d) args_len=%u args=%s", task_type_str(type),
              (int)type, args_len, args);

    return off + args_len;
}

int protocol_pack_result(uint8_t *buf, task_type_t type, int32_t status,
                         const uint8_t *data, uint32_t data_len) {
    // Pack MSG_RESULT body

    uint32_t btype = htonl((uint32_t)type);
    uint32_t bstatus = htonl((uint32_t)status);
    uint32_t bdata_len = htonl(data_len);

    size_t off = 0;
    memcpy(buf + off, &btype, 4);
    off += 4;
    memcpy(buf + off, &bstatus, 4);
    off += 4;
    memcpy(buf + off, &bdata_len, 4);
    off += 4;
    memcpy(buf + off, data, data_len);

    LOG_DEBUG("Pack_result: type=%s(%d) status=%d data_len=%u "
              "first4=%02x%02x%02x%02x",
              task_type_str(type), (int)type, status, data_len,
              data_len > 0 ? data[0] : 0, data_len > 1 ? data[1] : 0,
              data_len > 2 ? data[2] : 0, data_len > 3 ? data[3] : 0);

    return off + data_len;
}

// ── Unpack functions ───────────────────────────────────────────
int protocol_unpack_header(const uint8_t *buf, msg_header_t *hdr) {

    // Unpack 12 byte header
    // Keeps track of offset
    // 4 bytes for each item is predetermined
    size_t off = 0;
    memcpy(&hdr->magic, buf + off, 4);
    off += 4;
    memcpy(&hdr->msg_type, buf + off, 4);
    off += 4;
    memcpy(&hdr->body_len, buf + off, 4);

    hdr->magic = ntohl(hdr->magic);
    hdr->msg_type = ntohl(hdr->msg_type);
    hdr->body_len = ntohl(hdr->body_len);

    LOG_DEBUG("Unpack_header: magic=0x%08X type=%s(%d) body_len=%u", hdr->magic,
              msg_type_str(hdr->msg_type), (int)hdr->msg_type, hdr->body_len);

    if (hdr->magic != NIGHTFALL_MAGIC) {
        LOG_ERROR("Magic mismatch: expected=0x%08X got=0x%08X", NIGHTFALL_MAGIC,
                  hdr->magic);
        return -1;
    }

    return 0;
}

int protocol_unpack_checkin(const uint8_t *buf, size_t len,
                            uint64_t *session_id, char *hostname,
                            uint32_t *hostname_len) {
    if (len < 12) {
        LOG_ERROR("Unpack_checkin: body too short: %zu", len);
        return -1; // minimum: 8 (session_id) + 4 (hostname_len)
    }
    size_t off = 0;
    uint32_t sh, sl;
    memcpy(&sh, buf + off, 4);
    off += 4;
    memcpy(&sl, buf + off, 4);
    off += 4;
    sh = ntohl(sh);
    sl = ntohl(sl);
    *session_id = ((uint64_t)sh << 32) | sl;

    memcpy(hostname_len, buf + off, 4);
    off += 4;
    *hostname_len = ntohl(*hostname_len);
    if (len < 12 + *hostname_len) {
        LOG_ERROR("Unpack_checkin: body too short for hostname: %zu < %u", len,
                  12 + *hostname_len);
        return -1;
    }

    memcpy(hostname, buf + off, *hostname_len);
    off += *hostname_len;

    LOG_DEBUG("Unpack_checkin: session_id=%llu, hostname_len=%u, hostname=%s",
              (unsigned long long)*session_id, *hostname_len, hostname);

    return 0;
}

int protocol_unpack_checkin_ack(const uint8_t *buf, size_t len,
                                uint32_t *task_count) {
    if (len < 4) {
        LOG_ERROR("Unpack_checking_ack: body too short: %zu", len);
        return -1;
    }
    size_t off = 0;
    memcpy(task_count, buf + off, 4);
    off += 4;

    *task_count = ntohl(*task_count);

    LOG_DEBUG("Unpack_checkin_ack: task_count=%u", *task_count);

    return 0;
}

int protocol_unpack_task(const uint8_t *buf, size_t len, task_type_t *type,
                         char *args, uint32_t *args_len) {
    if (len < 8) {
        LOG_ERROR("Unpack_task: body too short: %zu", len);
        return -1;
    }
    size_t off = 0;

    //'type' needs to be casted into enum
    // Temp variable prevents storing network order value directly into enum
    uint32_t raw_type;
    memcpy(&raw_type, buf + off, 4);
    off += 4;
    raw_type = ntohl(raw_type);
    *type = (task_type_t)raw_type;

    memcpy(args_len, buf + off, 4);
    off += 4;
    *args_len = ntohl(*args_len);

    if (8 + *args_len > len) { // Bytes we need > bytes we have
        LOG_ERROR("Unpack_task: body too short for args: %zu < %u", len,
                  8 + *args_len);
        return -1;
    }
    memcpy(args, buf + off, *args_len);

    LOG_DEBUG("Unpack_task: type=%s(%d) args_len=%u args=%s",
              task_type_str(*type), (int)*type, *args_len, args);

    return 0;
}

int protocol_unpack_result(const uint8_t *buf, size_t len, task_type_t *type,
                           int32_t *status, uint8_t *data, uint32_t *data_len) {
    //
    if (len < 12) {
        LOG_ERROR("Unpack_result: body too short: %zu", len);
        return -1;
    }

    size_t off = 0;
    uint32_t raw_type;

    memcpy(&raw_type, buf + off, 4);
    off += 4;
    raw_type = ntohl(raw_type);
    *type = (task_type_t)raw_type;

    memcpy(status, buf + off, 4);
    off += 4;
    *status = (int32_t)ntohl(*status);

    memcpy(data_len, buf + off, 4);
    off += 4;
    *data_len = ntohl(*data_len);

    if (*data_len + 12 > len) {
        LOG_ERROR("Unpack_result: body too short for data: %zu < %u", len,
                  12 + *data_len);
        return -1;
    }

    memcpy(data, buf + off, *data_len);

    LOG_DEBUG("Unpack_result: type=%s(%d) status=%d data_len=%u",
              task_type_str(*type), (int)*type, *status, *data_len);

    return 0;
}

// ── TCP reassembly ─────────────────────────────────────────────
bool protocol_try_parse(recv_buf_t *rbuf, msg_header_t *out_hdr,
                        const uint8_t **out_body, uint32_t *out_body_len) {

    // Check if rbuf is more than header size
    if (rbuf->len < NIGHTFALL_HDR_SIZE) {
        LOG_DEBUG("try_parse: need %d bytes, have %zu", NIGHTFALL_HDR_SIZE,
                  rbuf->len);
        return false;
    }

    // Get header
    if (protocol_unpack_header(rbuf->buf, out_hdr) < 0) {
        LOG_ERROR("try_parse: header unpack failed (bad magic?)");
        return false;
    }

    size_t msg_size = NIGHTFALL_HDR_SIZE + out_hdr->body_len;

    // Check if rbuf is more than header + body
    if (rbuf->len < msg_size) {
        LOG_DEBUG("try_parse: incomplete msg, need %zu have %zu", msg_size,
                  rbuf->len);
        return false;
    }

    // Point body to start of body
    *out_body = rbuf->buf + NIGHTFALL_HDR_SIZE;
    // Point len to hdr struct
    *out_body_len = out_hdr->body_len;

    memmove(rbuf->buf, rbuf->buf + msg_size, rbuf->len - msg_size);
    rbuf->len -= msg_size;

    return true;
}