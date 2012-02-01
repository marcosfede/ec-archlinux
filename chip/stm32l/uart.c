/* Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* USART driver for Chrome EC */

#include <stdarg.h>

#include "board.h"
#include "config.h"
#include "registers.h"
#include "task.h"
#include "uart.h"
#include "util.h"

/* Baud rate for UARTs */
#define BAUD_RATE 115200

/* Console USART index */
#define UARTN CONFIG_CONSOLE_UART

/* record last TX control action */
static int should_stop;

void uart_tx_start(void)
{
	STM32L_USART_CR1(UARTN) |= 0x80;
	should_stop = 0;
	task_trigger_irq(STM32L_IRQ_USART(UARTN));
}

void uart_tx_stop(void)
{
	STM32L_USART_CR1(UARTN) &= ~0x80;
	should_stop = 1;
}

int uart_tx_stopped(void)
{
	return !(STM32L_USART_CR1(UARTN) & 0x80);
}

void uart_tx_flush(void)
{
	while (!(STM32L_USART_SR(UARTN) & 0x80))
		;
}

int uart_tx_ready(void)
{
	return STM32L_USART_SR(UARTN) & 0x80;
}

int uart_rx_available(void)
{
	return STM32L_USART_SR(UARTN) & 0x20;
}

void uart_write_char(char c)
{
	STM32L_USART_DR(UARTN) = c;
}

int uart_read_char(void)
{
	return STM32L_USART_DR(UARTN);
}

void uart_disable_interrupt(void)
{
	task_disable_irq(STM32L_IRQ_USART(UARTN));
}

void uart_enable_interrupt(void)
{
	task_enable_irq(STM32L_IRQ_USART(UARTN));
}

/* Interrupt handler for console USART */
static void uart_interrupt(void)
{
	/*
	 * Disable the TX empty interrupt before filling the TX buffer since it
	 * needs an actual write to DR to be cleared.
	 */
	STM32L_USART_CR1(UARTN) &= ~0x80;

	/* Read input FIFO until empty, then fill output FIFO */
	uart_process();

	/*
	 * Re-enable TX empty interrupt only if it was not disabled by
	 * uart_process.
	 */
	if (!should_stop)
		STM32L_USART_CR1(UARTN) |= 0x80;
}
DECLARE_IRQ(STM32L_IRQ_USART(UARTN), uart_interrupt, 1);

int uart_init(void)
{
	/*
	 * Check that the UART parameters used for panic/watchdog are matching
	 * the console USART parameters.
	*/
	BUILD_ASSERT(STM32L_USART_BASE(UARTN) == CONFIG_UART_ADDRESS);

	/* Enable USART clock */
	if (UARTN == 1)
		STM32L_RCC_APB2ENR |= 1 << 14; /* USART1 */
	else if (UARTN == 2)
		STM32L_RCC_APB1ENR |= 1 << 17; /* USART2 */
	else if (UARTN == 3)
		STM32L_RCC_APB1ENR |= 1 << 18; /* USART3 */

	/* UART enabled, 8 Data bits, oversampling x16, no parity,
	 * RXNE interrupt, TX and RX enabled.
	 */
	STM32L_USART_CR1(UARTN) = 0x202C;

	/* 1 stop bit, no fancy stuff */
	STM32L_USART_CR2(UARTN) = 0x0000;

	/* DMA disabled, special modes disabled, error interrupt disabled */
	STM32L_USART_CR3(UARTN) = 0x0000;

	/* Select the baud rate
	 * using x16 oversampling (OVER8 == 0)
	 */
	STM32L_USART_BRR(UARTN) = DIV_ROUND_NEAREST(CPU_CLOCK, BAUD_RATE);

	/* Enable interrupts */
	task_enable_irq(STM32L_IRQ_USART(UARTN));

	return EC_SUCCESS;
}
