#include "core.h"
#include "wiring_private.h"
#include <Adafruit_TinyUSB.h>

#define MIDI_R_SHIFT 5

extern struct device_configuration device_config;
extern uint8_t device_id;

struct MySettings : public midi::DefaultSettings, midi::DefaultSerialSettings {
  static const unsigned SysExMaxSize = 256;
};

Adafruit_USBD_MIDI usb_midi;
MIDI_CREATE_CUSTOM_INSTANCE(Adafruit_USBD_MIDI, usb_midi, MIDI_USB, MySettings);

// Serial MIDI - technically transmit only
Uart SerialTRS_A(&sercom2, TRSMIDI_A_RX, TRSMIDI_A_TX, SERCOM_RX_PAD_1, UART_TX_PAD_0);
MIDI_CREATE_INSTANCE(HardwareSerial, SerialTRS_A, SerialMidi1);
//Uart SerialTRS_B(&sercom5, TRSMIDI_B_RX, TRSMIDI_B_TX, SERCOM_RX_PAD_1, UART_TX_PAD_2);
MIDI_CREATE_INSTANCE(HardwareSerial, Serial5, SerialMidi2);

void SERCOM2_Handler() {
  SerialTRS_A.IrqHandler();
}

// ADC
uint16_t adc_values[NUMBER_CHANNELS];
uint16_t adc_current_value[NUMBER_CHANNELS];

void to_bootloader() {
  BOOT_DOUBLE_TAP_DATA = DOUBLE_TAP_MAGIC;
  NVIC_SystemReset();
}

void process_sysex(uint8_t *sysex_data, unsigned int size) {
  Serial.printf("-- Sysex Rx: %u bytes --\n", size);
  size_t i = 0;
  do {
    Serial.printf("%02x ", sysex_data[i]);
  } while (++i < size);
  Serial.println();

  if (size < 3) {
    return;
  }

  static sysex_buffer_t payload;

  if (sysex_data[0] == 0xf0 && (sysex_data[1] == 0x7d && sysex_data[2] == 0x00 &&
        sysex_data[3] == 0x00)) {
    payload.length = 0;
    payload.append(sysex_data + 4, size - 5);
  } else if (sysex_data[0] == 0xf7) {
    payload.append(sysex_data + 1, size - 2);
  } else
    return;

  if (sysex_data[size - 1] != 0xf7) return;

  switch (payload.buffer[0]) {
  case 0x1f:
    // 1F = "1nFo" - please send me your current config
    send_current_configuration();
    break;
  case 0x0e:
    // 0E - c0nfig Edit - here is a new config
    update_all_settings(payload.buffer + 5, payload.length - 5);
    break;
  case 0x1a:
    // 1A - 1nitiAlize - blank EEPROM and reset to factory settings.
    factory_settings_init();
    load_settings();
    break;
  case 0x0b:
    // 0B = Jump to bootloader
    // function to jump to bootloader
    to_bootloader();
    break;
  }
}

// Sysex command 0x0F - "c0nFig" - outputs its config:
void send_current_configuration(void) {
  // 9-byte header, 1-byte terminator
  // device_configuration struct is 224 bytes.
  static constexpr int config_data_size = sizeof(device_configuration);
  //static constexpr int config_data_size = 208;
  static constexpr int config_size = 9 + config_data_size + 1;

  uint8_t sysex_data[config_size] = {0};

  // First 9 bytes are the header
  sysex_data[0] = 0xf0;

  sysex_data[1] = 0x7d; // manufacturer
  sysex_data[2] = 0x00;
  sysex_data[3] = 0x00;

  sysex_data[4] = 0x0f; // ConFig;

  sysex_data[5] = device_id;     // Device 01, ie, dev board
  sysex_data[6] = MAJOR_VERSION; // major version
  sysex_data[7] = MINOR_VERSION; // minor version
  sysex_data[8] = POINT_VERSION; // point version

  uint8_t *cfgdata = (uint8_t *)&device_config;
  for (int i = 0; i < config_data_size; ++i) {
    // mask every byte to guard against garbage that exceeds 7-bit
    sysex_data[9 + i] = cfgdata[i] & 0x7f;
  }
  //memcpy((uint8_t *)&sysex_data + 9, (uint8_t *)&device_config, config_data_size);

  // terminate sysex packet
  sysex_data[config_size - 1] = 0xf7;

  Serial.printf("-- Sysex Tx: %u bytes --\n", config_size);
  size_t i = 0;
  do {
    Serial.printf("%02x ", sysex_data[i]);
  } while (++i < config_size);
  Serial.println();

  // send sysex
  uint8_t midi_tx_cnt = 0;
  uint8_t midi_tx_bytes = 0;
  while (midi_tx_cnt < config_size - 1) {
    tud_task();
    // read midi input so it doesn't overflow
    uint8_t packet[4] = {0};
    while (tud_midi_available())
      tud_midi_packet_read(packet);

    // send sysex
    midi_tx_bytes = tud_midi_stream_write(
        0, (uint8_t *)&sysex_data + midi_tx_cnt, config_size - midi_tx_cnt);
    midi_tx_cnt += midi_tx_bytes;
  }

  // read midi input so it doesn't overflow
  // MIDI_USB.read();
  // send sysex
  // usb_midi.write(sysex_data, config_size);
}

