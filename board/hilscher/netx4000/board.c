// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2019 Hilscher Gesellschaft fuer Systemautomation mbH
 */

#include <common.h>
#include <asm/io.h>
#include <config.h>
#include "device-label.h"

DECLARE_GLOBAL_DATA_PTR;

/*
 * board_init - early hardware init
 */
int board_init(void)
{
	/*
	 * Address of boot parameters passed to kernel
	 * Use default offset 0x100
	 */
	gd->bd->bi_boot_params = CONFIG_SYS_SDRAM_BASE + 0x100;

	return 0;
}

int misc_init_r(void)
{
	read_flash_device_label();

	return 0;
}

uint32_t netx4000_ddr_init(void);
/*
 * dram_init - sets u-boot's idea of sdram size
 */
int dram_init(void)
{
	gd->ram_size = netx4000_ddr_init();

	return 0;
}

int dram_init_banksize(void)
{
	gd->bd->bi_dram[0].start = CONFIG_SYS_SDRAM_BASE;
	gd->bd->bi_dram[0].size = gd->ram_size;

	return 0;
}
