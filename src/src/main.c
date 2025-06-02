#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "hardware/flash.h"
#include "hardware/sync.h"
#include "config.h"
#include "frsky_sport.h"
#include "crsf.h"
#include "telemetry_converter.h"

// Buffer for incoming FrSky data
static uint8_t frsky_buffer[FRSKY_BUFFER_SIZE];
static volatile uint16_t frsky_buffer_head = 0;
static volatile uint16_t frsky_buffer_tail = 0;

// Configuration storage
#define CONFIG_FLASH_OFFSET (256 * 1024)
#define CONFIG_MAGIC 0x46525343

typedef struct {
    uint32_t magic;
    uint16_t frsky_tx_pin;
    uint16_t frsky_rx_pin;
    uint32_t frsky_baud_rate;
    uint16_t crsf_tx_pin;
    uint16_t crsf_rx_pin;
    uint32_t crsf_baud_rate;
    uint16_t led_pin;
    uint32_t heartbeat_interval_us;
    uint32_t led_blink_interval_us;
    uint8_t debug_enabled;
    uint8_t reserved[32];
} config_data_t;

static config_data_t current_config = {
    .magic = CONFIG_MAGIC,
    .frsky_tx_pin = FRSKY_TX_PIN,
    .frsky_rx_pin = FRSKY_RX_PIN,
    .frsky_baud_rate = FRSKY_BAUD_RATE,
    .crsf_tx_pin = CRSF_TX_PIN,
    .crsf_rx_pin = CRSF_RX_PIN,
    .crsf_baud_rate = CRSF_BAUD_RATE,
    .led_pin = LED_PIN,
    .heartbeat_interval_us = HEARTBEAT_INTERVAL_US,
    .led_blink_interval_us = LED_BLINK_INTERVAL_US,
    .debug_enabled = DEBUG_ENABLED
};

// Statistics
static uint32_t frsky_packets_received = 0;
static uint32_t frsky_packets_valid = 0;
static uint32_t crsf_packets_sent = 0;

// UART interrupt handler
void on_frsky_uart_rx() {
    while (uart_is_readable(FRSKY_UART_ID)) {
        uint8_t ch = uart_getc(FRSKY_UART_ID);
        uint16_t next_head = (frsky_buffer_head + 1) % sizeof(frsky_buffer);
        if (next_head != frsky_buffer_tail) {
            frsky_buffer[frsky_buffer_head] = ch;
            frsky_buffer_head = next_head;
        }
    }
}

// Load configuration from flash
void load_config() {
    const config_data_t *flash_config = (const config_data_t *)(XIP_BASE + CONFIG_FLASH_OFFSET);
    if (flash_config->magic == CONFIG_MAGIC) {
        current_config = *flash_config;
        if (current_config.debug_enabled) {
            printf("Configuration loaded from flash\n");
        }
    }
}

// Save configuration to flash
void save_config() {
    uint32_t interrupts = save_and_disable_interrupts();
    flash_range_erase(CONFIG_FLASH_OFFSET, FLASH_SECTOR_SIZE);
    flash_range_program(CONFIG_FLASH_OFFSET, (const uint8_t*)&current_config, sizeof(current_config));
    restore_interrupts(interrupts);
    
    if (current_config.debug_enabled) {
        printf("Configuration saved to flash\n");
    }
}

// Initialize UARTs
void init_uarts() {
    // FrSky UART
    uart_init(FRSKY_UART_ID, current_config.frsky_baud_rate);
    gpio_set_function(current_config.frsky_tx_pin, GPIO_FUNC_UART);
    gpio_set_function(current_config.frsky_rx_pin, GPIO_FUNC_UART);
    uart_set_format(FRSKY_UART_ID, 8, 1, UART_PARITY_NONE);
    
    // Enable RX interrupt
    irq_set_exclusive_handler(UART0_IRQ, on_frsky_uart_rx);
    irq_set_enabled(UART0_IRQ, true);
    uart_set_irq_enables(FRSKY_UART_ID, true, false);
    
    // CRSF UART
    uart_init(CRSF_UART_ID, current_config.crsf_baud_rate);
    gpio_set_function(current_config.crsf_tx_pin, GPIO_FUNC_UART);
    gpio_set_function(current_config.crsf_rx_pin, GPIO_FUNC_UART);
    uart_set_format(CRSF_UART_ID, 8, 1, UART_PARITY_NONE);
}

// Get next byte from FrSky buffer
bool get_frsky_byte(uint8_t *byte) {
    if (frsky_buffer_tail == frsky_buffer_head) {
        return false;
    }
    
    *byte = frsky_buffer[frsky_buffer_tail];
    frsky_buffer_tail = (frsky_buffer_tail + 1) % sizeof(frsky_buffer);
    return true;
}

// Send CRSF packet
void send_crsf_packet(const uint8_t *data, uint8_t length) {
    uart_write_blocking(CRSF_UART_ID, data, length);
    crsf_packets_sent++;
}

