#pragma once
#include "core.h"

struct configuration_settings  {
  uint8_t led_on;
  uint8_t led_flash;
  uint8_t controller_flip;
  uint8_t i2c_master;
  uint8_t fader_min_msb;
  uint8_t fader_min_lsb;
  uint8_t fader_max_msb;
  uint8_t fader_max_lsb;
  uint8_t midi_thru;
  uint8_t delay_msb;
  uint8_t delay_lsb;
  uint8_t i2c_slave_address;
  uint8_t high_resolution;
  uint8_t byte_14;
  uint8_t byte_15;
  uint8_t byte_16;
};

struct device_configuration {
  configuration_settings configuration;
  uint8_t usb_channel[16];
  uint8_t trs_channel[16];
  uint8_t usb_control[16];
  uint8_t trs_control[16];
};

extern device_configuration device_config;
extern uint8_t device_id;
extern uint8_t channels_count;

void settings_init(void);
void factory_settings_init(void);
bool load_settings(void);
void update_all_settings(uint8_t const* sysex_data, uint32_t size);
uint16_t get_boot_delay_value(void);
uint16_t get_fader_max(void);
uint16_t get_fader_min(void);
