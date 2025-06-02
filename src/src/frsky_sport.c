#include "frsky_sport.h"
#include <string.h>

static frsky_sport_state_t frsky_state = FRSKY_STATE_IDLE;
static frsky_sport_packet_t current_packet;
static uint8_t packet_buffer[FRSKY_SPORT_PACKET_SIZE];
static uint8_t packet_index = 0;
static bool packet_ready = false;

void frsky_sport_init(void) {
    frsky_state = FRSKY_STATE_IDLE;
    packet_ready = false;
    packet_index = 0;
    memset(&current_packet, 0, sizeof(current_packet));
}

uint8_t frsky_sport_crc(const uint8_t *data, uint8_t length) {
    uint16_t crc = 0;
    for (uint8_t i = 0; i < length; i++) {
        crc += data[i];
        crc += crc >> 8;
        crc &= 0xFF;
    }
    return 0xFF - crc;
}

uint8_t frsky_sport_unstuff_byte(uint8_t byte) {
    if (byte == 0x5E) {
        return 0x7E;
    } else if (byte == 0x5D) {
        return 0x7D;
    }
    return byte;
}

void frsky_sport_process_byte(uint8_t byte) {
    static bool escape_next = false;
    
    switch (frsky_state) {
        case FRSKY_STATE_IDLE:
            if (byte == FRSKY_SPORT_START_BYTE) {
                frsky_state = FRSKY_STATE_START;
                packet_index = 0;
                escape_next = false;
            }
            break;
            
        case FRSKY_STATE_START:
            if (byte == FRSKY_SPORT_START_BYTE) {
                packet_index = 0;
            } else {
                if (byte == 0x7D) {
                    escape_next = true;
                } else {
                    if (escape_next) {
                        byte = frsky_sport_unstuff_byte(byte);
                        escape_next = false;
                    }
                    packet_buffer[packet_index++] = byte;
                    frsky_state = FRSKY_STATE_DATA;
                }
            }
            break;
            
        case FRSKY_STATE_DATA:
            if (byte == 0x7D && !escape_next) {
                escape_next = true;
                return;
            }
            
            if (escape_next) {
                byte = frsky_sport_unstuff_byte(byte);
                escape_next = false;
            }
            
            packet_buffer[packet_index++] = byte;
            
            if (packet_index >= FRSKY_SPORT_PACKET_SIZE) {
                uint8_t calculated_crc = frsky_sport_crc(packet_buffer, FRSKY_SPORT_PACKET_SIZE - 1);
                if (calculated_crc == packet_buffer[FRSKY_SPORT_PACKET_SIZE - 1]) {
                    current_packet.sensor_id = packet_buffer[0];
                    current_packet.frame_id = packet_buffer[1];
                    current_packet.data_id = (packet_buffer[3] << 8) | packet_buffer[2];
                    current_packet.value = (packet_buffer[7] << 24) | (packet_buffer[6] << 16) | 
                                         (packet_buffer[5] << 8) | packet_buffer[4];
                    current_packet.valid = true;
                    packet_ready = true;
                }
                frsky_state = FRSKY_STATE_IDLE;
            }
            break;
    }
}

bool frsky_sport_get_packet(frsky_sport_packet_t *packet) {
    if (packet_ready && current_packet.valid) {
        *packet = current_packet;
        packet_ready = false;
        current_packet.valid = false;
        return true;
    }
    return false;
}
