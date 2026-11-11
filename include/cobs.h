#ifndef UART_LIB_COBS_H
#define UART_LIB_COBS_H

#include <stdint.h>
#include <stddef.h>

/**
 * COBS -> Consistent Overhead Byte Stuffing
 *
 * Removes every 0x00 from the data, so that 0x00 can be used as the frame
 * delimiter. Each zero is replaced by a code byte telling how many bytes
 * follow before the next zero. Overhead: at most 1 byte every 254.
 *
 *     11 22 00 33  ->  03 11 22 02 33
 */

/* Minimum size of the output buffer of cobs_encode for an input of len bytes */
#define COBS_ENCODED_MAX_SIZE(len) ((len) + (len) / 254 + 1)

/**
 * Encodes len bytes of in into out. The 0x00 delimiter is NOT appended.
 * out must be at least COBS_ENCODED_MAX_SIZE(len) bytes.
 * Returns the number of bytes written to out.
 */
size_t cobs_encode(const uint8_t *in, size_t len, uint8_t *out);

/**
 * Decodes len bytes of in (without the 0x00 delimiter) into out.
 * out must be at least len bytes (decoded data is always shorter).
 * Returns the number of bytes written to out, or 0 if the input is invalid
 * (empty, contains a 0x00, or a code byte points past the end).
 * An encoded empty buffer also returns 0: frames are never empty, so it is
 * treated as an error.
 */
size_t cobs_decode(const uint8_t *in, size_t len, uint8_t *out);

#endif // UART_LIB_COBS_H
