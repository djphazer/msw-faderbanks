pio run -t clean && rm firmware.uf2
pio run
../tools/uf2conv.py -o firmware.uf2 .pio/build/main/firmware.bin
