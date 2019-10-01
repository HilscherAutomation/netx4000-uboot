/*
* GPIO driver for Hilscher netX4000 based platforms
*
* drivers/gpio/netx4000-gpio.c
*
* (C) Copyright 2019 Hilscher Gesellschaft fuer Systemautomation mbH
* http://www.hilscher.com
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License as
* published by the Free Software Foundation; version 2 of
* the License.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.test
*
*/

#include <common.h>
#include <dm.h>
#include <asm/gpio.h>
#include <dm/pinctrl.h>
#include <asm/io.h>
#include <errno.h>

struct netx4000_gpio_regs {
	u32 in;          /* value of the external input pin */
	u32 in_mask;     /* masks out values of 'in' */
	u32 out;         /* value of the output pins */
	u32 out_mask;    /* maks for 'out' */
	u32 toggle;      /* toggle (XOR) output data ('out') */
	u32 out_mask1;   /* alternative value for 'out' */
	u32 oe;          /* sets corresponding oe-pin active high */
	u32 irq_source;  /* irq source */
	u32 irq_pedge;   /* 1-> sensitive on rising edge */
	u32 irq_nedge;   /* 1-> sensitive on falling edge */
};

struct priv_data {
	struct udevice *dev;
	struct netx4000_gpio_regs __iomem *regs;
	uint32_t assigned;
};

static int netx4000_gpio_get_function(struct udevice *dev, unsigned offset)
{
	struct priv_data *pd = dev_get_priv(dev);

	if (readl(&pd->regs->oe) & BIT(offset))
		return GPIOF_OUTPUT;
	else
		return GPIOF_INPUT;
}

static int netx4000_gpio_get_value(struct udevice *dev, unsigned offset)
{
	struct priv_data *pd = dev_get_priv(dev);
	int rc;

	rc = netx4000_gpio_get_function(dev, offset);
	if (rc == GPIOF_OUTPUT)
		rc = !!(readl(&pd->regs->out) & BIT(offset));
	else
		rc = !!(readl(&pd->regs->in) & BIT(offset));

	return rc;
}

static int netx4000_gpio_set_value(struct udevice *dev, unsigned offset, int value)
{
	struct priv_data *pd = dev_get_priv(dev);

	if (value)
		clrsetbits_le32(&pd->regs->out, 0, BIT(offset));
	else
		clrsetbits_le32(&pd->regs->out, BIT(offset), 0);

	return 0;
}

static int netx4000_gpio_direction_input(struct udevice *dev, unsigned offset)
{
	struct priv_data *pd = dev_get_priv(dev);

	clrsetbits_le32(&pd->regs->oe, BIT(offset), 0);

	return 0;
}

static int netx4000_gpio_direction_output(struct udevice *dev, unsigned offset, int value)
{
	struct priv_data *pd = dev_get_priv(dev);

	clrsetbits_le32(&pd->regs->oe, 0, BIT(offset));
	netx4000_gpio_set_value(dev, offset, value);

	return 0;
}

static int netx4000_gpio_request(struct udevice *dev, unsigned offset, const char *label)
{
	struct priv_data *pd = dev_get_priv(dev);

	if (pd->assigned & BIT(offset))
		return -EBUSY;

	if (pinctrl_gpio_request(dev, offset) == -EBUSY)
		return -EBUSY;

	pd->assigned |= BIT(offset);

	return 0;
}

static int netx4000_gpio_free(struct udevice *dev, unsigned offset)
{
	struct priv_data *pd = dev_get_priv(dev);

	if (~pd->assigned & BIT(offset))
		return -EINVAL;

	pinctrl_gpio_free(dev, offset);

	pd->assigned &= ~BIT(offset);

	return 0;
}

static const struct dm_gpio_ops netx4000_gpio_ops = {
	.get_function = netx4000_gpio_get_function,
	.get_value = netx4000_gpio_get_value,
	.set_value = netx4000_gpio_set_value,
	.direction_input = netx4000_gpio_direction_input,
	.direction_output = netx4000_gpio_direction_output,
	.request = netx4000_gpio_request,
	.rfree = netx4000_gpio_free,
};

static int netx4000_gpio_probe(struct udevice *dev)
{
	struct gpio_dev_priv *uc_priv = dev_get_uclass_priv(dev);
	struct priv_data *pd = dev_get_priv(dev);
	fdt_addr_t addr = dev_read_addr(dev);

	if (addr == FDT_ADDR_T_NONE)
		return -EINVAL;

	pd->regs = (struct netx4000_gpio_regs *)addr;

	uc_priv->bank_name = dev->name;
	uc_priv->gpio_count = 32;

	return 0;
}

static const struct udevice_id netx4000_gpio_ids[] = {
	{ .compatible = "hilscher,netx4000-gpio" },
	{ }
};

U_BOOT_DRIVER(netx4000_gpio) = {
	.name	= "netx4000-gpio",
	.id	= UCLASS_GPIO,
	.of_match = netx4000_gpio_ids,
	.ops	= &netx4000_gpio_ops,
	.priv_auto_alloc_size = sizeof(struct priv_data),
	.probe	= netx4000_gpio_probe,
};
