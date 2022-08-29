#!/bin/bash

# echo odroid | sudo -S ./h3-i2ctest 
clear
# setterm -blank 0 -powersave off 2>/dev/null
# echo 0 > /sys/class/graphics/fbcon/cursor_blink
./h3-i2ctest 
# echo 1 > /sys/class/graphics/fbcon/cursor_blink
# clear
