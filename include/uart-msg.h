#ifndef UART_MSGS
#define UART_MSGS

#include <assert.h>
#include <stdint.h>

typedef uint8_t msg_id_t;

#pragma pack(push , 1)
typedef struct {
    uint32_t timestamp;
    int16_t ax, ay, az;
    int16_t gx, gy, gz;
} ImuMsg;
static_assert(sizeof(ImuMsg) == 16, "ImuMsg wire size changed");

typedef struct {
    int8_t angle_deg;
} ServoMsg;
static_assert(sizeof(ServoMsg) == 1, "ServoMsg wire size changed");

#pragma pack(pop)

/**
 * define a list of messages, with a name, the corresponding id byte, and the Msg struct.
 *
 * ID ranges:
 *   0x01 - 0x3F  STM -> Pi, telemetry
 *   0x40 - 0x7F  Pi -> STM, periodic commands
 *   0x80 - 0xBF  Pi -> STM, one-shot commands (ACKed)
 *   0xF0 - 0xFF  system (ACK, NACK, heartbeat)
 */
#define MSG_LIST(X)                     \
    X(IMU, 0x01, ImuMsg)                \
    X(SERVO, 0x40, ServoMsg)

/*
 * Message ID constants, one per entry of MSG_LIST. X generates:
 *     MSG_ID_IMU = 0x01,
 *     MSG_ID_SERVO = 0x40,
 * The values always fit in a msg_id_t (1 byte), which is what goes on the wire.
 */
enum {
#define X(name, id, type) MSG_ID_##name = id,
    MSG_LIST(X)
#undef X
};

/*
 * Returns the expected payload size (in bytes) of the message with the given
 * ID, or -1 if the ID is unknown. Used by the decoder to reject bad frames.
 *
 * For every entry of MSG_LIST, X generates one line:
 *     case 0x01: return (int)sizeof(ImuMsg);
 *     case 0x40: return (int)sizeof(ServoMsg);
 */
static inline int msg_payload_size(msg_id_t msg_id) {    
    switch (msg_id) {

        #define X(name, id, type) \
            case id:              \
                return (int)sizeof(type);
                MSG_LIST(X) // chiamo la macro sopra definita, che ha come parametro la x che definisco ora.
        #undef X
        
        default:
            return -1;
    }
}

/*
 * Union with one member per message, each as large as that message's struct.
 * A union is as large as its biggest member, so its sizeof is the size of the
 * largest payload. Never used to store data, only to compute the size.
 *
 * For every entry of MSG_LIST, X generates one member:
 *     uint8_t IMU[sizeof(ImuMsg)];
 *     uint8_t SERVO[sizeof(ServoMsg)];
 */
typedef union {
#define X(name, id, type) uint8_t name[sizeof(type)];
    MSG_LIST(X)
#undef X
} msg_any_payload_t;

/* Size in bytes of the largest message payload, used to size the buffers */
#define MSG_MAX_PAYLOAD_SIZE sizeof(msg_any_payload_t)


#endif // UART_MSGS

