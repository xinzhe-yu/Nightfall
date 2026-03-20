#include "protocol.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void) {
    uint8_t buf[MAX_MSG_SIZE];

    // test pack_header then unpack_header roundtrip
    int n = protocol_pack_header(buf, MSG_CHECKIN, 42);
    assert(n == NIGHTFALL_HDR_SIZE);

    msg_header_t hdr;
    int rc = protocol_unpack_header(buf, &hdr);
    assert(rc == 0);
    assert(hdr.magic == NIGHTFALL_MAGIC);
    assert(hdr.msg_type == MSG_CHECKIN);
    assert(hdr.body_len == 42);

    printf("pack_header roundtrip: PASS\n");
    return 0;
}