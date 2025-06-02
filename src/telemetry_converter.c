#include "telemetry_converter.h"
#include "config.h"
#include "pico/stdlib.h"
#include <string.h>

static telemetry_data_t telemetry_data;

void telemetry_converter_init(void) {
    memset(&telemetry_data, 0, sizeof(telemetry_data));
}

int32_t frsky_gps_to_decimal(uint32_t frsky_coord) {
    uint32_t degrees = frsky_coord / 1000000;
    uint32_t minutes = (frsky_coord % 1000000) / 10000;
    uint32_t minutes_frac = frsky_coord % 10000;
    
    int32_t decimal = degrees * 10000000;
    decimal += (minutes * 10000000) / 60;
    decimal += (minutes_frac * 1000) / 60;
    
    return decimal;
}

uint16_t frsky_voltage_to_mv(uint32_t frsky_voltage) {
    return (uint16_t)(frsky_voltage * 100);
}

uint16_t frsky_current_to_ma(uint32_t frsky_current) {
    return (uint16_t)(frsky_current * 100);
}

int16_t frsky_vspeed_to_cms(uint32_t frsky_vspeed) {
    int32_t signed_vspeed = (int32_t)frsky_vspeed;
    if (signed_vspeed > 0x7FFFFFFF) {
        signed_vspeed = signed_vspeed - 0x100000000;
    }
    return (int16_t)signed_vspeed;
}

uint16_t frsky_altitude_to_meters(uint32_t frsky_altitude) {
    return (uint16_t)(frsky_altitude / 10);
}

void update_telemetry_data(const frsky_sport_packet_t *frsky_packet) {
    uint32_t now = time_us_32();
    
    switch (frsky_packet->data_id) {
        case FRSKY_ID_GPS_LONG_LATI:
            if (frsky_packet->value & 0x80000000) {
                telemetry_data.longitude = frsky_gps_to_decimal(frsky_packet->value & 0x7FFFFFFF);
                if (frsky_packet->value & 0x40000000) {
                    telemetry_data.longitude = -telemetry_data.longitude;
                }
            } else {
                telemetry_data.latitude = frsky_gps_to_decimal(frsky_packet->value & 0x3FFFFFFF);
                if (frsky_packet->value & 0x40000000) {
                    telemetry_data.latitude = -telemetry_data.latitude;
                }
            }
            telemetry_data.gps_valid = true;
            telemetry_data.last_gps_update = now;
            break;
            
        case FRSKY_ID_GPS_ALT:
            telemetry_data.gps_altitude = frsky_altitude_to_meters(frsky_packet->value) + 1000;
            telemetry_data.gps_valid = true;
            telemetry_data.last_gps_update = now;
            break;
            
        case FRSKY_ID_GPS_SPEED:
            telemetry_data.gps_speed = (uint16_t)((frsky_packet->value * 1852) / 10000);
            telemetry_data.gps_valid = true;
            telemetry_data.last_gps_update = now;
            break;
            
        case FRSKY_ID_GPS_COURS:
            telemetry_data.gps_heading = (uint16_t)(frsky_packet->value / 100);
            telemetry_data.gps_valid = true;
            telemetry_data.last_gps_update = now;
            break;
            
        case FRSKY_ID_VFAS:
            telemetry_data.voltage = frsky_voltage_to_mv(frsky_packet->value);
            telemetry_data.battery_valid = true;
            telemetry_data.last_battery_update = now;
            break;
            
        case FRSKY_ID_CURR:
            telemetry_data.current = frsky_current_to_ma(frsky_packet->value);
            telemetry_data.battery_valid = true;
            telemetry_data.last_battery_update = now;
            break;
            
        case FRSKY_ID_FUEL:
            telemetry_data.fuel_percent = (uint8_t)frsky_packet->value;
            telemetry_data.battery_valid = true;
            telemetry_data.last_battery_update = now;
            break;
            
        case FRSKY_ID_ALT:
            telemetry_data.altitude = (int32_t)(frsky_packet->value / 10);
            telemetry_data.altitude_valid = true;
            telemetry_data.last_altitude_update = now;
            break;
            
        case FRSKY_ID_VSPD:
            telemetry_data.vertical_speed = frsky_vspeed_to_cms(frsky_packet->value);
            telemetry_data.vario_valid = true;
            telemetry_data.last_vario_update = now;
            break;
    }
}

