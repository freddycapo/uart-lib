#include "unity.h"
#include "crc16.h"
#include "cobs.h"
#include "frame.h"
#include <string.h>

// crc tests
void test_crc_known(void) {
    const uint8_t dati[] = "123456789";

    uint16_t risultato = crc16_ccitt(dati, 9);

    TEST_ASSERT_EQUAL_HEX16(0x29B1, risultato);
}

void test_crc_different_data(void) {
    const uint8_t dati_a[] = {1, 2, 3};
    const uint8_t dati_b[] = {1, 2, 4};

    uint16_t crc_a = crc16_ccitt(dati_a, sizeof(dati_a));
    uint16_t crc_b = crc16_ccitt(dati_b, sizeof(dati_b));

    TEST_ASSERT_NOT_EQUAL(crc_a, crc_b);
}

void test_crc_empty_data(void) {
    uint16_t risultato = crc16_ccitt(NULL, 0);

    TEST_ASSERT_EQUAL_HEX16(0xFFFF, risultato);
}

void test_frame_reject_crc_error(void) {
    ServoMsg input = {
        .servo_id = 14,
        .angle_cdeg = -1250
    };

    uint8_t encoded[FRAME_ENCODED_MAX_SIZE];

    size_t encoded_len = frame_encode(
        MSG_ID_SERVO,
        0,
        &input,
        sizeof(input),
        encoded
    );

    TEST_ASSERT_NOT_EQUAL(0, encoded_len);

    encoded[1] ^= 0x01;

    frame_t output;

    TEST_ASSERT_EQUAL(
        FRAME_ERR_CRC,
        frame_decode(encoded, encoded_len - 1, &output)
    );
}

// cobs tests
void test_cobs_encode_decode(void) {
    // const uint8_t input[] = {0x11, 0x22, 0x00, 0x33};
    const uint8_t input[] = {0x00, 0x00, 0x01, 0x00};

    uint8_t encoded[COBS_ENCODED_MAX_SIZE(sizeof(input))];
    uint8_t decoded[sizeof(input)];
    
    size_t encoded_len = cobs_encode(
        input, sizeof(input), encoded
    );

    size_t decoded_len = cobs_decode(
        encoded, encoded_len, decoded
    );

    TEST_ASSERT_EQUAL_size_t(sizeof(input), decoded_len);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(input, decoded, sizeof(input));
}

void test_cobs_invalid_input(void) {
    const uint8_t invalid[] = {0x03, 0x11};
    uint8_t decoded[10];

    size_t result = cobs_decode(
        invalid,
        sizeof(invalid),
        decoded
    );

    TEST_ASSERT_EQUAL_size_t(0, result);
}

void test_cobs_decode_null_input(void) {
    uint8_t output[10];

    TEST_ASSERT_EQUAL_size_t(
        0,
        cobs_decode(NULL, 0, output)
    );
}

void test_cobs_max_block_size(void) {
    uint8_t input[254];
    uint8_t encoded[COBS_ENCODED_MAX_SIZE(sizeof(input))];
    uint8_t decoded[sizeof(input)];

    memset(input, 0x55, sizeof(input));

    size_t encoded_len =
        cobs_encode(input, sizeof(input), encoded);

    size_t decoded_len =
        cobs_decode(encoded, encoded_len, decoded);

    TEST_ASSERT_EQUAL_size_t(sizeof(input), decoded_len);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(
        input,
        decoded,
        sizeof(input)
    );
}

void test_cobs_internal_zero(void) {
    const uint8_t invalid[] = {0x02, 0x11, 0x00};
    uint8_t decoded[10];

    TEST_ASSERT_EQUAL_size_t(
        0,
        cobs_decode(invalid, sizeof(invalid), decoded)
    );
}

void test_frame_reject_cobs_invalid(void) {
    const uint8_t invalid[] = {0x03, 0x11};
    frame_t output;
    memset(&output, 0xAA, sizeof(output));

    frame_status_t status =
        frame_decode(invalid, sizeof(invalid), &output);

    TEST_ASSERT_EQUAL(FRAME_ERR_COBS, status);

    for (size_t i = 0; i < sizeof(output); i++) {
        TEST_ASSERT_EQUAL_UINT8(
            0xAA,
            ((uint8_t *)&output)[i]
        );
    }
}

