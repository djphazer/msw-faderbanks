#include "core.h"
#include <Adafruit_TinyUSB.h>

static constexpr int ANALOG_BITS = 12;
static constexpr int MIDI_R_SHIFT = ANALOG_BITS - 7;
static constexpr int MIDI_L_SHIFT = 14 - ANALOG_BITS;

struct MySettings : public midi::DefaultSettings, midi::DefaultSerialSettings {
  static const unsigned SysExMaxSize = 256;
};
Adafruit_USBD_MIDI usb_midi;
MIDI_CREATE_CUSTOM_INSTANCE(Adafruit_USBD_MIDI, usb_midi, MIDI_USB, MySettings);

// Serial5 is PB22/PB23 aka pins 30/31 - EDBG/UART
// This is different on Adafruit vs. Arduino core libraries
MIDI_CREATE_INSTANCE(HardwareSerial, Serial5, MIDI_TRS);

void to_bootloader() {
  BOOT_DOUBLE_TAP_DATA = DOUBLE_TAP_MAGIC;
  NVIC_SystemReset();
}

// globals
device_configuration device_config;
uint8_t device_id = 0x04;
uint8_t channels_count = 8;

uint8_t mux = 0;

// i2c
bool er301_present = false;
bool ansible_present = false;
bool txo_present = false;
uint16_t i2c_value_requested;

// ADC
uint16_t adc_values[16];
uint16_t adc_current_value[16];

