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

    LOG_DEBUG("pack_result: type=%s(%d) status=%d data_len=%u "
              "first4=%02x%02x%02x%02x",
              task_type_str(type), (int)type, status, data_len,
              data_len > 0 ? data[0] : 0, data_len > 1 ? data[1] : 0,
              data_len > 2 ? data[2] : 0, data_len > 3 ? data[3] : 0);

    return off + data_len;
}
