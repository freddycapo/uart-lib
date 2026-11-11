#include "cobs.h"

size_t cobs_encode(const uint8_t *in, size_t len, uint8_t *out) {
    size_t code_idx = 0;    // position reserved for the current code byte
    size_t o = 1;           // next write position in out
    uint8_t code = 1;       // distance from code_idx to the next zero

    for (size_t i = 0; i < len; i++) {
        if (in[i] == 0x00) {
            // close the block: the zero is replaced by the code
            out[code_idx] = code;
            code_idx = o++;
            code = 1;
        } else {
            out[o++] = in[i];
            code++;
            if (code == 0xFF) {
                // block of 254 non-zero bytes: close it, no implicit zero
                out[code_idx] = code;
                code_idx = o++;
                code = 1;
            }
        }
    }

    out[code_idx] = code;   // last block (its zero is the implicit end of data)
    return o;
}

size_t cobs_decode(const uint8_t *in, size_t len, uint8_t *out) {
    size_t i = 0;
    size_t o = 0;

    while (i < len) {
        uint8_t code = in[i++];

        if (code == 0x00)
            return 0;                       // zero inside the encoded data
        if ((size_t)(code - 1) > len - i)
            return 0;                       // code points past the end

        for (uint8_t k = 1; k < code; k++) {
            if (in[i] == 0x00)
                return 0;
            out[o++] = in[i++];
        }

        // every block ends with a zero, except 0xFF blocks and the last one
        if (code != 0xFF && i < len)
            out[o++] = 0x00;
    }

    return o;
}