// Configuration menu
void print_config_menu() {
    printf("\n=== FrSky to CRSF Converter Configuration ===\n");
    printf("1. FrSky TX Pin: %d\n", current_config.frsky_tx_pin);
    printf("2. FrSky RX Pin: %d\n", current_config.frsky_rx_pin);
    printf("3. FrSky Baud Rate: %d\n", current_config.frsky_baud_rate);
    printf("4. CRSF TX Pin: %d\n", current_config.crsf_tx_pin);
    printf("5. CRSF RX Pin: %d\n", current_config.crsf_rx_pin);
    printf("6. CRSF Baud Rate: %d\n", current_config.crsf_baud_rate);
    printf("7. LED Pin: %d\n", current_config.led_pin);
    printf("8. Debug Enabled: %s\n", current_config.debug_enabled ? "Yes" : "No");
    printf("\nCommands:\n");
    printf("s - Save configuration\n");
    printf("r - Reset to defaults\n");
    printf("t - Show statistics\n");
    printf("x - Exit configuration\n");
    printf("\nEnter option: ");
}

// Handle configuration input
void handle_config_input() {
    int ch = getchar_timeout_us(0);
    if (ch == PICO_ERROR_TIMEOUT) return;
    
    static bool in_config_mode = false;
    
    if (!in_config_mode && ch == 'c') {
        in_config_mode = true;
        print_config_menu();
        return;
    }
    
    if (!in_config_mode) return;
    
    switch (ch) {
        case 's':
            save_config();
            printf("Configuration saved!\n");
            print_config_menu();
            break;
            
        case 'r':
            current_config.frsky_tx_pin = FRSKY_TX_PIN;
            current_config.frsky_rx_pin = FRSKY_RX_PIN;
            current_config.frsky_baud_rate = FRSKY_BAUD_RATE;
            current_config.crsf_tx_pin = CRSF_TX_PIN;
            current_config.crsf_rx_pin = CRSF_RX_PIN;
            current_config.crsf_baud_rate = CRSF_BAUD_RATE;
            current_config.led_pin = LED_PIN;
            current_config.debug_enabled = DEBUG_ENABLED;
            printf("Configuration reset to defaults!\n");
            print_config_menu();
            break;
            
        case 't':
            printf("\n=== Statistics ===\n");
            printf("FrSky packets received: %d\n", frsky_packets_received);
            printf("FrSky packets valid: %d\n", frsky_packets_valid);
            printf("CRSF packets sent: %d\n", crsf_packets_sent);
            printf("Success rate: %.1f%%\n", 
                   frsky_packets_received > 0 ? 
                   (100.0 * frsky_packets_valid / frsky_packets_received) : 0.0);
            print_config_menu();
            break;
            
        case 'x':
            in_config_mode = false;
            printf("Exiting configuration mode\n");
            break;
            
        default:
            print_config_menu();
            break;
    }
}

int main() {
    stdio_init_all();
    
    // Load configuration
    load_config();
    
    // Initialize LED
    gpio_init(current_config.led_pin);
    gpio_set_dir(current_config.led_pin, GPIO_OUT);
    gpio_put(current_config.led_pin, 1);
    
    // Initialize UARTs
    init_uarts();
    
    // Initialize telemetry systems
    frsky_sport_init();
    crsf_init();
    telemetry_converter_init();
    
    if (current_config.debug_enabled) {
        printf("FrSky S.PORT to CRSF Converter Started\n");
        printf("Press 'c' for configuration menu\n");
        printf("FrSky: GPIO%d/%d @ %d baud\n", 
               current_config.frsky_tx_pin, current_config.frsky_rx_pin, current_config.frsky_baud_rate);
        printf("CRSF: GPIO%d/%d @ %d baud\n", 
               current_config.crsf_tx_pin, current_config.crsf_rx_pin, current_config.crsf_baud_rate);
    }
    
    uint32_t last_heartbeat = 0;
    uint8_t byte;
    
    while (1) {
        // Handle configuration
        handle_config_input();
        
        // Process FrSky data
        while (get_frsky_byte(&byte)) {
            frsky_sport_process_byte(byte);
        }
        
        // Convert packets
        frsky_sport_packet_t frsky_packet;
        if (frsky_sport_get_packet(&frsky_packet)) {
            frsky_packets_received++;
            frsky_packets_valid++;
            
            if (current_config.debug_enabled && DEBUG_FRSKY_PACKETS) {
                printf("FrSky: ID=0x%04X, Value=0x%08X\n", 
                       frsky_packet.data_id, frsky_packet.value);
            }
            
            crsf_packet_t crsf_packet;
            if (convert_frsky_to_crsf(&frsky_packet, &crsf_packet)) {
                send_crsf_packet(crsf_packet.data, crsf_packet.length);
                
                if (current_config.debug_enabled && DEBUG_CRSF_PACKETS) {
                    printf("CRSF: Type=0x%02X, Length=%d\n", 
                           crsf_packet.data[2], crsf_packet.length);
                }
            }
        }
        
        // Send heartbeat
        uint32_t now = time_us_32();
        if (now - last_heartbeat > current_config.heartbeat_interval_us) {
            crsf_packet_t heartbeat;
            if (crsf_create_heartbeat(&heartbeat)) {
                send_crsf_packet(heartbeat.data, heartbeat.length);
            }
            last_heartbeat = now;
        }
        
        // Toggle LED
        static uint32_t last_led_toggle = 0;
        if (now - last_led_toggle > current_config.led_blink_interval_us) {
            gpio_xor_mask(1u << current_config.led_pin);
            last_led_toggle = now;
        }
        
        tight_loop_contents();
    }
    
    return 0;
}
