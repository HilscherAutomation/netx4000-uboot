// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2019 Hilscher Gesellschaft fuer Systemautomation mbH
 */

#include <common.h>
#include <linux/io.h>

DECLARE_GLOBAL_DATA_PTR;

extern uint32_t get_netx4000_cpu_rate(void);

/* Global Timer */
#define GLOBAL_TIMER_OFFSET	0x200

/* Global Timer Counter Registers */
#define GTIMER_CNT_L		0x00
#define GTIMER_CNT_H		0x04

/* Global Timer Control Register */
#define GTIMER_CTRL		0x08

static void *get_global_timer_base(void)
{
	void *val;

	asm("mrc p15, 4, %0, c15, c0, 0" : "=r" (val) : : "memory");

	return val + GLOBAL_TIMER_OFFSET;
}

unsigned long timer_read_counter(void)
{
	/*
	 * ARM 64bit Global Timer is too much for our purpose.
	 * We use only lower 32 bit of the timer counter.
	 */
	return readl(get_global_timer_base() + GTIMER_CNT_L);
}

int timer_init(void)
{
	gd->arch.timer_rate_hz = get_netx4000_cpu_rate() / 2;

	/* enable timer */
	writel(1, get_global_timer_base() + GTIMER_CTRL);

	return 0;
}

ulong get_tbclk(void)
{
        return gd->arch.timer_rate_hz;
}
