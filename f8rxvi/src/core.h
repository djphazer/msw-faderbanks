/*
 * Core header for F8R/XVI
 * Sets up global constants, function signatures, and helpers
 */
#pragma once

// hardware framework, and peripheral drivers
#include <Arduino.h> // also pulls in stdlib, stdint stuff
#include <SPI.h>
#include <Wire.h>
#include <MIDI.h>

// fka configuration - eeprom storage stuff
#include "settings.h"

#define BOOT_DOUBLE_TAP_ADDRESS		((volatile uint32_t *)(HMCRAMC0_ADDR + HMCRAMC0_SIZE - 4))
#define BOOT_DOUBLE_TAP_DATA		(*((volatile uint32_t *)BOOT_DOUBLE_TAP_ADDRESS))
#define DOUBLE_TAP_MAGIC			0xf01669efUL // Randomly selected, adjusted to have first and last bit set

/* Function Prototypes */
void to_bootloader(void);
void set_mux(void);
void midi_task(void);
void i2c_master_task(void);
void i2c_slave_task(void);
void i2c_scan_devices(void);
void process_value(void);
void cdc_task(void);

void system_init(void);

struct i2c_packet {
  uint8_t cmd;
  uint8_t port;
  uint8_t value_h;
  uint8_t value_l;
};
void i2c_delay(void);

struct sysex_buffer_t {
  uint8_t buffer[256];
  uint8_t length;
};
void send_current_configuration(void);


// --- compatibility macros
#define delay_ms delay
#define delay_us delayMicroseconds

/*
 * firmware metadata
 */
#define MAJOR_VERSION 0x02
#define MINOR_VERSION 0x02
#define POINT_VERSION 0x00

//amount of smoothing on slider values use 2,4,8,16,32,64 NOT 0!
#define AVERAGING 8

//how long to let the MUX settle before reading ADC. (Microseconds)
//long mux time with high averaging will be slow.
#define MUXTIME 2
//amount of 12 ADC noise to ignore before changing value
#define THRESHOLD 10
//DIP switch (all three entries below)
#define ANSIABLE_ADDRESS 0x20
#define ER301_ADDRESS 0x31
#define TXO_ADDRESS	0x60

// --- Pin aliases
#define ADC_PIN 14 //PIN_PA02
// default Wire pins
/*#define SDA PIN_PA08*/
/*#define SCL PIN_PA09*/

// 4 dipswitches on the back
#define SW1_1 1 //PIN_PA10
#define SW1_2 0 //PIN_PA11
#define SW1_3 11 //PIN_PA16
#define SW1_4 13 //PIN_PA17

#define V3_ENABLE 12 //PIN_PA19
#define MUX_SW_0 24 //PIN_PB11
#define MUX_SW_1 6 //PIN_PA20
#define MUX_SW_2 7 //PIN_PA21
#define MUX_SW_3 20 //PIN_PA22

// override LED pin
#undef LED_BUILTIN
#define LED_BUILTIN 15 // PIN_PB08

const uint8_t INPUTS[] = {
  SW1_1, SW1_2, SW1_3, SW1_4, ADC_PIN,
};
const uint8_t OUTPUTS[] = {
  MUX_SW_0, MUX_SW_1, MUX_SW_2, MUX_SW_3,
  V3_ENABLE
};
