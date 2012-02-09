# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#
# Board specific files build
#

# the IC is TI Stellaris LM4
CHIP:=lm4

board-y=board.o
board-$(CONFIG_CHARGER)+=dummy_charger.o
board-$(CONFIG_TEMP_SENSOR)+=board_temp_sensor.o