bool create_crsf_from_telemetry(uint8_t crsf_type, crsf_packet_t *crsf_packet) {
    uint32_t now = time_us_32();
    const uint32_t timeout_us = TELEMETRY_TIMEOUT_US;
    
    switch (crsf_type) {
        case CRSF_FRAMETYPE_GPS:
            if (telemetry_data.gps_valid && (now - telemetry_data.last_gps_update) < timeout_us) {
                crsf_gps_t gps_data = {
                    .latitude = telemetry_data.latitude,
                    .longitude = telemetry_data.longitude,
                    .groundspeed = telemetry_data.gps_speed,
                    .heading = telemetry_data.gps_heading,
                    .altitude = telemetry_data.gps_altitude,
                    .satellites = telemetry_data.satellites
                };
                return crsf_create_gps_packet(&gps_data, crsf_packet);
            }
            break;
            
        case CRSF_FRAMETYPE_BATTERY_SENSOR:
            if (telemetry_data.battery_valid && (now - telemetry_data.last_battery_update) < timeout_us) {
                crsf_battery_t battery_data = {
                    .voltage = telemetry_data.voltage,
                    .current = telemetry_data.current,
                    .capacity = telemetry_data.capacity_used,
                    .remaining = telemetry_data.fuel_percent
                };
                return crsf_create_battery_packet(&battery_data, crsf_packet);
            }
            break;
            
        case CRSF_FRAMETYPE_VARIO:
            if (telemetry_data.vario_valid && (now - telemetry_data.last_vario_update) < timeout_us) {
                crsf_vario_t vario_data = {
                    .vertical_speed = telemetry_data.vertical_speed
                };
                return crsf_create_vario_packet(&vario_data, crsf_packet);
            }
            break;
            
        case CRSF_FRAMETYPE_BARO_ALT:
            if (telemetry_data.altitude_valid && (now - telemetry_data.last_altitude_update) < timeout_us) {
                crsf_baro_alt_t baro_data = {
                    .altitude = (uint16_t)(telemetry_data.altitude + 10000),
                    .vertical_speed = telemetry_data.vertical_speed
                };
                return crsf_create_baro_alt_packet(&baro_data, crsf_packet);
            }
            break;
    }
    
    return false;
}

bool convert_frsky_to_crsf(const frsky_sport_packet_t *frsky_packet, crsf_packet_t *crsf_packet) {
    update_telemetry_data(frsky_packet);
    
    switch (frsky_packet->data_id) {
        case FRSKY_ID_GPS_LONG_LATI:
        case FRSKY_ID_GPS_ALT:
        case FRSKY_ID_GPS_SPEED:
        case FRSKY_ID_GPS_COURS:
            return create_crsf_from_telemetry(CRSF_FRAMETYPE_GPS, crsf_packet);
            
        case FRSKY_ID_VFAS:
        case FRSKY_ID_CURR:
        case FRSKY_ID_FUEL:
            return create_crsf_from_telemetry(CRSF_FRAMETYPE_BATTERY_SENSOR, crsf_packet);
            
        case FRSKY_ID_VSPD:
            return create_crsf_from_telemetry(CRSF_FRAMETYPE_VARIO, crsf_packet);
            
        case FRSKY_ID_ALT:
            return create_crsf_from_telemetry(CRSF_FRAMETYPE_BARO_ALT, crsf_packet);
            
        default:
            return false;
    }
}
