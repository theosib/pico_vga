#!/bin/bash

# cp build/pico_motor.uf2 /Volumes/RPI-RP2/
ELF=build/vga.elf
openocd -f interface/cmsis-dap.cfg -f target/rp2040.cfg -c "program $ELF verify reset ; init ; reset halt ; rp2040.core1 arp_reset assert 0 ; rp2040.core0 arp_reset assert 0 ; exit"
