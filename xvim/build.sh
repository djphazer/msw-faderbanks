pio run -t clean && rm xvim.uf2
pio run
../tools/uf2conv.py -o xvim.uf2 .pio/build/main/firmware.bin
