/*
 * LED drivers
 * handles SPI bus comms and green/red blend timing for orange
 */
#include "core.h"
#include <wiring_private.h>

uint32_t ledframe = 0; // on/off
uint8_t ledred[NUMBER_CHANNELS] = {0}; // green/red blend
uint8_t ledbright[NUMBER_CHANNELS] = {0}; // on/off blend

SPIClass mySPI(&sercom3, 11, SPI_SCK_PIN, SPI_MOSI_PIN, SPI_PAD_2_SCK_3, SERCOM_RX_PAD_0);

void leds_init() {
  mySPI.begin();
  pinPeripheral(SPI_MOSI_PIN, PIO_SERCOM_ALT);
  pinPeripheral(SPI_SCK_PIN, PIO_SERCOM_ALT);
  ledframe = 0;
}

void leds_send(uint32_t data) {
  //delay_us(10);
  mySPI.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0));
  digitalWrite(LED_LATCH, LOW);
  mySPI.transfer((void*)&data, 4);
  digitalWrite(LED_LATCH, HIGH);
  mySPI.endTransaction();
}

void update_leds() {
  static uint8_t orange_counter = 0;
  static uint8_t brightness_counter = 0;

  ++brightness_counter &= 0xf;
  if (brightness_counter & 1)
    ++orange_counter &= 0xf;

  uint32_t leds_out = ledframe;
  for (int i = 0; i < NUMBER_CHANNELS; ++i) {
    if (ledbright[i] <= brightness_counter) {
      leds_out &= ~(0x3 << (i*2));
    } else if (((ledframe >> (i * 2)) & 0x3) == LED_ORANGE) {
      const uint32_t orng = (orange_counter >= ledred[i]) ? LED_RED : LED_GREEN;
      leds_out ^= uint32_t(orng) << (i * 2);
    }
  }
  leds_send(leds_out);
}

uint32_t get_led(uint8_t led) {
  return (ledframe >> ((NUMBER_CHANNELS - 1 - led) * 2)) & 0x3;
}

void set_led(uint8_t led, uint32_t color) {
  const int shift = (NUMBER_CHANNELS - 1 - led) * 2;
  const uint32_t mask = 0x3 << shift;
  ledframe = (ledframe & ~mask) | ((color & 0x3) << shift);
}

// 8-bit value, 0-255
void set_led_blend(uint8_t led, uint8_t redlvl) {
  ledred[NUMBER_CHANNELS - 1 - led] = redlvl >> 4;
}
// 8-bit value, 0-255
void set_led_level(uint8_t led, uint8_t level) {
  ledbright[NUMBER_CHANNELS - 1 - led] = level >> 4;
}

void cycle_led(uint8_t led) {
  set_led(led, get_led(led) + 1);
}
