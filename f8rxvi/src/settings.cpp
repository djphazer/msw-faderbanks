#include "core.h"
#include <FlashStorage_SAMD.h>

FlashStorageClass<device_configuration> storage((void*)0x10000);

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
    .midi_thru = 0,
    .delay_msb = 0,
    .delay_lsb = 1,
    .i2c_slave_address = 0x34,
    .high_resolution = 1,
    .byte_14 = 0,
    .byte_15 = 0,
    .byte_16 = 0,
  },
  .usb_channel = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16},
  .trs_channel = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16},
  .usb_control = {32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47},
  .trs_control = {7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22},
};

void settings_init(void) {
  const bool config_exists = load_settings();

  // if no configuration is found, reset to defaults
  if (!config_exists) {
    factory_settings_init();
    load_settings();
  }
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

void update_settings_block(uint8_t const* config_data) {
  memcpy((uint8_t *)&device_config, config_data, sizeof(device_configuration));
  // todo: validate new data?
  storage.write(device_config);
}

void update_all_settings(uint8_t const* sysex_data, uint32_t size) {
  if (size - 9 < sizeof(device_configuration)) return;

  // get current i2c mode from memory settings
  uint8_t i2c_mode = device_config.configuration.i2c_master;

  // save settings
  update_settings_block(sysex_data + 9);

  // if the i2c mode changed
  // soft reset
  if (device_config.configuration.i2c_master != i2c_mode) {
    NVIC_SystemReset();
  }
}

uint16_t get_boot_delay_value(void) {
  uint16_t boot_time = (device_config.configuration.delay_msb | (device_config.configuration.delay_lsb << 7));
  return (boot_time == 0) ? 1 : boot_time;
}
uint16_t get_fader_max(void) {
  return device_config.configuration.fader_max_msb |
         (device_config.configuration.fader_max_lsb << 7);
}
uint16_t get_fader_min(void) {
  return device_config.configuration.fader_min_msb |
         (device_config.configuration.fader_min_lsb << 7);
}
// uint32_t get_chip_id()
//{
// uint32_t chip_id[4];
//
////flash_read(&nvm, 0x0080A00C, &chip_id[0], 4);
////flash_read(&nvm, 0x0080A040, &chip_id[1], 4);
////flash_read(&nvm, 0x0080A044, &chip_id[2], 4);
////flash_read(&nvm, 0x0080A048, &chip_id[3], 4);
//
// return chip_id;
//}
