#include "core.h"
#include <FlashStorage_SAMD.h>

// FlashStorage(storage, device_configuration);
FlashStorageClass<device_configuration> storage((void*)0x10000);

device_configuration device_config;
uint8_t device_id;

device_configuration factory_settings = {
    .configuration = {
      .led_on = 1,
      .led_flash = 1,
      .controller_flip = 0,
      .i2c_master = 0,
      .fader_min_msb = 20,
      .fader_min_lsb = 0,
      .fader_max_msb = 110,
      .fader_max_lsb = 31,
      .midi_thru = 1,
      .high_resolution = 1,
      .byte_11 = 4,
      .byte_12 = 5,
      .byte_13 = 6,
      .byte_14 = 7,
      .byte_15 = 8,
      .byte_16 = 9,
    },
    .midi_port = {USB_PORT, USB_PORT, USB_PORT, USB_PORT, USB_PORT, USB_PORT,
                  USB_PORT, USB_PORT, USB_PORT, USB_PORT, USB_PORT, USB_PORT,
                  USB_PORT, USB_PORT, USB_PORT, USB_PORT},
    .channel = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16},
    .control = {9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9},
    .sw_mode = {TOGGLE_2, TOGGLE_2, TOGGLE_2, TOGGLE_2, TOGGLE_2, TOGGLE_2,
                TOGGLE_2, TOGGLE_2, TOGGLE_2, TOGGLE_2, TOGGLE_2, TOGGLE_2,
                TOGGLE_2, TOGGLE_2, TOGGLE_2, TOGGLE_2},
    .sw_control = {{80, 80, 81, 82},
                   {80, 80, 81, 82},
                   {80, 80, 81, 82},
                   {80, 80, 81, 82},
                   {80, 80, 81, 82},
                   {80, 80, 81, 82},
                   {80, 80, 81, 82},
                   {80, 80, 81, 82},
                   {80, 80, 81, 82},
                   {80, 80, 81, 82},
                   {80, 80, 81, 82},
                   {80, 80, 81, 82},
                   {80, 80, 81, 82},
                   {80, 80, 81, 82},
                   {80, 80, 81, 82},
                   {80, 80, 81, 82}},
    .sw_value = {{0, 127, 127, 127},
                 {0, 127, 127, 127},
                 {0, 127, 127, 127},
                 {0, 127, 127, 127},
                 {0, 127, 127, 127},
                 {0, 127, 127, 127},
                 {0, 127, 127, 127},
                 {0, 127, 127, 127},
                 {0, 127, 127, 127},
                 {0, 127, 127, 127},
                 {0, 127, 127, 127},
                 {0, 127, 127, 127},
                 {0, 127, 127, 127},
                 {0, 127, 127, 127},
                 {0, 127, 127, 127},
                 {0, 127, 127, 127}},
    .sw_position = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
};

void settings_init(void) {
  // if no valid configuration is found, initialize to factory defaults
  if (!load_settings()) {
    factory_settings_init();
    load_settings();
  }

  device_id = 0x07;
}

void factory_settings_init(void) {
  storage.write(factory_settings);
}

bool load_settings(void) {
  storage.read(device_config);

  // minimal validation against blank flash
  // TODO: consider more thorough validation
  return device_config.configuration.led_on != 0xFF;
}

void update_all_settings(uint8_t const *data, uint32_t size) {
  //if (size - 9 < sizeof(device_configuration)) return;
  size_t payload_size = min(sizeof(device_configuration), size);

  // get current i2c mode from memory settings
  uint8_t i2c_mode = device_config.configuration.i2c_master;

  // save settings
  memcpy((uint8_t *)&device_config, data, payload_size);
  // todo: validate new data?
  storage.write(device_config);

  // if the i2c mode changed
  // soft reset
  if (device_config.configuration.i2c_master != i2c_mode) {
    NVIC_SystemReset();
  }
}
