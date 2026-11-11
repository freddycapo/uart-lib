#ifndef CRC_H
#define CRC_H

#include <stdint.h>
#include <stddef.h>

/**
 * CRC -> Cyclic Redundancy Check
 * 
 * The sender calculates the CRC of the data, and append it to the message.
 * The receiver recalculates it and checks if it corresponds with the one that arrived,
 * so it can understand if the data are corrupted or not.
 */

uint16_t crc16_ccitt(const uint8_t *data, size_t len);




#endif
