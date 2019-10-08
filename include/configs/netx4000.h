/* SPDX-License-Identifier: GPL-2.0+ */
/*
 *  (C) Copyright 2019 Hilscher Gesellschaft fuer Systemautomation mbH
 *
 */

#ifndef __CONFIG_H
#define __CONFIG_H

#include <linux/sizes.h>

#define CONFIG_SYS_SDRAM_BASE	0x40000000
#define CONFIG_SYS_LOAD_ADDR	0x50000000
#define CONFIG_SYS_INIT_SP_ADDR	0x0507F000
#define CONFIG_SYS_MALLOC_LEN	(8 * SZ_1M)

/* Kernel/Fitimage Load address in env */
#define CONFIG_LOADADDR	CONFIG_SYS_LOAD_ADDR
#define CONFIG_EXTRA_ENV_SETTINGS \
	"initrd_high=0xffffffff\0"    \
	"fdt_high=0xffffffff\0"   \
	"mmcdev=0\0" \
	"mmcpart=1\0" \
	"script=boot.scr\0" \
	"loadbootscript=" \
		"load mmc ${mmcdev}:${mmcpart} ${loadaddr} ${script};\0" \
	"bootscript=echo Running bootscript from mmc ...; " \
		"source\0" \
	"bootargs_legacy=root=/dev/mmcblk0p2 rw rootwait console=ttyAMA0,115200 earlyprintk\0" \
	"bootcmd_legacy=load mmc 0:1 ${loadaddr} fitImage; setenv bootargs ${bootargs_legacy}; bootm\0"

#define CONFIG_BOOTCOMMAND	"mmc dev ${mmcdev}; if mmc rescan; then " \
					"if run loadbootscript; then " \
						"run bootscript; " \
					"else " \
						"run bootcmd_legacy; " \
					"fi; " \
				"else " \
					"run bootcmd_legacy;" \
				"fi"

#endif /* __CONFIG_H */

/* Ethernet need */
#ifdef CONFIG_DWC_ETH_QOS
#define CONFIG_SYS_NONCACHED_MEMORY	(1 * SZ_1M) /* 1M */
#endif
