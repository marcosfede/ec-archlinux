/* Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * TI TPS65090 PMU driver.
 */

#include "board.h"
#include "console.h"
#include "common.h"
#include "i2c.h"
#include "pmu_tpschrome.h"
#include "timer.h"
#include "util.h"

#define CPUTS(outstr) cputs(CC_CHARGER, outstr)
#define CPRINTF(format, args...) cprintf(CC_CHARGER, format, ## args)

#define TPS65090_I2C_ADDR 0x90

#define IRQ1_REG 0x00
#define IRQ2_REG 0x01
#define IRQ1MASK 0x02
#define IRQ2MASK 0x03
#define CG_CTRL0 0x04
#define CG_CTRL1 0x05
#define CG_CTRL2 0x06
#define CG_CTRL3 0x07
#define CG_CTRL4 0x08
#define CG_CTRL5 0x09
#define CG_STATUS1 0x0a
#define CG_STATUS2 0x0b
#define TPSCHROME_VER 0x19

/* Charger control */
#define CG_CTRL0_EN   1

/* Charger termination voltage/current */
#define CG_VSET_SHIFT   3
#define CG_VSET_MASK    (3 << CG_VSET_SHIFT)
#define CG_ISET_SHIFT   0
#define CG_ISET_MASK    (7 << CG_ISET_SHIFT)
#define CG_NOITERM      (1 << 5)

/* IRQ events */
#define EVENT_VACG    (1 <<  1)
#define EVENT_VBATG   (1 <<  3)

/* Charger alarm */
#define CHARGER_ALARM 3

/* Clear tps65090 irq */
static inline int pmu_clear_irq(void)
{
	return pmu_write(IRQ1_REG, 0);
}

/* Read all tps65090 interrupt events */
static int pmu_get_event(int *event)
{
	static int prev_event;
	int rv;
	int irq1, irq2;

	pmu_clear_irq();

	rv = pmu_read(IRQ1_REG, &irq1);
	if (rv)
		return rv;
	rv = pmu_read(IRQ2_REG, &irq2);
	if (rv)
		return rv;

	*event = irq1 | (irq2 << 8);

	if (prev_event != *event) {
		CPRINTF("pmu event: %016b\n", *event);
		prev_event = *event;
	}

	return EC_SUCCESS;
}

/* Read/write tps65090 register */
int pmu_read(int reg, int *value)
{
	return i2c_read8(I2C_PORT_CHARGER, TPS65090_I2C_ADDR, reg, value);
}

int pmu_write(int reg, int value)
{
	return i2c_write8(I2C_PORT_CHARGER, TPS65090_I2C_ADDR, reg, value);
}

/**
 * Read tpschrome version
 *
 * @param version       output value of tpschrome version
 */
int pmu_version(int *version)
{
	return pmu_read(TPSCHROME_VER, version);
}

int pmu_is_charger_alarm(void)
{
	int status;

	/**
	 * if the I2C access to the PMU fails, we consider the failure as
	 * non-critical and wait for the next read without send the alert.
	 */
	if (!pmu_read(CG_STATUS1, &status) && (status & CHARGER_ALARM))
		return 1;
	return 0;
}

int pmu_get_power_source(int *ac_good, int *battery_good)
{
	int rv, event;

	rv = pmu_get_event(&event);
	if (rv)
		return rv;

	if (ac_good)
		*ac_good = event & EVENT_VACG;
	if (battery_good)
		*battery_good = event & EVENT_VBATG;

	return EC_SUCCESS;
}

int pmu_enable_charger(int enable)
{
	int rv;
	int reg;

	rv = pmu_read(CG_CTRL0, &reg);
	if (rv)
		return rv;

	if (reg & CG_CTRL0_EN)
		return EC_SUCCESS;

	return pmu_write(CG_CTRL0, enable ? (reg | CG_CTRL0_EN) :
			(reg & ~CG_CTRL0));
}

/**
 * Set termination current for temperature ranges
 *
 * @param range           T01 T12 T23 T34 T40
 * @param current         enum termination current, I0250 == 25.0%:
 *                        I0000 I0250 I0375 I0500 I0625 I0750 I0875 I1000
 */
int pmu_set_term_current(enum TPS_TEMPERATURE_RANGE range,
		enum TPS_TERMINATION_CURRENT current)
{
	int rv;
	int reg_val;

	rv = pmu_read(CG_CTRL1 + range, &reg_val);
	if (rv)
		return rv;

	reg_val &= ~CG_ISET_MASK;
	reg_val |= current << CG_ISET_SHIFT;

	return pmu_write(CG_CTRL1 + range, reg_val);
}

/**
 * Set termination voltage for temperature ranges
 *
 * @param range           T01 T12 T23 T34 T40
 * @param voltage         enum termination voltage, V2050 == 2.05V:
 *                        V2000 V2050 V2075 V2100
 */
int pmu_set_term_voltage(enum TPS_TEMPERATURE_RANGE range,
		enum TPS_TERMINATION_VOLTAGE voltage)
{
	int rv;
	int reg_val;

	rv = pmu_read(CG_CTRL1 + range, &reg_val);
	if (rv)
		return rv;

	reg_val &= ~CG_VSET_MASK;
	reg_val |= voltage << CG_VSET_SHIFT;

	return pmu_write(CG_CTRL1 + range, reg_val);
}

/**
 * Enable low current charging
 *
 * @param enable         enable/disable low current charging
 */
int pmu_low_current_charging(int enable)
{
	int rv;
	int reg_val;

	rv = pmu_read(CG_CTRL5, &reg_val);
	if (rv)
		return rv;

	if (enable)
		reg_val |= CG_NOITERM;
	else
		reg_val &= ~CG_NOITERM;

	return pmu_write(CG_CTRL5, reg_val);
}

void pmu_init(void)
{
#ifdef CONFIG_PMU_BOARD_INIT
	board_pmu_init();
#else
	/* Init configuration
	 *   Fast charge timer    : 2 hours
	 *   Charger              : disable
	 *   External pin control : enable
	 *
	 * TODO: move settings to battery pack specific init
	 */
	pmu_write(CG_CTRL0, 2);
	/* Limit full charge current to 50%
	 * TODO: remove this temporary hack.
	 */
	pmu_write(CG_CTRL3, 0xbb);
#endif
	/* Enable interrupt mask */
	pmu_write(IRQ1MASK, 0xff);
	pmu_write(IRQ2MASK, 0xff);

}

#ifdef CONFIG_CMD_PMU
static int print_pmu_info(void)
{
	int reg, ret;
	int value;

	for (reg = 0; reg < 0xc; reg++) {
		ret = pmu_read(reg, &value);
		if (ret)
			return ret;
		if (!reg)
			ccputs("PMU: ");

		ccprintf("%02x ", value);
	}
	ccputs("\n");

	return 0;
}

static int command_pmu(int argc, char **argv)
{
	int repeat = 1;
	int rv = 0;
	int loop;
	char *e;

	if (argc > 1) {
		repeat = strtoi(argv[1], &e, 0);
		if (*e) {
			ccputs("Invalid repeat count\n");
			return EC_ERROR_INVAL;
		}
	}

	for (loop = 0; loop < repeat; loop++) {
		rv = print_pmu_info();
		usleep(1000);
	}

	if (rv)
		ccprintf("Failed - error %d\n", rv);

	return rv ? EC_ERROR_UNKNOWN : EC_SUCCESS;
}
DECLARE_CONSOLE_COMMAND(pmu, command_pmu,
			"<repeat_count>",
			"Print PMU info",
			NULL);
#endif