void midi_task() {
  USBDevice.task();
  MIDI_USB.read();
}
/*
  if (MIDI_USB.available()) {
    uint8_t midi_rx_buffer[512] = {0};
    uint32_t midi_rx_size = 0;

    while (MIDI_USB.available()) {
      midi_rx_size = tud_midi_stream_read(midi_rx_buffer, 512);
    }

    memcpy(sysex_buffer.buffer + sysex_buffer.length, midi_rx_buffer,
           midi_rx_size);
    sysex_buffer.length += midi_rx_size;

    if (sysex_buffer.buffer[0] == 0xf0 &&
        sysex_buffer.buffer[sysex_buffer.length - 1] == 0xf7) {
      process_sysex(sysex_buffer.buffer, sysex_buffer.length - 1);
      sysex_buffer.length = 0;
    } else if (sysex_buffer.buffer[0] != 0xf0) {
      if (device_config.configuration.midi_thru == true) {
        SerialTRS_A.write(sysex_buffer.buffer, sysex_buffer.length);
        sysex_buffer.length = 0;
      }
    }
  }
*/

// send fader CC for current mux position
void midi_send_fader(uint8_t mux) {
  if (!device_config.configuration.high_resolution) {
    // -- low-res CC message
    // right shift the value to 7bit
    uint16_t midi_value = adc_values[mux] >> MIDI_R_SHIFT;

    switch (device_config.midi_port[mux]) {
    case USB_PORT:
      MIDI_USB.sendControlChange(device_config.control[mux], midi_value,
                                 device_config.channel[mux]);
      break;
    case TRS_PORT_A:
      SerialMidi1.sendControlChange(device_config.control[mux], midi_value,
                                    device_config.channel[mux]);
      break;
    case TRS_PORT_B:
      SerialMidi2.sendControlChange(device_config.control[mux], midi_value,
                                    device_config.channel[mux]);
      break;
    default:
      break;
    }
  } else {
    // -- high-res pitch bend message
    // TODO: constrain the midi value to min and max fader configuration
    // midi_value = map(midi_value, 0, 127, device_config.min_value[mux], device_config.max_value[mux]);
    // left shift the 12-bit value to 14-bit
    const int16_t midi_value = (adc_values[mux] << 2) - 8192;

    switch (device_config.midi_port[mux]) {
    case USB_PORT:
      MIDI_USB.sendPitchBend(midi_value, device_config.channel[mux]);
      break;
    case TRS_PORT_A:
      SerialMidi1.sendPitchBend(midi_value, device_config.channel[mux]);
      break;
    case TRS_PORT_B:
      SerialMidi2.sendPitchBend(midi_value, device_config.channel[mux]);
      break;
    default:
      break;
    }
  }
}

// basically a button press handler
void midi_send_switch(uint8_t mux, bool falling = false) {
    uint8_t cc_packet[3];

    // advance switch state
    if (MOMENTARY == device_config.sw_mode[mux]) {
      cc_packet[1] = device_config.sw_control[mux][1 - falling];
      cc_packet[2] = device_config.sw_value[mux][1 - falling];
    } else {
      if (++device_config.sw_position[mux] > device_config.sw_mode[mux]) {
        device_config.sw_position[mux] = 0;
      }
      cc_packet[1] = device_config.sw_control[mux][device_config.sw_position[mux]];
      cc_packet[2] = device_config.sw_value[mux][device_config.sw_position[mux]];
    }

    switch (device_config.midi_port[mux]) {
    case USB_PORT:
      MIDI_USB.sendControlChange(cc_packet[1], cc_packet[2], device_config.channel[mux]);
      break;
    case TRS_PORT_A:
      SerialMidi1.sendControlChange(cc_packet[1], cc_packet[2], device_config.channel[mux]);
      break;
    case TRS_PORT_B:
      SerialMidi2.sendControlChange(cc_packet[1], cc_packet[2], device_config.channel[mux]);
    default:
      break;
    }

    set_led(mux, led_color_t((device_config.sw_mode[mux] == MOMENTARY)
                                 ? (1 - falling)
                                 : device_config.sw_position[mux]));
}

uint16_t fader_max(void) {
  return device_config.configuration.fader_max_msb |
         (device_config.configuration.fader_max_lsb << 7);
}
uint16_t fader_min(void) {
  return device_config.configuration.fader_min_msb |
         (device_config.configuration.fader_min_lsb << 7);
}
uint16_t read_fader() {
  // TODO: constrain/scale results between min and max
  return analogRead(ADC_PIN);
}

