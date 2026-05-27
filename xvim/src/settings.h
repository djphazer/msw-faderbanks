/*
 * Config settings data structures
 * These define the binary format stored in flash, and transferred via Sysex
 */
#pragma once

#include <Arduino.h>

// actual Sysex data, only 7-bit numbers round here

enum modes_t : uint8_t {
  MOMENTARY = 0,
  TOGGLE_2  = 1,
  TOGGLE_3  = 2,
  TOGGLE_4  = 3
};

enum midi_ports : uint8_t {
  USB_PORT,
  TRS_PORT_A,
  TRS_PORT_B
};

struct configuration_settings {
  uint8_t led_on;
  uint8_t led_flash;
  uint8_t controller_flip;
  uint8_t i2c_master;
  uint8_t fader_min_msb;
  uint8_t fader_min_lsb;
  uint8_t fader_max_msb;
  uint8_t fader_max_lsb;
  uint8_t midi_thru;
  uint8_t high_resolution;
  uint8_t byte_11;
  uint8_t byte_12;
  uint8_t byte_13;
  uint8_t byte_14;
  uint8_t byte_15;
  uint8_t byte_16;
};

struct device_configuration {
  configuration_settings configuration;
  uint8_t midi_port[16];
  uint8_t channel[16];
  uint8_t control[16];
  modes_t sw_mode[16];
  uint8_t sw_control[16][4];
  uint8_t sw_value[16][4];
  uint8_t sw_position[16];
};

extern device_configuration device_config;
extern uint8_t device_id;

void settings_init(void);
void factory_settings_init(void);
bool load_settings(void);
void update_all_settings(uint8_t const* sysex_data, uint32_t size);
uint16_t get_boot_delay_value(void);