const uint8_t mux_mapping[16] = {15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
// const uint8_t mux_mapping[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};

bool volatile midi_tx_flag = false;
bool volatile i2c_tx_flag = false;

// callbacks
void process_sysex(uint8_t *sysex_data, unsigned int size) {
  if (size < 3) { return; }

  if (!(sysex_data[1] == 0x7d && sysex_data[2] == 0x00 &&
        sysex_data[3] == 0x00)) {
    return;
  }

  switch (sysex_data[4]) {
  case 0x1f:
    // 1F = "1nFo" - please send me your current config
    send_current_configuration();
    break;
  case 0x0e:
    // 0E - c0nfig Edit - here is a new config
    update_all_settings(sysex_data, size);
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

void send_current_configuration(void) {
  // 9-byte header, 1-byte terminator
  // device_configuration struct is 80 bytes.
  static constexpr int config_data_size = sizeof(device_configuration);
  static constexpr int config_size = 9 + config_data_size + 1;
  static_assert(config_data_size == 80, "uh-oh, config data size changed!");

  uint8_t sysex_data[config_size] = {0};

  sysex_data[0] = 0xf0;

  sysex_data[1] = 0x7d; // manufacturer
  sysex_data[2] = 0x00;
  sysex_data[3] = 0x00;

  sysex_data[4] = 0x0f; // ConFig;

  sysex_data[5] = device_id;     // Device 01, ie, dev board
  sysex_data[6] = MAJOR_VERSION; // major version
  sysex_data[7] = MINOR_VERSION; // minor version
  sysex_data[8] = POINT_VERSION; // point version

  const uint8_t offset = 9;
  uint8_t *cfgdata = (uint8_t *)&device_config;
  for (int i = 0; i < config_data_size; ++i) {
    // mask every byte to guard against garbage that exceeds 7-bit
    sysex_data[offset + i] = cfgdata[i] & 0x7f;
  }

  // terminate sysex packet
  sysex_data[config_size - 1] = 0xf7;

  // send sysex
  uint8_t midi_tx_cnt = 0;
  uint8_t midi_tx_bytes = 0;
  while (midi_tx_cnt < config_size) {
    tud_task();
    // read midi input so it doesn't overflow
    uint8_t packet[4] = {0};
    while (tud_midi_available())
      tud_midi_packet_read(packet);

    // send sysex
    midi_tx_bytes = tud_midi_stream_write(0, (uint8_t *)&sysex_data + midi_tx_cnt, config_size - midi_tx_cnt); // 154
    midi_tx_cnt += midi_tx_bytes + 1;
  }
}

void i2c_scan_devices(void) {
  /*
  ******************
  i2c device scanner
  ******************
  Send dummy packet to slave devices from address 0x8 to 0x78
  If no error is return, device is found at the address
  */
  uint8_t dummy_packet[4] = {0, 0, 0, 0};

  for (uint8_t i = 8; i < 120; i++) {

    delay_ms(10);

    // set i2c slave address
    Wire.beginTransmission(i);
    // send dummy packet
    Wire.write((uint8_t *)&dummy_packet, 4);
    uint8_t i2c_error_flag = Wire.endTransmission();
    // if no error is returned device is detected at the address
    if (!i2c_error_flag) {
      if (i == ANSIABLE_ADDRESS) {
        ansible_present = true;
      }
      if (i == TXO_ADDRESS) {
        txo_present = true;
      }
      if (i == ER301_ADDRESS) {
        er301_present = true;
      }
    }
  }
  return;
}

void process_fader(void) {
  // TODO: remap reading between min and max setting
  adc_current_value[mux] = analogRead(ADC_PIN);
  // TODO: smoothing?

  // if the value has changed, process the value
  // set flags to send midi and i2c packets
  if (abs(adc_current_value[mux] - adc_values[mux]) > 8) {
    digitalWrite(LED_BUILTIN, HIGH);
    //Serial.printf("Fader %u = %u \n", mux, adc_current_value[mux]);

    if (device_config.configuration.high_resolution == true) {
      midi_tx_flag = true;
    } else {
      // send packets if the 7bit value changed
      if (adc_values[mux] >> MIDI_R_SHIFT !=
          adc_current_value[mux] >> MIDI_R_SHIFT) {
        midi_tx_flag = true;
      }
    }

    adc_values[mux] = adc_current_value[mux];

    // always send i2c packet
    i2c_tx_flag = true;
  }

  return;
}

uint16_t value_flip(uint16_t value) {
  if (device_config.configuration.controller_flip) {
    return 16384 - value;
  }
  return value;
}

void i2c_master_task(void) {
  if (i2c_tx_flag) {

    // if (i2c_is_master())
    if (device_config.configuration.i2c_master) {
      // shift the value to 14bit?
      uint16_t i2c_value =
          ((device_id == 0x04) ? adc_values[mux] : value_flip(adc_values[mux]));

      //?? this was copied from the 16n code
      uint8_t port = mux % 4;
      uint8_t device = mux / 4;

      // TXo
      if (txo_present) {
        i2c_packet txo_packet = {0x11, port, uint8_t(i2c_value >> 8),
                                 uint8_t(i2c_value & 0xFF)};
        Wire.beginTransmission(TXO_ADDRESS + device);
        Wire.write( (uint8_t *)&txo_packet, 4);
        Wire.endTransmission();
      }

      // ER-301
      if (er301_present) {
        i2c_packet er301_packet = {0x11, mux, uint8_t(i2c_value >> 8),
                                   uint8_t(i2c_value & 0xFF)};
        Wire.beginTransmission(ER301_ADDRESS);
        Wire.write((uint8_t *)&er301_packet, 4);
        Wire.endTransmission();

#if DEBUG
        Serial.printf("Pot%x = %x (%x)\n\r", mux, i2c_value >> 8, i2c_value & 0xFF);
#endif
      }

      // ANSIBLE
      if (ansible_present) {
        i2c_packet ansible_packet = {0x06, port, uint8_t(i2c_value >> 8),
                                     uint8_t(i2c_value & 0xFF)};
        Wire.beginTransmission(ANSIABLE_ADDRESS + (device << 1));
        Wire.write((uint8_t *)&ansible_packet, 4);
        Wire.endTransmission();
      }
    }

    i2c_tx_flag = false;
  }

  return;
}

void midi_task() {
  USBDevice.task();
  MIDI_USB.read();
  MIDI_TRS.read();

  // if (device_config.configuration.midi_thru == true) {
    // TODO:
  // }

  if (midi_tx_flag) {
    const uint16_t midi_value =
        ((device_id == 0x04) ? adc_values[mux] : value_flip(adc_values[mux]));

    if (device_config.configuration.high_resolution == false) {
      // send regular midi CC
      MIDI_USB.sendControlChange(device_config.usb_control[mux],
                                 uint8_t(midi_value >> MIDI_R_SHIFT),
                                 device_config.usb_channel[mux]);

      MIDI_TRS.sendControlChange(device_config.trs_control[mux],
                                 uint8_t(midi_value >> MIDI_R_SHIFT),
                                 device_config.trs_channel[mux]);
    } else {
      // send midi pitch bend
      const int pbval = int(midi_value << MIDI_L_SHIFT) - 8192;
      MIDI_USB.sendPitchBend(pbval, device_config.usb_channel[mux]);
      MIDI_TRS.sendPitchBend(pbval, device_config.trs_channel[mux]);
    }

    midi_tx_flag = false;
  }

  return;
}

void i2c_master_init(void) {
  // i2c master enable
  Wire.begin();
}

void i2c_slave_init(void) {
  // i2c slave enable
  Wire.begin(device_config.configuration.i2c_slave_address);
}

void i2c_slave_task() {
  uint8_t c = 0;

  if (Wire.available()) {
      //(SERCOM2)->I2CS.INTFLAG.bit.DRDY) {
    // read i2c fader request from master
    c = Wire.read();

    // flip the value based on the config and shift left to make it 14bit
    i2c_value_requested = ((device_id == 0x04) ? adc_values[c] : value_flip(adc_values[c]));

    // send i2c reply packet with fader value
    uint8_t i2c_s_packet[2] = {uint8_t(i2c_value_requested >> 8),
                               uint8_t(i2c_value_requested & 0xFF)};
    Wire.write(i2c_s_packet, 2);

#if DEBUG
    Serial.printf("Pot%x = %x (%x)\n\r", c, i2c_value_requested & 0xFF, i2c_value_requested >> 8);
#endif
  }
}


//--------------------------------------------------------------------+
// MUX TASK
//--------------------------------------------------------------------+
void set_mux(void) {
  // increment mux index - assumes channel count is pow2
  mux = (mux + 1) & (channels_count - 1);

  uint8_t mux_value = mux_mapping[mux];

  // flip fader order
  if (device_config.configuration.controller_flip) {
    mux_value = mux_mapping[channels_count - mux - 1];
  }

  // Set mux pins
  digitalWrite(MUX_SW_0, mux_value & (1 << 0));
  digitalWrite(MUX_SW_1, mux_value & (1 << 1));
  digitalWrite(MUX_SW_2, mux_value & (1 << 2));
  digitalWrite(MUX_SW_3, mux_value & (1 << 3));

  delay_us(MUXTIME);
}

void setup(void) {
  Serial.begin(115200);

  pinMode(SW1_1, INPUT_PULLUP);
  pinMode(SW1_2, INPUT_PULLUP);
  pinMode(SW1_3, INPUT_PULLUP);
  pinMode(SW1_4, INPUT_PULLUP);

  pinMode(V3_ENABLE, OUTPUT);
  digitalWrite(V3_ENABLE, true);

  pinMode(MUX_SW_1, OUTPUT);
  digitalWrite(MUX_SW_1, false);

  pinMode(MUX_SW_2, OUTPUT);
  digitalWrite(MUX_SW_2, false);

  pinMode(MUX_SW_3, OUTPUT);
  digitalWrite(MUX_SW_3, false);

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, false);

  pinMode(MUX_SW_0, OUTPUT);
  digitalWrite(MUX_SW_0, false);

  delay_ms(10); // idk if ADC needs this but ok
  analogReadResolution(ANALOG_BITS);
  pinMode(ADC_PIN, INPUT);
  analogRead(ADC_PIN);

  settings_init();

  if (digitalRead(SW1_1)) {
    // F8R
    device_id = 0x04;
    channels_count = 8;
  } else {
    // XVI
    device_id = 0x05;
    channels_count = 16;
  }

  // i2c initialization
  if (device_config.configuration.i2c_master) {
    // delay required for slave devices to fully boot
    //delay_ms(5000);
    i2c_master_init();
    i2c_scan_devices();
  } else {
    i2c_slave_init();
  }

  // extra i2c delay according to DIP switch 3 and 4
  delay_ms(get_boot_delay_value());

  digitalWrite(LED_BUILTIN, HIGH);

  USBDevice.setManufacturerDescriptor("MiSW");
  USBDevice.setProductDescriptor("MiSW F8R/XVI");
  USBDevice.setID(0x16D0, 0x1161);

  MIDI_USB.begin(MIDI_CHANNEL_OMNI);
  MIDI_USB.setHandleSystemExclusive(process_sysex);

  MIDI_TRS.begin(MIDI_CHANNEL_OMNI);

  // flashy LED splash
  Serial.println("-=*=- Pew! Pew! Pew! -=*=-");
  int i = 0;
  while (i++ < 10) {
    digitalWrite(LED_BUILTIN, LOW);
    delay(100);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(100);
  }

  digitalWrite(LED_BUILTIN, device_config.configuration.led_on);
}

void loop(void) {
  uint16_t ticks = 0;

  while (1) {
    // process one mux position each time
    set_mux();
    process_fader();
    midi_task();
    i2c_master_task();
    i2c_slave_task();

#if DEBUG
    if (Serial && Serial.available()) {
      Serial.write(Serial.read());
      Serial.write('\n');
      Serial.printf("tick=%u ADC[%u]=%u\n", ticks, mux, adc_current_value[mux]);
    }
#endif

    ++ticks;
    if (ticks == 0) {
      digitalWrite(LED_BUILTIN, LOW);
    }
  }
}
