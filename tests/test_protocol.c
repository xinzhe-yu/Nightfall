#include "protocol.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

/*
gcc -Wall -Wextra -Werror -pedantic -std=c11 \
    -Icommon/include \
    tests/test_protocol.c \
    common/src/protocol.c \
    common/src/logging.c \
    common/src/types.c \
    -o tests/test_protocol
*/

int main(void) {

    uint8_t buf[MAX_MSG_SIZE];

    // Case 1
    uint64_t session_id = 482318;
    char *hostname = "TESTPC1";
    uint32_t hostname_len = strlen(hostname);

    int body_len = protocol_pack_checkin(buf + NIGHTFALL_HDR_SIZE, session_id,
                                         hostname, hostname_len);

    protocol_pack_header(buf, MSG_CHECKIN, body_len);

    recv_buf_t rbuf;
    recv_buf_reset(&rbuf);

    size_t total = NIGHTFALL_HDR_SIZE + body_len;
    memcpy(rbuf.buf, buf, total);
    rbuf.len = total;

    msg_header_t hdr;
    const uint8_t *body;
    uint32_t body_len_out;

    if (protocol_try_parse(&rbuf, &hdr, &body, &body_len_out)) {
        // unpack the body and verify values
    }

    uint64_t out_session_id;
    char out_hostname[256];
    uint32_t out_hostname_len;

    protocol_unpack_checkin(body, body_len_out, &out_session_id, out_hostname,
                            &out_hostname_len);

    out_hostname[out_hostname_len] = '\0';

    assert(out_session_id == session_id);
    assert(out_hostname_len == hostname_len);
    assert(memcmp(out_hostname, hostname, hostname_len) == 0);

    printf("checkin roundtrip: PASS\n");
}
