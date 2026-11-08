pio run -t clean && rm f8rxvi.uf2
pio run
../tools/uf2conv.py -o f8rxvi.uf2 .pio/build/main/firmware.bin
