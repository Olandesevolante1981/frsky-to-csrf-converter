#ifndef CRSF_H
#define CRSF_H

#include <stdint.h>
#include <stdbool.h>

// CRSF protocol constants
#define CRSF_MAX_PACKET_SIZE 64

// CRSF frame types
#define CRSF_FRAMETYPE_GPS 0x02
#define CRSF_FRAMETYPE_VARIO 0x07
#define CRSF_FRAMETYPE_BATTERY_SENSOR 0x08
#define CRSF_FRAMETYPE_BARO_ALT 0x09
#define CRSF_FRAMETYPE_HEARTBEAT 0x0B

// CRSF addresses
#define CRSF_ADDRESS_FLIGHT_CONTROLLER 0xC8

typedef struct {
    uint8_t data[CRSF_MAX_PACKET_SIZE];
    uint8_t length;
} crsf_packet_t;

// CRSF telemetry structures
typedef struct {
    int32_t latitude;   // degrees * 1e7
    int32_t longitude;  // degrees * 1e7
    uint16_t groundspeed; // km/h * 100
    uint16_t heading;   // degrees * 100
    uint16_t altitude;  // meters + 1000
    uint8_t satellites;
} __attribute__((packed)) crsf_gps_t;

typedef struct {
    int16_t vertical_speed; // cm/s
} __attribute__((packed)) crsf_vario_t;

typedef struct {
    uint16_t voltage;    // mV
    uint16_t current;    // mA
    uint32_t capacity;   // mAh
    uint8_t remaining;   // %
} __attribute__((packed)) crsf_battery_t;

typedef struct {
    uint16_t altitude;   // meters + 10000
    int16_t vertical_speed; // cm/s
} __attribute__((packed)) crsf_baro_alt_t;

// Function prototypes
void crsf_init(void);
uint8_t crsf_crc8(const uint8_t *data, uint8_t length);
bool crsf_create_packet(uint8_t type, const void *payload, uint8_t payload_size, crsf_packet_t *packet);
bool crsf_create_gps_packet(const crsf_gps_t *gps, crsf_packet_t *packet);
bool crsf_create_vario_packet(const crsf_vario_t *vario, crsf_packet_t *packet);
bool crsf_create_battery_packet(const crsf_battery_t *battery, crsf_packet_t *packet);
bool crsf_create_baro_alt_packet(const crsf_baro_alt_t *baro, crsf_packet_t *packet);
bool crsf_create_heartbeat(crsf_packet_t *packet);

#endif // CRSF_H
