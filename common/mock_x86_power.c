/* Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Mock X86 chipset power control module for Chrome EC */

#include "chipset.h"
#include "console.h"
#include "lpc.h"
#include "timer.h"
#include "uart.h"
#include "util.h"
#include "x86_power.h"

static int mock_power_on = 0;

void x86_power_cpu_overheated(int too_hot)
{
	/* Print transitions */
	static int last_val = 0;
	if (too_hot != last_val) {
		if (too_hot)
			uart_printf("CPU overheated.\n");
		else
			uart_printf("CPU no longer overheated.\n");
		last_val = too_hot;
	}
}


void x86_power_force_shutdown(void)
{
	uart_puts("Force shutdown\n");
	mock_power_on = 0;
}


void x86_power_reset(int cold_reset)
{
	uart_printf("X86 Power %s reset\n", cold_reset ? "cold" : "warm");
}


void chipset_throttle_cpu(int throttle)
{
	/* Print transitions */
	static int last_val = 0;
	if (throttle != last_val) {
		if (throttle)
			uart_printf("Throttle CPU.\n");
		else
			uart_printf("No longer throttle CPU.\n");
		last_val = throttle;
	}
}


void chipset_exit_hard_off(void)
{
	/* Not implemented */
	return;
}


int chipset_in_state(int state_mask)
{
	if (mock_power_on)
		return state_mask == CHIPSET_STATE_ON;
	else
		return (state_mask == CHIPSET_STATE_SOFT_OFF) ||
		       (state_mask == CHIPSET_STATE_ANY_OFF);
}


void x86_power_interrupt(enum gpio_signal signal)
{
	/* Not implemented */
	return;
}


void x86_power_task(void)
{
	/* Do nothing */
	while (1)
		usleep(5000000);
}


static int command_mock_power(int argc, char **argv)
{
	if (argc != 2)
		return EC_ERROR_PARAM_COUNT;

	if (!strcasecmp(argv[1], "on")) {
		mock_power_on = 1;
	}
	else if (!strcasecmp(argv[1], "off")) {
		mock_power_on = 0;
	}
	else
		return EC_ERROR_PARAM1;

	return EC_SUCCESS;
}
DECLARE_CONSOLE_COMMAND(powermock, command_mock_power,
			"<on | off>",
			"Mock power state",
			NULL);
