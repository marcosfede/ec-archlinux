# -*- makefile -*-
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#
# on-board test binaries build
#

test-list=hello pingpong timer_calib timer_dos timer_jump mutex thermal
test-list+=power_button kb_deghost kb_debounce scancode typematic charging
test-list+=flash_overwrite flash_rw_erase
#disable: powerdemo

pingpong-y=pingpong.o
powerdemo-y=powerdemo.o
timer_calib-y=timer_calib.o
timer_dos-y=timer_dos.o
mutex-y=mutex.o
flash_overwrite-y=flash.o
flash_rw_erase-y=flash.o

# Mock modules for 'thermal'
chip-mock-thermal-lpc.o=mock_lpc.o
chip-mock-thermal-pwm.o=mock_pwm.o
common-mock-thermal-x86_power.o=mock_x86_power.o
common-mock-thermal-temp_sensor.o=mock_temp_sensor.o

# Mock modules for 'power_button'
chip-mock-power_button-gpio.o=mock_gpio.o
chip-mock-power_button-pwm.o=mock_pwm.o
common-mock-power_button-x86_power.o=mock_x86_power.o
common-mock-power_button-i8042.o=mock_i8042.o

# Mock modules for 'kb_deghost'
chip-mock-kb_deghost-keyboard_scan_stub.o=mock_keyboard_scan_stub.o
common-mock-kb_deghost-i8042.o=mock_i8042.o

# Mock modules for 'scancode'
chip-mock-scancode-keyboard_scan_stub.o=mock_keyboard_scan_stub.o
common-mock-scancode-i8042.o=mock_i8042.o

# Mock modules for 'typematic'
chip-mock-typematic-keyboard_scan_stub.o=mock_keyboard_scan_stub.o
common-mock-typematic-i8042.o=mock_i8042.o

# Mock modules for 'kb_debounce'
chip-mock-kb_debounce-keyboard_scan_stub.o=mock_keyboard_scan_stub.o
common-mock-kb_debounce-i8042.o=mock_i8042.o

# Mock modules for 'charging'
chip-mock-charging-gpio.o=mock_gpio.o
common-mock-charging-x86_power.o=mock_x86_power.o
common-mock-charging-smart_battery_stub.o=mock_smart_battery_stub.o
common-mock-charging-charger_bq24725.o=mock_charger.o

# Mock modules for 'flash_overwrite'
chip-mock-flash_overwrite-flash.o=mock_flash.o
chip-mock-flash_overwrite-gpio.o=mock_gpio.o

# Mock modules for 'flash_rw_erase'
chip-mock-flash_rw_erase-flash.o=mock_flash.o
