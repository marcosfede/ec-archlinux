/* Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "board.h"
#include "console.h"
#include "dma.h"
#include "registers.h"
#include "timer.h"
#include "util.h"

/* Console output macros */
#define CPUTS(outstr) cputs(CC_DMA, outstr)
#define CPRINTF(format, args...) cprintf(CC_DMA, format, ## args)


struct dma_channel *dma_get_channel(int channel)
{
	struct dma_channel *chan;
	struct dma_ctlr *dma;

	/* Get a pointer to the correct controller and channel */
	ASSERT(channel < DMA_NUM_CHANNELS);
	if (channel < DMA1_NUM_CHANNELS) {
		dma = (struct dma_ctlr *)STM32_DMA1_BASE;
		chan = &dma->chan[channel];
	} else {
		dma = (struct dma_ctlr *)STM32_DMA2_BASE;
		chan = &dma->chan[channel - DMA1_NUM_CHANNELS];
	}

	return chan;
}

void dma_disable(unsigned channel)
{
	struct dma_channel *chan;

	chan = dma_get_channel(channel);

	if (REG32(&chan->ccr) & DMA_EN)
		REG32(&chan->ccr) &= ~DMA_EN;
}

/**
 * Prepare a channel for use and start it
 *
 * @param chan		Channel number to read (DMAC_...)
 * @param count		Number of bytes to transfer
 * @param periph	Pointer to peripheral data register
 * @param memory	Pointer to memory address
 * @param flags		DMA flags for the control register, normally:
				DMA_MINC_MASK |
 *				(DMA_DIR_FROM_MEM_MASK for tx
 *				0 for rx)
 */
static void prepare_channel(struct dma_channel *chan, unsigned count,
		void *periph, const void *memory, unsigned flags)
{
	uint32_t ctrl;

	if (REG32(&chan->ccr) & DMA_EN)
		REG32(&chan->ccr) &= ~DMA_EN;

	/* Following the order in Doc ID 15965 Rev 5 p194 */
	REG32(&chan->cpar) = (uint32_t)periph;
	REG32(&chan->cmar) = (uint32_t)memory;
	REG32(&chan->cndtr) = count;
	ctrl = DMA_PL_VERY_HIGH << DMA_PL_SHIFT;
	REG32(&chan->ccr) = ctrl;

	ctrl |= flags;
	ctrl |= 0 << 10; /* MSIZE (memory size in bytes) */
	ctrl |= 1 << 8;  /* PSIZE (16-bits for now) */
	REG32(&chan->ccr) = ctrl;
}

void dma_go(struct dma_channel *chan)
{
	/* Fire it up */
	REG32(&chan->ccr) |= DMA_EN;
}

void dma_prepare_tx(struct dma_channel *chan, unsigned count, void *periph,
	       const void *memory)
{
	prepare_channel(chan, count, periph, memory,
				DMA_MINC_MASK | DMA_DIR_FROM_MEM_MASK);
}

int dma_start_rx(unsigned channel, unsigned count, void *periph,
		 const void *memory)
{
	struct dma_channel *chan = dma_get_channel(channel);

	prepare_channel(chan, count, periph, memory, DMA_MINC_MASK);
	dma_go(chan);
	return 0;
}

int dma_bytes_done(struct dma_channel *chan, int orig_count)
{
	if (!(REG32(&chan->ccr) & DMA_EN))
		return 0;
	return orig_count - REG32(&chan->cndtr);
}


#ifdef CONFIG_DMA_HELP
void dma_dump(unsigned channel)
{
	struct dma_channel *chan;
	struct dma_ctlr *dma;

	/* Get a pointer to the correct controller and channel */
	ASSERT(channel < DMA_NUM_CHANNELS);
	dma = (struct dma_ctlr *)STM32_DMA1_BASE;

	chan = dma_get_channel(channel);
	CPRINTF("ccr=%x, cndtr=%x, cpar=%x, cmar=%x\n", chan->ccr,
		chan->cndtr, chan->cpar, chan->cmar);
	CPRINTF("chan %d, isr=%x, ifcr=%x\n",
		channel,
		(dma->isr >> (channel * 4)) & 0xf,
		(dma->ifcr >> (channel * 4)) & 0xf);
}

void dma_check(int channel, char *buff)
{
	struct dma_channel *chan;
	int count;
	int i;

	chan = dma_get_channel(channel);
	count = REG32(&chan->cndtr);
	CPRINTF("c=%d\n", count);
	udelay(1000 * 100);
	CPRINTF("c=%d\n",
		    REG32(&chan->cndtr));
	for (i = 0; i < count; i++)
		CPRINTF("%02x ", buff[i]);
	udelay(1000 * 100);
	CPRINTF("c=%d\n",
		    REG32(&chan->cndtr));
	for (i = 0; i < count; i++)
		CPRINTF("%02x ", buff[i]);
}

/* Run a check of memory-to-memory DMA */
void dma_test(void)
{
	unsigned channel = 3;
	struct dma_channel *chan;
	uint32_t ctrl;
	char periph[16], memory[16];
	unsigned count = sizeof(periph);
	int i;

	chan = dma_get_channel(channel);
	memset(memory, '\0', sizeof(memory));
	for (i = 0; i < count; i++)
		periph[i] = 10 + i;

	/* Following the order in Doc ID 15965 Rev 5 p194 */
	REG32(&chan->cpar) = (uint32_t)periph;
	REG32(&chan->cmar) = (uint32_t)memory;
	REG32(&chan->cndtr) = count;
	ctrl = DMA_PL_MEDIUM << DMA_PL_SHIFT;
	REG32(&chan->ccr) = ctrl;

	ctrl |= DMA_MINC_MASK; /* | DMA_DIR_FROM_MEM_MASK */;
	ctrl |= 1 << 14; /* MEM2MEM */
	ctrl |= 1 << 6;  /* PINC */
/*	ctrl |= 2 << 10; */
/*	ctrl |= 2 << 8; */
	REG32(&chan->ccr) = ctrl;

	ctrl |= DMA_EN;
	REG32(&chan->ccr) = ctrl;
	for (i = 0; i < count; i++)
		CPRINTF("%d/%d ", periph[i], memory[i]);
	CPRINTF("\ncount=%d\n", REG32(&chan->cndtr));
}
#endif /* CONFIG_DMA_HELP */

void dma_init(void)
{
	/* Enable DMA1, we don't support DMA2 yet */
	STM32_RCC_AHBENR |= RCC_AHBENR_DMA1EN;
}
