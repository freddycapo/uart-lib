#include "frame.h"

#include <string.h>

#include "crc16.h"

size_t frame_encode(msg_id_t id, uint8_t seq,
                    const void *payload, size_t len, uint8_t *out) {
    int expected = msg_payload_size(id);
    if (expected < 0 || (size_t)expected != len)
        return 0;

    // raw frame: [id][seq][payload][crc16 LE]
    uint8_t raw[FRAME_RAW_MAX_SIZE];
    size_t n = 0;

    raw[n++] = id;
    raw[n++] = seq;
    memcpy(&raw[n], payload, len);
    n += len;

    uint16_t crc = crc16_ccitt(raw, n);
    raw[n++] = (uint8_t)(crc & 0xFF);       // low byte first
    raw[n++] = (uint8_t)(crc >> 8);

    size_t o = cobs_encode(raw, n, out);
    out[o++] = 0x00;                        // delimiter
    return o;
}

frame_status_t frame_decode(const uint8_t *in, size_t len, frame_t *out) {
    if (len == 0 || len > FRAME_ENCODED_MAX_SIZE - 1)
        return FRAME_ERR_LEN;

    // COBS output is never longer than its input, so size raw on the input
    uint8_t raw[FRAME_ENCODED_MAX_SIZE];
    size_t n = cobs_decode(in, len, raw);
    if (n == 0)
        return FRAME_ERR_COBS;

    if (n < FRAME_HEADER_SIZE + FRAME_CRC_SIZE || n > FRAME_RAW_MAX_SIZE)
        return FRAME_ERR_LEN;

    // CRC first: if the data is corrupted, id and length can't be trusted
    uint16_t rx_crc = (uint16_t)(raw[n - 2] | (raw[n - 1] << 8));
    if (crc16_ccitt(raw, n - FRAME_CRC_SIZE) != rx_crc)
        return FRAME_ERR_CRC;

    msg_id_t id = raw[0];
    size_t payload_len = n - FRAME_HEADER_SIZE - FRAME_CRC_SIZE;

    int expected = msg_payload_size(id);
    if (expected < 0)
        return FRAME_ERR_ID;
    if ((size_t)expected != payload_len)
        return FRAME_ERR_LEN;

    out->id = id;
    out->seq = raw[1];
    out->len = (uint8_t)payload_len;
    memcpy(out->payload, &raw[FRAME_HEADER_SIZE], payload_len);
    return FRAME_OK;
}
