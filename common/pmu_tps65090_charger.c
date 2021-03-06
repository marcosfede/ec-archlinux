/* Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * TI TPS65090 PMU charging task.
 */

#include "board.h"
#include "chipset.h"
#include "console.h"
#include "gpio.h"
#include "pmu_tpschrome.h"
#include "smart_battery.h"
#include "system.h"
#include "task.h"
#include "timer.h"
#include "util.h"

#define CPUTS(outstr) cputs(CC_CHARGER, outstr)
#define CPRINTF(format, args...) cprintf(CC_CHARGER, format, ## args)

/* Charging and discharging alarms */
#define ALARM_DISCHARGING (ALARM_TERMINATE_DISCHARGE | ALARM_OVER_TEMP)
#define ALARM_CHARGING (ALARM_TERMINATE_CHARGE | \
		ALARM_OVER_CHARGED | \
		ALARM_OVER_TEMP)


/* Time delay in usec for idle, charging and discharging.
 * Defined in battery charging flow.
 */
#define T1_USEC 5000000
#define T2_USEC 10000000
#define T3_USEC 10000000

/* Non-SBS charging states */
enum charging_state {
	ST_NONE = 0,
	ST_IDLE,
	ST_PRE_CHARGING,
	ST_CHARGING,
	ST_DISCHARGING,
};

static const char * const state_list[] = {
	"none",
	"idle",
	"pre-charging",
	"charging",
	"discharging"
};

static void enable_charging(int enable)
{
	enable = enable ? 1 : 0;
	if (gpio_get_level(GPIO_CHARGER_EN) != enable)
		gpio_set_level(GPIO_CHARGER_EN, enable);
}


/**
 * Get AC state through GPIO
 *
 * @return 0        AC off
 * @return 1        AC on
 *
 * TODO: This is a board specific function, should be moved to
 * system_common.c or board.c
 */
static int get_ac(void)
{
	/*
	 * Detect AC state using combined gpio pins
	 *
	 * On daisy and snow, there's no single gpio signal to detect AC.
	 *   GPIO_AC_PWRBTN_L provides AC on and PWRBTN release.
	 *   GPIO_KB_PWR_ON_L provides PWRBTN release.
	 * Hence the ac state can be logical OR of these two signal line.
	 *
	 * One drawback of this detection is, when press-and-hold power
	 * button. AC state will be unknown. The implementation below treats
	 * that condition as AC off.
	 *
	 * TODO(rongchang): move board related function to board/ and common
	 * interface to system_get_ac()
	 */

	return gpio_get_level(GPIO_AC_PWRBTN_L) &&
			gpio_get_level(GPIO_KB_PWR_ON_L);
}

/*
 * TODO(rongchang): move battery vendor specific functions to battery pack
 * module
 */
static int battery_temperature_celsius(int t)
{
	return (t - 2731) / 10;
}

static int battery_start_charging_range(int t)
{
	t = battery_temperature_celsius(t);
	return (t > 5 && t < 40);
}

static int battery_charging_range(int t)
{
	t = battery_temperature_celsius(t);
	return (t > 5 && t < 60);
}

static int battery_discharging_range(int t)
{
	t = battery_temperature_celsius(t);
	return (t < 70);
}

static int wait_t1_idle(void)
{
	usleep(T1_USEC);
	return ST_IDLE;
}

static int wait_t2_charging(void)
{
	usleep(T2_USEC);
	return ST_CHARGING;
}

static int wait_t3_discharging(void)
{
	usleep(T3_USEC);
	return ST_DISCHARGING;
}

static int system_off(void)
{
	if (chipset_in_state(CHIPSET_STATE_ON)) {
		CPUTS("[pmu] turn system off\n");
		chipset_exit_hard_off();

		/* TODO(rongchang): After have impl in chipset_exit_hard_off(),
		 * remove these gpio hack
		 */
		gpio_set_level(GPIO_EN_PP3300, 0);
		gpio_set_level(GPIO_EN_PP1350, 0);
		gpio_set_level(GPIO_PMIC_PWRON_L, 1);
		gpio_set_level(GPIO_EN_PP5000, 0);
	}

	return wait_t1_idle();
}

