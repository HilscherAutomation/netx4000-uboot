/*
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */
#ifndef _NETX4000_DMA_H_
#define _NETX4000_DMA_H_

#include <mach/netx4000_regs.h>

#define RAP_DMAC_REG_DST_END	0x00000018

#define RAP_DMAC_CH_0_SA	0x00000000
#define RAP_DMAC_CH_0_DA	0x00000004
#define RAP_DMAC_CH_0_TB	0x00000008
#define RAP_DMAC_CH_CHCTRL	0x00000028
#define RAP_DMAC_CH_CHCFG	0x0000002C

#define MSK_NX4000_RAP_DMAC_CH_CHCTRL_SETEN	0x00000001
#define MSK_NX4000_RAP_DMAC_CH_CHCTRL_CLREN	0x00000002
#define MSK_NX4000_RAP_DMAC_CH_CHCTRL_STG	0x00000004
#define MSK_NX4000_RAP_DMAC_CH_CHCTRL_SWRST	0x00000008
#define MSK_NX4000_RAP_DMAC_CH_CHCTRL_CLRSUS	0x00000200

#define MSK_NX4000_RAP_DMAC_CH_CHCFG_SAD	0x00100000
#define MSK_NX4000_RAP_DMAC_CH_CHCFG_TM		0x00400000
#define SRT_NX4000_RAP_DMAC_CH_CHCFG_DDS	16
#define SRT_NX4000_RAP_DMAC_CH_CHCFG_SDS	12
#define DFLT_VAL_NX4000_RAP_DMAC_CH_CHCFG	0x00000000

#endif /* _NETX4000_DMA_H_ */