// frame tests
void test_frame_encode_decode_servo(void) {
    ServoMsg input = {
        .servo_id = 14,
        .angle_cdeg = -1250
    };

    uint8_t encoded[FRAME_ENCODED_MAX_SIZE];

    size_t encoded_len = frame_encode(
        MSG_ID_SERVO,
        7,
        &input,
        sizeof(input),
        encoded
    );

    TEST_ASSERT_NOT_EQUAL(0, encoded_len);
    TEST_ASSERT_EQUAL_UINT8(
        0x00,
        encoded[encoded_len - 1]
    );

    frame_t decoded;

    frame_status_t status = frame_decode(
        encoded,
        encoded_len - 1,
        &decoded
    );

    TEST_ASSERT_EQUAL(FRAME_OK, status);
    TEST_ASSERT_EQUAL_UINT8(MSG_ID_SERVO, decoded.id);
    TEST_ASSERT_EQUAL_UINT8(7, decoded.seq);
    TEST_ASSERT_EQUAL_UINT8(sizeof(ServoMsg), decoded.len);

    TEST_ASSERT_EQUAL_UINT8_ARRAY(
        (uint8_t *)&input,
        decoded.payload,
        sizeof(input)
    );
}

void test_message_sizes(void) {
    TEST_ASSERT_EQUAL_size_t(16, sizeof(ImuMsg));
    TEST_ASSERT_EQUAL_size_t(3, sizeof(ServoMsg));

    TEST_ASSERT_EQUAL(16, msg_payload_size(MSG_ID_IMU));
    TEST_ASSERT_EQUAL(3, msg_payload_size(MSG_ID_SERVO));
    TEST_ASSERT_EQUAL(-1, msg_payload_size(0x99));
}

// ==========================================
// TEST AGGIUNTIVI (Edge Case & Sicurezza)
// ==========================================

// 1. COBS: blocco da 255 byte non-zero (verifica lo split automatico con 0xFF)
void test_cobs_255_nonzero_bytes(void) {
    uint8_t input[255];
    uint8_t encoded[COBS_ENCODED_MAX_SIZE(sizeof(input))];
    uint8_t decoded[sizeof(input)];

    // Riempi con 255 byte diversi da zero
    memset(input, 0xAA, sizeof(input));

    size_t encoded_len = cobs_encode(input, sizeof(input), encoded);
    
    // Con 255 byte senza zeri, COBS deve aggiungere 2 byte di overhead (totale 257)
    TEST_ASSERT_EQUAL_size_t(257, encoded_len);
    TEST_ASSERT_EQUAL_UINT8(0xFF, encoded[0]);

    size_t decoded_len = cobs_decode(encoded, encoded_len, decoded);

    TEST_ASSERT_EQUAL_size_t(sizeof(input), decoded_len);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(input, decoded, sizeof(input));
}

// 2. FRAME: Round-trip completo per il messaggio IMU (16 byte)
void test_frame_encode_decode_imu(void) {
    // Inizializza con un pattern noto per non dipendere dai nomi interni dei campi
    ImuMsg input;
    memset(&input, 0x5A, sizeof(input));

    uint8_t encoded[FRAME_ENCODED_MAX_SIZE];

    size_t encoded_len = frame_encode(
        MSG_ID_IMU,
        42,
        &input,
        sizeof(input),
        encoded
    );

    TEST_ASSERT_NOT_EQUAL(0, encoded_len);
    TEST_ASSERT_EQUAL_UINT8(0x00, encoded[encoded_len - 1]);

    frame_t decoded;
    frame_status_t status = frame_decode(
        encoded,
        encoded_len - 1,
        &decoded
    );

    TEST_ASSERT_EQUAL(FRAME_OK, status);
    TEST_ASSERT_EQUAL_UINT8(MSG_ID_IMU, decoded.id);
    TEST_ASSERT_EQUAL_UINT8(42, decoded.seq);
    TEST_ASSERT_EQUAL_UINT8(sizeof(ImuMsg), decoded.len);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(
        (uint8_t *)&input,
        decoded.payload,
        sizeof(input)
    );
}