static int notify_battery_low(void)
{
	static timestamp_t last_notify_time;
	timestamp_t now;

	if (chipset_in_state(CHIPSET_STATE_ON)) {
		now = get_time();
		if (now.val - last_notify_time.val > 60000000) {
			CPUTS("[pmu] notify battery low (< 10%)\n");
			last_notify_time = now;
			/* TODO(rongchang): notify AP ? */
		}
	}
	return ST_DISCHARGING;
}

static int calc_next_state(int state)
{
	int batt_temp, alarm, capacity, charge;

	switch (state) {
	case ST_IDLE:

		/* Check AC and chiset state */
		if (!get_ac()) {
			if (chipset_in_state(CHIPSET_STATE_ON))
				return ST_DISCHARGING;

			/* Enable charging and wait ac on */
			enable_charging(1);
			return wait_t1_idle();
		}

		/* Enable charging when battery doesn't respond */
		if (battery_temperature(&batt_temp)) {
			enable_charging(1);
			wait_t1_idle();
			return ST_PRE_CHARGING;
		}

		/* Turn off charger when battery temperature is out
		 * of the start charging range.
		 */
		if (!battery_start_charging_range(batt_temp)) {
			enable_charging(0);
			return wait_t1_idle();
		}

		/* Turn off charger on battery charging alarm */
		if (battery_status(&alarm) || (alarm & ALARM_CHARGING)) {
			if (!(alarm & ALARM_TERMINATE_CHARGE))
				CPRINTF("[pmu] idle %016b\n", alarm);
			enable_charging(0);
			return wait_t1_idle();
		}

		/* Start charging only when battery charge lower than 100% */
		if (!battery_state_of_charge(&charge) && charge < 100) {
			enable_charging(1);
			wait_t1_idle();
			return ST_CHARGING;
		}

		return wait_t1_idle();

	case ST_PRE_CHARGING:
		if (!get_ac())
			return wait_t1_idle();

		/* If the battery goes online after enable the charger,
		 * go into charging state.
		 */
		if (battery_temperature(&batt_temp) == EC_SUCCESS)
			return ST_CHARGING;

		wait_t1_idle();
		return ST_PRE_CHARGING;

	case ST_CHARGING:
		if (!get_ac())
			break;
		if (battery_temperature(&batt_temp) ||
				!battery_charging_range(batt_temp)) {
			CPUTS("[pmu] charging: battery hot\n");
			enable_charging(0);
			break;
		}
		if (battery_status(&alarm) || (alarm & ALARM_CHARGING)) {
			CPUTS("[pmu] charging: battery alarm\n");
			enable_charging(0);
			break;
		}
		if (pmu_is_charger_alarm()) {
			CPUTS("[pmu] charging: charger alarm\n");
			enable_charging(0);
			break;
		}
		return wait_t2_charging();

	case ST_DISCHARGING:

		if (get_ac())
			return wait_t1_idle();

		/* Check battery discharging temperature range */
		if (battery_temperature(&batt_temp) == 0) {
			if (!battery_discharging_range(batt_temp)) {
				CPUTS("[pmu] discharging: battery hot\n");
				enable_charging(0);
				return system_off();
			}
		}
		/* Check discharging alarm */
		if (!battery_status(&alarm) && (alarm & ALARM_DISCHARGING)) {
			CPRINTF("[pmu] discharging: battery alarm %016b\n",
					alarm);
			enable_charging(0);
			return system_off();
		}
		/* Check remaining charge % */
		if (battery_state_of_charge(&capacity) == 0 && capacity < 10)
			return notify_battery_low();

		return wait_t3_discharging();
	}

	return wait_t1_idle();
}

void pmu_charger_task(void)
{
	int state = ST_IDLE;
	int next_state;

	pmu_init();

	while (1) {
		next_state = calc_next_state(state);
		if (next_state != state) {
			CPRINTF("[batt] state %s -> %s\n",
				state_list[state],
				state_list[next_state]);
			state = next_state;
		}

		/* TODO(sjg@chromium.org): root cause crosbug.com/p/11285 */
		usleep(5000 * 1000);
	}
}
