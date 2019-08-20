/* SPDX-License-Identifier: GPL-2.0+ */
/*
 *  (C) Copyright 2019 Hilscher Gesellschaft fuer Systemautomation mbH
 *
 */

#ifndef __CONFIG_H
#define __CONFIG_H

#include <linux/sizes.h>

#define CONFIG_SYS_SDRAM_BASE	0x40000000
#define CONFIG_SYS_LOAD_ADDR	0x05000000
#define CONFIG_SYS_INIT_SP_ADDR	0x0507F000
#define CONFIG_SYS_MALLOC_LEN	(8 * SZ_1M)

/* Kernel/Fitimage Load address in env */
#define CONFIG_LOADADDR		0x42000000
#define CONFIG_EXTRA_ENV_SETTINGS \
	"fdtaddr=0x44000000\0"    \
	"fdt_high=0x44010000\0"   \
	"bootargs_legacy=root=/dev/mmcblk0p2 rw console=ttyAMA0,115200 earlyprintk\0" \
	"bootcmd_legacy=fatload mmc 0:1 ${loadaddr} fitImage; setenv bootargs ${bootargs_legacy}; bootm\0"

#define CONFIG_BOOTCOMMAND	"run bootcmd_legacy"

#endif /* __CONFIG_H */