void ledsplash() {
  elapsedMillis frame_width = 0;
  LOOP(led, NUMBER_CHANNELS) set_led(led, LED_ORANGE);
  LOOP(step, 16) {
    LOOP(fade, 32) {
      int i = step & 0xf;
      do {
        //if (i > NUMBER_CHANNELS) continue;
        set_led_level(i, fade << 3);
        set_led_blend(i, i << 4);
      } while (i-- > 0);

      // refresh for this long in ms
      while (frame_width < 5) { update_leds(); }
      frame_width = 0;
    }
  }
  LOOP(start, 48) {
    LOOP(step, 16) {
      set_led_blend((start + step) & 0xf, step << 4);
    }
    while (frame_width < 30) { update_leds(); }
    frame_width = 0;
  }
  LOOP(led, NUMBER_CHANNELS) set_led(led, LED_OFF);
}

void setup(void) {
  Serial.begin(115200);

  // load settings from flash
  settings_init();

  // initialize ADC
  digitalWrite(ADC_PIN, LOW);
  pinMode(ADC_PIN, INPUT);
  analogReadResolution(12);
  // first ADC reading should not be used - read SAMD21G18A datasheet 33.6.2.1
  analogRead(ADC_PIN);
  // button
  digitalWrite(SWITCH_PIN, LOW);
  pinMode(SWITCH_PIN, INPUT);

  // midi init
  SerialMidi1.begin(MIDI_CHANNEL_OMNI);
  SerialMidi2.begin(MIDI_CHANNEL_OMNI);

  pinPeripheral(TRSMIDI_A_TX, PIO_SERCOM_ALT); // tx
  pinPeripheral(TRSMIDI_A_RX, PIO_SERCOM_ALT); // rx
  // pinPeripheral(TRSMIDI_B_TX, PIO_SERCOM_ALT); // tx
  // pinPeripheral(TRSMIDI_B_RX, PIO_SERCOM); // rx

  for (size_t i = 0; i < ARRAY_SIZE(OUTPUTS); ++i) {
    pinMode(OUTPUTS[i], OUTPUT);
  }
  digitalWrite(LED_LATCH, HIGH);

  leds_init();

  // initializes tinyusb?
  USBDevice.setManufacturerDescriptor("MiSW");
  USBDevice.setProductDescriptor("MiSW XVI-M");
  USBDevice.setID(0x16d0, 0x016f);

  MIDI_USB.begin(MIDI_CHANNEL_OMNI);
  MIDI_USB.setHandleSystemExclusive(process_sysex);

  ledsplash();
}

static uint32_t button_state = 0;
void Poll(void) {
  for (uint8_t mux = 0; mux < NUMBER_CHANNELS; ++mux) {
    // Set mux pins
    digitalWrite(MUX_SW_0, ~mux & (1 << 0));
    digitalWrite(MUX_SW_1, ~mux & (1 << 1));
    digitalWrite(MUX_SW_2, ~mux & (1 << 2));
    digitalWrite(MUX_SW_3, ~mux & (1 << 3));
    delay_us(MUXTIME);

    adc_current_value[mux] = read_fader();
    // if the value has changed significantly,
    // process the value, send midi packets
    if (abs(adc_current_value[mux] - adc_values[mux]) > 16) {
      Serial.printf("Pot%x = %i (%x)\n\r", mux, adc_current_value[mux], adc_current_value[mux]);
      if (device_config.configuration.high_resolution ||
          (adc_values[mux] >> MIDI_R_SHIFT !=
           adc_current_value[mux] >> MIDI_R_SHIFT)) {
        midi_send_fader(mux);
      }

      // update values array with new value
      adc_values[mux] = adc_current_value[mux];

      // fader sets brightness and color
      set_led_level(mux, adc_values[mux] >> 4);
      set_led_blend(mux, (adc_values[mux] & (1 << 11)) ? (adc_values[mux] - (1 << 11)) >> 3 : 0);
    }

    // simple button scan
    if (!digitalRead(SWITCH_PIN)) {
      // rising edge
      if (~button_state & (1u << mux)) {
        midi_send_switch(mux);
      }

      button_state |= (1u << mux);
    } else {
      // falling edge
      if (button_state & (1u << mux)) {
        if (MOMENTARY == device_config.sw_mode[mux]) {
          midi_send_switch(mux, true);
        }
      }

      button_state &= ~(1u << mux);
    }
    // scan_switches(mux);
  }
}

#if DEBUG
struct Profiler {
  uint32_t min_, max_, average_;
  void reset() { min_ = 0xffffffff, max_ = 0, average_ = 0; }
  void push(uint32_t val) {
    min_ = min(val, min_);
    max_ = max(val, max_);
    average_ = (average_ + val) / 2;
  }
  void push() {
    static elapsedMicros t_ = 0;
    push(t_);
    t_ = 0;
  }
};
Profiler prof;
elapsedSeconds debug_timer = 0;
#endif

uint16_t ticks = 0;
void loop(void) {
#if DEBUG
  prof.push();
  prof.reset();
#endif

  while (1) {
    Poll(); // scan all mux targets and process signal changes
    update_leds();
    midi_task(); // for handling input via USB

    ++ticks;

#if DEBUG
    prof.push();
    if (debug_timer > 5) {
      Serial.printf("#%u loop us: %lu / %lu / %lu \n",
          ticks, prof.min_, prof.average_, prof.max_);
      Serial.printf("Btn: %08X \n", button_state);
      debug_timer = 0;
      prof.reset();
    }
#endif
  }
}
