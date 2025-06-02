#ifndef FRSKY_SPORT_H
#define FRSKY_SPORT_H

#include <stdint.h>
#include <stdbool.h>

// FrSky S.PORT protocol constants
#define FRSKY_SPORT_START_BYTE 0x7E
#define FRSKY_SPORT_PACKET_SIZE 9

// FrSky data IDs
#define FRSKY_ID_VFAS 0x0210    // Battery voltage
#define FRSKY_ID_CURR 0x0200    // Current
#define FRSKY_ID_VSPD 0x0110    // Vertical speed
#define FRSKY_ID_ALT 0x0100     // Altitude
#define FRSKY_ID_GPS_LONG_LATI 0x0800 // GPS coordinates
#define FRSKY_ID_GPS_ALT 0x0820 // GPS altitude
#define FRSKY_ID_GPS_SPEED 0x0830 // GPS speed
#define FRSKY_ID_GPS_COURS 0x0840 // GPS course
#define FRSKY_ID_FUEL 0x0600    // Fuel level
#define FRSKY_ID_RPM 0x0500     // RPM
#define FRSKY_ID_TEMP1 0x0401   // Temperature 1
#define FRSKY_ID_TEMP2 0x0402   // Temperature 2

typedef struct {
    uint8_t sensor_id;
    uint8_t frame_id;
    uint16_t data_id;
    uint32_t value;
    bool valid;
} frsky_sport_packet_t;

typedef enum {
    FRSKY_STATE_IDLE,
    FRSKY_STATE_START,
    FRSKY_STATE_DATA
} frsky_sport_state_t;

// Function prototypes
void frsky_sport_init(void);
void frsky_sport_process_byte(uint8_t byte);
bool frsky_sport_get_packet(frsky_sport_packet_t *packet);
uint8_t frsky_sport_crc(const uint8_t *data, uint8_t length);
uint8_t frsky_sport_unstuff_byte(uint8_t byte);

#endif // FRSKY_SPORT_H
