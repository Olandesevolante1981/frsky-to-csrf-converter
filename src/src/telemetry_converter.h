#ifndef TELEMETRY_CONVERTER_H
#define TELEMETRY_CONVERTER_H

#include "frsky_sport.h"
#include "crsf.h"
#include <stdbool.h>

// Telemetry data storage
typedef struct {
    // GPS data
    int32_t latitude;
    int32_t longitude;
    uint16_t gps_altitude;
    uint16_t gps_speed;
    uint16_t gps_heading;
    uint8_t satellites;
    bool gps_valid;
    
    // Battery data
    uint16_t voltage;
    uint16_t current;
    uint32_t capacity_used;
    uint8_t fuel_percent;
    bool battery_valid;
    
    // Altitude/Vario data
    int32_t altitude;
    int16_t vertical_speed;
    bool altitude_valid;
    bool vario_valid;
    
    // Timestamps for data freshness
    uint32_t last_gps_update;
    uint32_t last_battery_update;
    uint32_t last_altitude_update;
    uint32_t last_vario_update;
} telemetry_data_t;

// Function prototypes
void telemetry_converter_init(void);
bool convert_frsky_to_crsf(const frsky_sport_packet_t *frsky_packet, crsf_packet_t *crsf_packet);
void update_telemetry_data(const frsky_sport_packet_t *frsky_packet);
bool create_crsf_from_telemetry(uint8_t crsf_type, crsf_packet_t *crsf_packet);

// Utility functions
int32_t frsky_gps_to_decimal(uint32_t frsky_coord);
uint16_t frsky_voltage_to_mv(uint32_t frsky_voltage);
uint16_t frsky_current_to_ma(uint32_t frsky_current);
int16_t frsky_vspeed_to_cms(uint32_t frsky_vspeed);
uint16_t frsky_altitude_to_meters(uint32_t frsky_altitude);

#endif // TELEMETRY_CONVERTER_H
