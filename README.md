# MSW Faderbanks

Open source firmwares and web editor for the [F8R](https://michigansynthworks.com/products/8m-fader-bank-with-cv-i2c-and-midi), [XVI](https://michigansynthworks.com/products/xvi-16-channel-fader-bank-with-cv-i2c-and-midi), and [XVI-M](https://michigansynthworks.com/products/xvi-m) faderbanks from Michigan Synth Works. Inspired by the 16n, these devices use a SAMD21G18A processor.

Old code has been ported from Atmel Studio projects to the Adafruit SAMD Arduino core libraries, targeting Feather M0. I primarily use PlatformIO for the toolchain, but Arduino IDE should work if you install the board support.

## What's in here?

**F8R** and **XVI** (Eurorack or Desktop version) use the same firmware, contained in the `f8rxvi` directory.

**XVI-M** (Desktop only) uses a separate firmware, contained in the `xvim` directory.

Inside each subdirectory, there is a `build.sh` script that will build the firmware and produce a `firmware.uf2` file. You can also run `pio run -t upload` to reflash directly over USB Serial, but this tends to erase stored settings.

# Web Editor

I've included a copy of the web editor project for the F8R/XVI, as-is, in the `editor` directory. It's a SvelteKit project that I haven't attempted to open or modify. I'd rather replace it with a static HTML page, but someone else might be able to utilize it.

The firmwares in this repo are tested and working with the existing web editors:
* F8R/XVI - https://misw.us/editor/
* XVI-M - https://misw.us/EditorM/

## Caveats & Known Issues

Settings stored with previous firmwares will not load in the new open source firmwares. Please export a copy of your configuration from the web editor before updating.

MIDI Thru is broken.

MIDI TRS outputs on XVI-M are untested.

I2C is untested.

# Credits

Authored by Nicholas J. Michalek in partnership with Michigan Synth Works. MIT License.