// 3. FRAME: Rifiuta pacchetto troppo corto (previene out-of-bounds read)
void test_frame_refuse_too_short(void) {
    // Un frame COBS valido di soli 2 byte (meno dell'header minimo + CRC)
    const uint8_t raw_short[] = {0x01, 0x02};
    uint8_t encoded[COBS_ENCODED_MAX_SIZE(sizeof(raw_short))];
    size_t enc_len = cobs_encode(raw_short, sizeof(raw_short), encoded);

    frame_t output;
    frame_status_t status = frame_decode(encoded, enc_len, &output);

    TEST_ASSERT_NOT_EQUAL(FRAME_OK, status);
    // Se hai un codice specifico per la lunghezza minima, scommenta sotto:
    // TEST_ASSERT_EQUAL(FRAME_ERR_LEN, status);
}

// 4. FRAME: Rifiuta ID sconosciuto lato ricezione (frame_decode)
void test_frame_decode_rejects_unknown_msg_id(void) {
    // Costruisce un frame raw formalmente valido con ID = 0x99:
    // [ID=0x99, SEQ=1, LEN=1, PAYLOAD=0x42, CRC_MSB, CRC_LSB]
    uint8_t raw[6] = {0x99, 0x01, 0x01, 0x42, 0x00, 0x00};
    
    uint16_t crc = crc16_ccitt(raw, 4);
    raw[4] = (uint8_t)(crc >> 8);    // Inverti se il tuo protocollo usa Little Endian
    raw[5] = (uint8_t)(crc & 0xFF);

    uint8_t encoded[COBS_ENCODED_MAX_SIZE(sizeof(raw))];
    size_t enc_len = cobs_encode(raw, sizeof(raw), encoded);

    frame_t output;
    frame_status_t status = frame_decode(encoded, enc_len, &output);

    TEST_ASSERT_NOT_EQUAL(FRAME_OK, status);
    // TEST_ASSERT_EQUAL(FRAME_ERR_UNKNOWN_ID, status);
}

// 5. FRAME: Rifiuta payload con lunghezza incoerente rispetto all'ID
void test_frame_decode_rejects_wrong_payload_len(void) {
    // MSG_ID_SERVO richiede 3 byte, ma dichiariamo LEN = 1 nel header
    uint8_t raw[6] = {MSG_ID_SERVO, 0x01, 0x01, 0xFF, 0x00, 0x00};
    
    uint16_t crc = crc16_ccitt(raw, 4);
    raw[4] = (uint8_t)(crc >> 8);
    raw[5] = (uint8_t)(crc & 0xFF);

    uint8_t encoded[COBS_ENCODED_MAX_SIZE(sizeof(raw))];
    size_t enc_len = cobs_encode(raw, sizeof(raw), encoded);

    frame_t output;
    frame_status_t status = frame_decode(encoded, enc_len, &output);

    TEST_ASSERT_NOT_EQUAL(FRAME_OK, status);
    // TEST_ASSERT_EQUAL(FRAME_ERR_LEN, status);
}

// main function to run the tests
int main(void) {

    UNITY_BEGIN();

    RUN_TEST(test_crc_known);
    RUN_TEST(test_crc_different_data);
    RUN_TEST(test_crc_empty_data);
    RUN_TEST(test_frame_reject_crc_error);
    RUN_TEST(test_cobs_encode_decode);
    RUN_TEST(test_cobs_invalid_input);
    RUN_TEST(test_cobs_decode_null_input);
    RUN_TEST(test_cobs_max_block_size);
    RUN_TEST(test_cobs_internal_zero);
    RUN_TEST(test_frame_reject_cobs_invalid);
    RUN_TEST(test_frame_encode_decode_servo);
    RUN_TEST(test_frame_decode_rejects_unknown_msg_id);
    RUN_TEST(test_message_sizes);
    RUN_TEST(test_cobs_255_nonzero_bytes);
    RUN_TEST(test_frame_encode_decode_imu);
    RUN_TEST(test_frame_refuse_too_short);
    RUN_TEST(test_frame_decode_rejects_unknown_msg_id);
    RUN_TEST(test_frame_decode_rejects_wrong_payload_len);
    
    return UNITY_END();
}