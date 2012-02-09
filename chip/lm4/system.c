/* Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* System module for Chrome EC : hardware specific implementation */

#include "cpu.h"
#include "registers.h"
#include "system.h"


static int wait_for_hibctl_wc(void)
{
	int i;
	/* Wait for write-capable */
	for (i = 0; i < 1000000; i++) {
		if (LM4_HIBERNATE_HIBCTL & 0x80000000)
			return EC_SUCCESS;
	}
	return EC_ERROR_UNKNOWN;
}


static void check_reset_cause(void)
{
	enum system_image_copy_t copy = system_get_image_copy();
	uint32_t hib_status = LM4_HIBERNATE_HIBRIS;
	enum system_reset_cause_t reset_cause = SYSTEM_RESET_UNKNOWN;
	uint32_t raw_reset_cause;

	/* Read the raw reset cause */
	raw_reset_cause = LM4_SYSTEM_RESC;

	if (hib_status & 0x0d) {
		/* the hibernation module wakes up the system */
		if (hib_status & 0x8)
			reset_cause = SYSTEM_RESET_WAKE_PIN;
		else if (hib_status & 0x1)
			reset_cause = SYSTEM_RESET_RTC_ALARM;
		else if (hib_status & 0x4)
			reset_cause = SYSTEM_RESET_LOW_BATTERY;
		/* clear the pending interrupt */
		wait_for_hibctl_wc();
		LM4_HIBERNATE_HIBIC = hib_status;
	} else if (copy == SYSTEM_IMAGE_RW_A || copy == SYSTEM_IMAGE_RW_B) {
		/* If we're in image A or B, the only way we can get there is
		 * via a warm reset. */
		reset_cause = SYSTEM_RESET_SOFT_WARM;
	} else if (raw_reset_cause & 0x28) {
		/* Watchdog timer 0 or 1 */
		reset_cause = SYSTEM_RESET_WATCHDOG;
	} else if (raw_reset_cause & 0x10) {
		reset_cause = SYSTEM_RESET_SOFT_COLD;
	} else if (raw_reset_cause & 0x04) {
		reset_cause = SYSTEM_RESET_BROWNOUT;
	} else if (raw_reset_cause & 0x02) {
		reset_cause = SYSTEM_RESET_POWER_ON;
	} else if (raw_reset_cause & 0x01) {
		reset_cause = SYSTEM_RESET_RESET_PIN;
	} else if (raw_reset_cause) {
		reset_cause = SYSTEM_RESET_OTHER;
	} else {
		reset_cause = SYSTEM_RESET_UNKNOWN;
	}
	system_set_reset_cause(reset_cause);
}


void system_hibernate(uint32_t seconds, uint32_t microseconds)
{
	/* clear pending interrupt */
	wait_for_hibctl_wc();
	LM4_HIBERNATE_HIBIC = LM4_HIBERNATE_HIBRIS;
	/* set RTC alarm match */
	wait_for_hibctl_wc();
	LM4_HIBERNATE_HIBRTCM0 = seconds;
	wait_for_hibctl_wc();
	LM4_HIBERNATE_HIBRTCSS = (microseconds * 512 / 15625) << 16;

	/* start counting toward the alarm */
	wait_for_hibctl_wc();
	LM4_HIBERNATE_HIBRTCLD = 0;
	/* go to hibernation and wake on RTC match or WAKE pin */
	wait_for_hibctl_wc();
	LM4_HIBERNATE_HIBCTL = 0x5B;
	/* we are going to hibernate ... */
	while (1) ;
}


int system_pre_init(void)
{
	volatile uint32_t scratch  __attribute__((unused));

	/* Enable clocks to the hibernation module */
	LM4_SYSTEM_RCGCHIB = 1;
	/* Wait 3 clock cycles before using the module */
	scratch = LM4_SYSTEM_RCGCHIB;

	/* Enable the hibernation oscillator, if it's not already enabled.  We
	 * use this to hold our scratchpad value across reboots. */
	if (!(LM4_HIBERNATE_HIBCTL & 0x40)) {
		int rv, i;
		rv = wait_for_hibctl_wc();
		if (rv != EC_SUCCESS)
			return rv;

		/* Enable clock to hibernate module */
		LM4_HIBERNATE_HIBCTL |= 0x40;
		/* Wait for write-complete */
		for (i = 0; i < 1000000; i++) {
			if (LM4_HIBERNATE_HIBRIS & 0x10)
				break;
		}
	}
	/* initialize properly registers after reset (cf errata) */
	wait_for_hibctl_wc();
	LM4_HIBERNATE_HIBRTCT = 0x7fff;
	wait_for_hibctl_wc();
	LM4_HIBERNATE_HIBIM = 0;

	check_reset_cause();

	return EC_SUCCESS;
}


int system_init(void)
{
	/* Clear the hardware reset cause, now that we've committed to running
	 * this image. */
	LM4_SYSTEM_RESC = 0;

	return EC_SUCCESS;
}


int system_reset(int is_cold)
{
	/* TODO: (crosbug.com/p/7470) support cold boot; this is a
	   warm boot. */
	CPU_NVIC_APINT = 0x05fa0004;

	/* Spin and wait for reboot; should never return */
	/* TODO: (crosbug.com/p/7471) should disable task swaps while
	   waiting */
	while (1) {}

	return EC_ERROR_UNKNOWN;
}


int system_set_scratchpad(uint32_t value)
{
	int rv;

	/* Wait for ok-to-write */
	rv = wait_for_hibctl_wc();
	if (rv != EC_SUCCESS)
		return rv;

	/* Write scratchpad */
	/* TODO: (crosbug.com/p/7472) might be more elegant to have a
	 * write_hibernate_reg() method which takes an address and
	 * data and does the delays.  Then we could move the hibernate
	 * register accesses to a separate module. */
	LM4_HIBERNATE_HIBDATA = value;

	/* Wait for write-complete */
	return wait_for_hibctl_wc();
}


uint32_t system_get_scratchpad(void)
{
	return LM4_HIBERNATE_HIBDATA;
}