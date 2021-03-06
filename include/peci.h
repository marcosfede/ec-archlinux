/* Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* PECI module for Chrome EC */

#ifndef __CROS_EC_PECI_H
#define __CROS_EC_PECI_H

#include "common.h"

/* Return the current CPU temperature in degrees K, or -1 if error.
 *
 * Note that the PECI interface is currently a little flaky; if you get an
 * error, retry a bit later. */
int peci_get_cpu_temp(void);

/* Read the CPU temperature sensor via PECI.  This interface is for the
 * temperature sensor module.  Returns the temperature in degrees K, or -1 if
 * error. */
int peci_temp_sensor_get_val(int idx);

/* Temperature polling of CPU temperature sensor via PECI. */
int peci_temp_sensor_poll(void);

#endif  /* __CROS_EC_PECI_H */
