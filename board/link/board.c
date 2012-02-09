/* Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
/* EC for Link board configuration */

#include "board.h"
#include "config.h"
#include "gpio.h"
#include "power_button.h"
#include "registers.h"
#include "util.h"
#include "x86_power.h"
#include "lm4_adc.h"
#include "adc.h"

#ifndef CONFIG_TASK_X86POWER
#define x86_power_interrupt NULL
#endif
#ifndef CONFIG_TASK_POWERBTN
#define power_button_interrupt NULL
#endif


/* GPIO signal list.  Must match order from enum gpio_signal. */
const struct gpio_info gpio_list[GPIO_COUNT] = {
	/* Inputs with interrupt handlers are first for efficiency */
	{"POWER_BUTTONn",       LM4_GPIO_K, (1<<7), GPIO_INT_BOTH,
	 power_button_interrupt},
	{"LID_SWITCHn",         LM4_GPIO_K, (1<<5), GPIO_INT_BOTH,
	 power_button_interrupt},
	/* Other inputs */
	{"POWER_ONEWIRE",       LM4_GPIO_H, (1<<2), 0, NULL},
	{"THERMAL_DATA_READYn", LM4_GPIO_B, (1<<4), 0, NULL},
	{"AC_PRESENT",          LM4_GPIO_H, (1<<3), 0, NULL},
	{"PCH_BKLTEN",          LM4_GPIO_J, (1<<3), GPIO_INT_BOTH,
	 x86_power_interrupt},
	{"PCH_SLP_An",          LM4_GPIO_G, (1<<5), GPIO_INT_BOTH,
	 x86_power_interrupt},
	{"PCH_SLP_ME_CSW_DEVn", LM4_GPIO_G, (1<<4), GPIO_INT_BOTH,
	 x86_power_interrupt},
	{"PCH_SLP_S3n",         LM4_GPIO_J, (1<<0), GPIO_INT_BOTH,
	 x86_power_interrupt},
	{"PCH_SLP_S4n",         LM4_GPIO_J, (1<<1), GPIO_INT_BOTH,
	 x86_power_interrupt},
	{"PCH_SLP_S5n",         LM4_GPIO_J, (1<<2), GPIO_INT_BOTH,
	 x86_power_interrupt},
	{"PCH_SLP_SUSn",        LM4_GPIO_G, (1<<3), GPIO_INT_BOTH,
	 x86_power_interrupt},
	{"PCH_SUSWARNn",        LM4_GPIO_G, (1<<2), GPIO_INT_BOTH,
	 x86_power_interrupt},
	{"PGOOD_1_5V_DDR",      LM4_GPIO_K, (1<<0), GPIO_INT_BOTH,
	 x86_power_interrupt},
	{"PGOOD_1_5V_PCH",      LM4_GPIO_K, (1<<1), GPIO_INT_BOTH,
	 x86_power_interrupt},
	{"PGOOD_1_8VS",         LM4_GPIO_K, (1<<3), GPIO_INT_BOTH,
	 x86_power_interrupt},
	{"PGOOD_5VALW",         LM4_GPIO_H, (1<<0), GPIO_INT_BOTH,
	 x86_power_interrupt},
	{"PGOOD_CPU_CORE",      LM4_GPIO_M, (1<<3), GPIO_INT_BOTH,
	 x86_power_interrupt},
	{"PGOOD_VCCP",          LM4_GPIO_K, (1<<2), GPIO_INT_BOTH,
	 x86_power_interrupt},
	{"PGOOD_VCCSA",         LM4_GPIO_H, (1<<1), GPIO_INT_BOTH,
	 x86_power_interrupt},
	{"PGOOD_VGFX_CORE",     LM4_GPIO_D, (1<<2), GPIO_INT_BOTH,
	 x86_power_interrupt},
	{"RECOVERYn",           LM4_GPIO_H, (1<<7), 0, NULL},
	{"USB1_STATUSn",        LM4_GPIO_E, (1<<7), 0, NULL},
	{"USB2_STATUSn",        LM4_GPIO_E, (1<<1), 0, NULL},
	{"WRITE_PROTECTn",      LM4_GPIO_J, (1<<4), 0, NULL},
	/* Outputs; all unasserted by default */
	{"CPU_PROCHOTn",        LM4_GPIO_F, (1<<2), GPIO_OUT_HIGH, NULL},
	{"ENABLE_1_5V_DDR",     LM4_GPIO_H, (1<<5), GPIO_OUT_LOW, NULL},
	{"ENABLE_BACKLIGHT",    LM4_GPIO_H, (1<<4), GPIO_OUT_LOW, NULL},
	{"ENABLE_VCORE",        LM4_GPIO_F, (1<<7), GPIO_OUT_LOW, NULL},
	{"ENABLE_VS",           LM4_GPIO_G, (1<<6), GPIO_OUT_LOW, NULL},
	{"ENTERING_RW",         LM4_GPIO_J, (1<<5), GPIO_OUT_LOW, NULL},
	{"PCH_A20GATE",         LM4_GPIO_Q, (1<<6), GPIO_OUT_LOW, NULL},
	{"PCH_DPWROK",          LM4_GPIO_G, (1<<0), GPIO_OUT_LOW, NULL},
	{"PCH_HDA_SDO",         LM4_GPIO_G, (1<<1), GPIO_OUT_LOW, NULL},
	{"PCH_LID_SWITCHn",     LM4_GPIO_F, (1<<0), GPIO_OUT_HIGH, NULL},
	{"PCH_NMIn",            LM4_GPIO_M, (1<<2), GPIO_OUT_HIGH, NULL},
	{"PCH_PWRBTNn",         LM4_GPIO_G, (1<<7), GPIO_OUT_HIGH, NULL},
	{"PCH_PWROK",           LM4_GPIO_F, (1<<5), GPIO_OUT_LOW, NULL},
	{"PCH_RCINn",           LM4_GPIO_Q, (1<<7), GPIO_OUT_HIGH, NULL},
	/* Exception: RSMRST# is asserted at power-on */
	{"PCH_RSMRSTn",         LM4_GPIO_F, (1<<1), GPIO_OUT_LOW, NULL},
	{"PCH_SMIn",            LM4_GPIO_F, (1<<4), GPIO_OUT_HIGH, NULL},
	{"PCH_SUSACKn",         LM4_GPIO_F, (1<<3), GPIO_OUT_HIGH, NULL},
	{"SHUNT_1_5V_DDR",      LM4_GPIO_F, (1<<6), GPIO_OUT_HIGH, NULL},
	{"USB1_CTL1",           LM4_GPIO_E, (1<<2), GPIO_OUT_LOW, NULL},
	{"USB1_CTL2",           LM4_GPIO_E, (1<<3), GPIO_OUT_LOW, NULL},
	{"USB1_CTL3",           LM4_GPIO_E, (1<<4), GPIO_OUT_LOW, NULL},
	{"USB1_ENABLE",         LM4_GPIO_E, (1<<5), GPIO_OUT_LOW, NULL},
	{"USB1_ILIM_SEL",       LM4_GPIO_E, (1<<6), GPIO_OUT_LOW, NULL},
	{"USB2_CTL1",           LM4_GPIO_D, (1<<4), GPIO_OUT_LOW, NULL},
	{"USB2_CTL2",           LM4_GPIO_D, (1<<5), GPIO_OUT_LOW, NULL},
	{"USB2_CTL3",           LM4_GPIO_D, (1<<6), GPIO_OUT_LOW, NULL},
	{"USB2_ENABLE",         LM4_GPIO_D, (1<<7), GPIO_OUT_LOW, NULL},
	{"USB2_ILIM_SEL",       LM4_GPIO_E, (1<<0), GPIO_OUT_LOW, NULL},
};

/* ADC channels. Must be in the exactly same order as in enum adc_channel. */
const struct adc_t adc_channels[ADC_CH_COUNT] =
{
	/* EC internal temperature is calculated by
	 * 273 + (295 - 450 * ADC_VALUE / ADC_READ_MAX) / 2
	 * = -225 * ADC_VALUE / ADC_READ_MAX + 420.5
	 */
	{"ECTemp", LM4_ADC_SEQ0, -225, ADC_READ_MAX, 420,
	 LM4_AIN_NONE, 0x0e /* TS0 | IE0 | END0 */},

	/* Charger current is mapped from 0~4000mA to 0~1.6V.
	 * And ADC maps 0~3.3V to ADC_READ_MAX.
	 */
	{"ChargerCurrent", LM4_ADC_SEQ1, 33 * 4000, ADC_READ_MAX * 16, 0,
	 LM4_AIN(ADC_IN0), 0x06 /* IE0 | END0 */},
};

void configure_board(void)
{
}