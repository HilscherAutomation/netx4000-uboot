/* SPDX-License-Identifier: GPL-2.0+ */
/*
 *  (C) Copyright 2019 Hilscher Gesellschaft fuer Systemautomation mbH
 *
 */

#ifndef __CONFIG_H
#define __CONFIG_H

#include <linux/sizes.h>

#define CONFIG_ENV_SIZE		(8 * SZ_1K)

#define CONFIG_SYS_SDRAM_BASE	0x40000000
#define CONFIG_SYS_LOAD_ADDR	0x05000000
#define CONFIG_SYS_INIT_SP_ADDR	0x0507F000
#define CONFIG_SYS_MALLOC_LEN	(8 * SZ_1M)

//#define CONFIG_PL011_CLOCK      (100 * 1000 * 1000)
//#define CONFIG_PL01x_PORTS      {(void *)0xF8036000}

/* Kernel/Fitimage Load address in env */
#define CONFIG_LOADADDR		0x40000000

#define CONFIG_EXTRA_ENV_SETTINGS \
	"fdtaddr=0x42000000\0"

#endif /* __CONFIG_H */
