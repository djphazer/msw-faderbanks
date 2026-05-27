/*
 * consolidated core header
 */

#pragma once

#include <Arduino.h>
#include <SPI.h>
#include <elapsedMillis.h>
#include <MIDI.h>

#include <stdlib.h>
#include <stdint.h>
#include "settings.h"

#define BOOT_DOUBLE_TAP_ADDRESS   ((volatile uint32_t *)(HMCRAMC0_ADDR + HMCRAMC0_SIZE - 4))
#define BOOT_DOUBLE_TAP_DATA    (*((volatile uint32_t *)BOOT_DOUBLE_TAP_ADDRESS))
#define DOUBLE_TAP_MAGIC      0xf01669efUL // Randomly selected, adjusted to have first and last bit set

#define delay_us delayMicroseconds
#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))
#define LOOP(i, n) for (int i = 0; i < n; ++i)

constexpr int LED_PIN = 15; //PIN_PB08;
constexpr int LED_LATCH = 13; // PIN_PA17;
constexpr int SPI_MOSI_PIN = 10; // PIN_PA18;
constexpr int SPI_SCK_PIN = 12; // PIN_PA19;

#define MUX_SW_0 24 //PIN_PB11
#define MUX_SW_1 6 //PIN_PA20
#define MUX_SW_2 7 //PIN_PA21
#define MUX_SW_3 20 //PIN_PA22
#define ADC_PIN 14 //PIN_PA02
#define SWITCH_PIN 23 //PIN_PB10

#define TRSMIDI_A_RX 3 // PIN_PA09
#define TRSMIDI_A_TX 4 // PIN_PA08
#define TRSMIDI_B_RX 21 // PIN_PA23
#define TRSMIDI_B_TX 30 // PIN_PB22

const int OUTPUTS[] = {
  SPI_SCK_PIN, SPI_MOSI_PIN, LED_LATCH,
  MUX_SW_0, MUX_SW_1, MUX_SW_2, MUX_SW_3,
};

static constexpr int NUMBER_CHANNELS = 16;

/* Function Prototypes */
void to_bootloader(void);
void set_mux(void);

void midi_task(void);

void i2c_master_task(void);
void i2c_slave_task(void);
void i2c_scan_devices(void);

//static void cdc_task(void);

void process_value(void);

/*
 * firmware metadata
 * from config.h
 */
#define MAJOR_VERSION 0x01
#define MINOR_VERSION 0x01
#define POINT_VERSION 0x00

//amount of smoothing on slider values use 2,4,8,16,32,64 NOT 0!
#define AVERAGING 8

//how long to let the MUX settle before reading ADC. (Microseconds)
//long mux time with high averaging will be slow.
//value in microseconds us
#define MUXTIME 2

//amount of 12 ADC noise to ignore before changing value
#define THRESHOLD 10

#define ANSIABLE_ADDRESS 0x20
#define ER301_ADDRESS 0x31
#define TXO_ADDRESS 0x60


struct switch_state_t {
  uint8_t state;
  uint8_t count;
};
struct sysex_buffer_t {
  static constexpr size_t BUFFER_SZ = 256;
  uint8_t buffer[BUFFER_SZ];
  size_t length = 0;
  void append(uint8_t *data, size_t size) {
    if (length + size < BUFFER_SZ) {
      memcpy(buffer + length, data, size);
      length += size;
    }
  }
};

enum led_color_t : uint32_t {
  LED_OFF,
  LED_RED,
  LED_GREEN,
  LED_ORANGE,
};

void leds_init(void);
void update_leds(void);
void set_led(uint8_t led, uint32_t color);
void set_led_level(uint8_t led, uint8_t level);
void set_led_blend(uint8_t led, uint8_t green);
void cycle_led(uint8_t led);
void register_timer_events(void);

uint16_t fader_max(void);
uint16_t fader_min(void);
uint16_t read_fader();

void send_current_configuration(void);
void process_sysex(uint8_t const* sysex_data, uint32_t size);
