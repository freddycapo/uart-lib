#ifndef UART_LIB_FRAME_H
#define UART_LIB_FRAME_H

#include <stdint.h>
#include <stddef.h>

#include "uart-msg.h"
#include "cobs.h"

/**
 * Frame on the wire:
 *
 *     COBS( [msg_id][seq][payload][crc16 LE] ) 0x00
 *
 * The CRC (CRC-16/CCITT-FALSE) covers msg_id, seq and payload, and is
 * computed before COBS encoding.
 */

#define FRAME_HEADER_SIZE       2   // msg_id + seq
#define FRAME_CRC_SIZE          2

/* Largest frame before COBS encoding */
#define FRAME_RAW_MAX_SIZE      (FRAME_HEADER_SIZE + MSG_MAX_PAYLOAD_SIZE + FRAME_CRC_SIZE)

/* Largest frame on the wire: COBS overhead + the 0x00 delimiter */
#define FRAME_ENCODED_MAX_SIZE  (COBS_ENCODED_MAX_SIZE(FRAME_RAW_MAX_SIZE) + 1)

static_assert(MSG_MAX_PAYLOAD_SIZE <= 255, "frame_t.len is a uint8_t");

/* Decoded frame. Copy payload into the struct matching id with memcpy. */
typedef struct {
    msg_id_t id;
    uint8_t  seq;
    uint8_t  len;                           // payload length in bytes
    uint8_t  payload[MSG_MAX_PAYLOAD_SIZE];
} frame_t;

typedef enum {
    FRAME_OK = 0,
    FRAME_ERR_COBS,     // invalid COBS encoding
    FRAME_ERR_CRC,      // CRC mismatch, data corrupted
    FRAME_ERR_ID,       // valid frame, but msg_id not in MSG_LIST
    FRAME_ERR_LEN       // frame too short/long, or payload size != msg_payload_size(id)
} frame_status_t;

/**
 * Builds the frame for message id with sequence number seq.
 * len must be equal to msg_payload_size(id).
 * out must be at least FRAME_ENCODED_MAX_SIZE bytes.
 * Returns the number of bytes to send (0x00 delimiter included),
 * or 0 if id is unknown or len does not match.
 */
size_t frame_encode(msg_id_t id, uint8_t seq,
                    const void *payload, size_t len, uint8_t *out);

/**
 * Decodes one frame. in holds the bytes received before the 0x00 delimiter
 * (delimiter excluded).
 * On FRAME_OK, out is filled; on error, out is left untouched.
 */
frame_status_t frame_decode(const uint8_t *in, size_t len, frame_t *out);

#endif // UART_LIB_FRAME_H
