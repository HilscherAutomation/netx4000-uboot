/*
* Clock driver for Hilscher netx4000 based platforms
*
* drivers/clk/clk-netx4000.c
*
* (C) Copyright 2015 Hilscher Gesellschaft fuer Systemautomation mbH
* http://www.hilscher.com
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License as
* published by the Free Software Foundation; version 2 of
* the License.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
*/

#include <clk-uclass.h>
#include <common.h>
#include <dm.h>
#include <linux/io.h>

#include <mach/netx4000_regs.h>


/***********************************************
 * netX4000 Peripheral Clock
 **********************************************/

/* *************************/
/* Gated peripheral clocks */
/* *************************/
#define NOCPWRCTRL_OFS          (0x40)
#define NOCPWRMASK_OFS          (0x44)
#define NOCPWRSTAT_OFS          (0x48)
#define CLKCFG_OFS              (0x4C)

struct netx4000_periph_clk {
	u32 rate;
	u32 portctrl_mask;
};

static ulong netx4000_periph_get_rate(struct clk *clk)
{
	struct netx4000_periph_clk *priv = dev_get_priv(clk->dev);

	return priv->rate;
}

static int netx4000_periph_enable(struct clk *clk)
{
	struct netx4000_periph_clk *priv = dev_get_priv(clk->dev);
	u32 mask = priv->portctrl_mask;
	void __iomem *base = (void __iomem*)(NETX4000_SYSTEMCTRL_VIRT_BASE);
	uint32_t val;

	/* Check if this clock is allowed to be disabled/enabled by hardware */
	if((readl(base + NOCPWRMASK_OFS) & mask) != mask)
		return -EPERM;

	/* Enable clock and power */
	while( (readl(base + NOCPWRSTAT_OFS) & mask) != mask) {
		val = readl(base + CLKCFG_OFS);
		writel(val | mask, base + CLKCFG_OFS);
		val = readl(base + NOCPWRCTRL_OFS);
		writel(val | mask, base + NOCPWRCTRL_OFS);
	}

	return 0;
}

static int netx4000_periph_disable(struct clk *clk)
{
	struct netx4000_periph_clk *priv = dev_get_priv(clk->dev);
	u32 mask = priv->portctrl_mask;
	void __iomem *base = (void __iomem*)(NETX4000_SYSTEMCTRL_VIRT_BASE);
	uint32_t val;

	/* Check if this clock is allowed to be disabled/enabled by hardware */
	if((readl(base + NOCPWRMASK_OFS) & mask) != mask)
		return 0;

	/* Disable clock and power */
	val = readl(base + NOCPWRCTRL_OFS);
	writel(val & ~mask, base + NOCPWRCTRL_OFS);
	while( (readl(base + NOCPWRSTAT_OFS) & mask) == mask) ;
	val = readl(base + CLKCFG_OFS);
	writel(val & ~mask, base + CLKCFG_OFS);

	return 0;
}

static const struct clk_ops netx4000_periph_ops = {
	.get_rate = netx4000_periph_get_rate,
	.enable = netx4000_periph_enable,
	.disable = netx4000_periph_disable,
};

static int netx4000_periph_clk_probe(struct udevice *dev)
{
	struct netx4000_periph_clk *priv = dev_get_priv(dev);

	ofnode_read_u32(dev_ofnode(dev), "clock-frequency", &priv->rate);
	ofnode_read_u32(dev_ofnode(dev), "clock-mask", &priv->portctrl_mask);

	return 0;
}

static const struct udevice_id netx4000_periph_clk_id[] = {
        { .compatible = "hilscher,netx4000-periph-clock" },
        { }
};

U_BOOT_DRIVER(netx4000_periph_clk) = {
	.name = "netx4000-periph-clk",
	.id = UCLASS_CLK,
	.of_match = netx4000_periph_clk_id,
	.probe = netx4000_periph_clk_probe,
	.priv_auto_alloc_size = sizeof(struct netx4000_periph_clk),
	.ops = &netx4000_periph_ops,
	.flags = DM_FLAG_PRE_RELOC,
};


/***********************************************
 * netX4000 CPU Clock
 **********************************************/
struct netx4000_cpu_clk {
	u32 rate;
};

static ulong netx4000_cpu_get_rate(struct clk *clk)
{
	struct netx4000_cpu_clk *priv = dev_get_priv(clk->dev);

	return priv->rate;
}

static const struct clk_ops netx4000_cpu_ops = {
        .get_rate = netx4000_cpu_get_rate,
};

extern uint32_t get_netx4000_cpu_rate(void);
static int netx4000_cpu_clk_probe(struct udevice *dev)
{
	struct netx4000_cpu_clk *priv = dev_get_priv(dev);

	priv->rate = get_netx4000_cpu_rate();

	return 0;
}

static const struct udevice_id netx4000_cpu_clk_id[] = {
	{ .compatible = "hilscher,netx4000-cpu-clock" },
	{ }
};

U_BOOT_DRIVER(netx4000_cpu_clk) = {
	.name = "netx4000-cpu-clk",
	.id = UCLASS_CLK,
	.of_match = netx4000_cpu_clk_id,
	.probe = netx4000_cpu_clk_probe,
	.priv_auto_alloc_size = sizeof(struct netx4000_cpu_clk),
	.ops = &netx4000_cpu_ops,
	.flags = DM_FLAG_PRE_RELOC,
};
