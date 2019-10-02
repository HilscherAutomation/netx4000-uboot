/*
* MDIO-GPIO-Bitbang driver
*
* drivers/net/phy/mdio-gpio-bb.c
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
#include <miiphy.h>
#include <asm/gpio.h>
#include <dm/device_compat.h>
#include <linux/delay.h>

struct priv_data {
	struct udevice *dev;
	struct gpio_desc mdc_gpio;
	struct gpio_desc mdio_gpio;
};

#define mdio_gpio_bb_active(pd)       { dm_gpio_set_dir_flags(&pd->mdio_gpio, GPIOD_IS_OUT); }
#define mdio_gpio_bb_tristate(pd)     { dm_gpio_set_dir_flags(&pd->mdio_gpio, GPIOD_IS_IN); }
#define mdio_gpio_bb_set_mdio(pd, v)  { dm_gpio_set_value(&pd->mdio_gpio, v); }
#define mdio_gpio_bb_get_mdio(pd, v)  { *v = dm_gpio_get_value(&pd->mdio_gpio); }
#define mdio_gpio_bb_set_mdc(pd, v)   { dm_gpio_set_value(&pd->mdc_gpio, v); }
#define mdio_gpio_bb_delay(pd)        { udelay(1); }

/*****************************************************************************
 *
 * Utility to send the preamble, address, and register (common to read
 * and write).
 */
static void mdio_gpio_bb_pre(struct priv_data *pd, char read, unsigned char addr, unsigned char reg)
{
	int j;

	/*
	 * Send a 32 bit preamble ('1's) with an extra '1' bit for good measure.
	 * The IEEE spec says this is a PHY optional requirement.  The AMD
	 * 79C874 requires one after power up and one after a MII communications
	 * error.  This means that we are doing more preambles than we need,
	 * but it is safer and will be much more ropdt.
	 */

	mdio_gpio_bb_active(pd);
	mdio_gpio_bb_set_mdio(pd, 1);
	for (j = 0; j < 32; j++) {
		mdio_gpio_bb_set_mdc(pd, 0);
		mdio_gpio_bb_delay(pd);
		mdio_gpio_bb_set_mdc(pd, 1);
		mdio_gpio_bb_delay(pd);
	}

	/* send the start bit (01) and the read opcode (10) or write (10) */
	mdio_gpio_bb_set_mdc(pd, 0);
	mdio_gpio_bb_set_mdio(pd, 0);
	mdio_gpio_bb_delay(pd);
	mdio_gpio_bb_set_mdc(pd, 1);
	mdio_gpio_bb_delay(pd);
	mdio_gpio_bb_set_mdc(pd, 0);
	mdio_gpio_bb_set_mdio(pd, 1);
	mdio_gpio_bb_delay(pd);
	mdio_gpio_bb_set_mdc(pd, 1);
	mdio_gpio_bb_delay(pd);
	mdio_gpio_bb_set_mdc(pd, 0);
	mdio_gpio_bb_set_mdio(pd, read);
	mdio_gpio_bb_delay(pd);
	mdio_gpio_bb_set_mdc(pd, 1);
	mdio_gpio_bb_delay(pd);
	mdio_gpio_bb_set_mdc(pd, 0);
	mdio_gpio_bb_set_mdio(pd, !read);
	mdio_gpio_bb_delay(pd);
	mdio_gpio_bb_set_mdc(pd, 1);
	mdio_gpio_bb_delay(pd);

	/* send the PHY address */
	for (j = 0; j < 5; j++) {
		mdio_gpio_bb_set_mdc(pd, 0);
		if ((addr & 0x10) == 0) {
			mdio_gpio_bb_set_mdio(pd, 0);
		} else {
			mdio_gpio_bb_set_mdio(pd, 1);
		}
		mdio_gpio_bb_delay(pd);
		mdio_gpio_bb_set_mdc(pd, 1);
		mdio_gpio_bb_delay(pd);
		addr <<= 1;
	}

	/* send the register address */
	for (j = 0; j < 5; j++) {
		mdio_gpio_bb_set_mdc(pd, 0);
		if ((reg & 0x10) == 0) {
			mdio_gpio_bb_set_mdio(pd, 0);
		} else {
			mdio_gpio_bb_set_mdio(pd, 1);
		}
		mdio_gpio_bb_delay(pd);
		mdio_gpio_bb_set_mdc(pd, 1);
		mdio_gpio_bb_delay(pd);
		reg <<= 1;
	}
}

/*****************************************************************************
 *
 * Read a MII PHY register.
 *
 * Returns:
 *   0 on success
 */
static int mdio_gpio_bb_read(struct mii_dev *mdiodev, int addr, int devad, int reg)
{
	struct priv_data *pd = mdiodev->priv;
	unsigned short rdreg; /* register working value */
	int v;
	int j; /* counter */

	mdio_gpio_bb_pre (pd, 1, addr, reg);

	/* tri-state our MDIO I/O pin so we can read */
	mdio_gpio_bb_set_mdc(pd, 0);
	mdio_gpio_bb_tristate(pd);
	mdio_gpio_bb_delay(pd);
	mdio_gpio_bb_set_mdc(pd, 1);
	mdio_gpio_bb_delay(pd);

	/* check the turnaround bit: the PHY should be driving it to zero */
	mdio_gpio_bb_get_mdio(pd, &v);
	if (v != 0) {
		/* puts ("PHY didn't drive TA low\n"); */
		for (j = 0; j < 32; j++) {
			mdio_gpio_bb_set_mdc(pd, 0);
			mdio_gpio_bb_delay(pd);
			mdio_gpio_bb_set_mdc(pd, 1);
			mdio_gpio_bb_delay(pd);
		}
		/* There is no PHY, return */
		return -1;
	}

	mdio_gpio_bb_set_mdc(pd, 0);
	mdio_gpio_bb_delay(pd);

	/* read 16 bits of register data, MSB first */
	rdreg = 0;
	for (j = 0; j < 16; j++) {
		mdio_gpio_bb_set_mdc(pd, 1);
		mdio_gpio_bb_delay(pd);
		rdreg <<= 1;
		mdio_gpio_bb_get_mdio(pd, &v);
		rdreg |= (v & 0x1);
		mdio_gpio_bb_set_mdc(pd, 0);
		mdio_gpio_bb_delay(pd);
	}

	mdio_gpio_bb_set_mdc(pd, 1);
	mdio_gpio_bb_delay(pd);
	mdio_gpio_bb_set_mdc(pd, 0);
	mdio_gpio_bb_delay(pd);
	mdio_gpio_bb_set_mdc(pd, 1);
	mdio_gpio_bb_delay(pd);

#ifdef DEBUG
	printf("miiphy_read(0x%x) @ 0x%x = 0x%04x\n", reg, addr, rdreg);
#endif

	return rdreg;
}


/*****************************************************************************
 *
 * Write a MII PHY register.
 *
 * Returns:
 *   0 on success
 */
static int mdio_gpio_bb_write(struct mii_dev *mdiodev, int addr, int devad, int reg, u16 value)
{
	struct priv_data *pd = mdiodev->priv;
	int j; /* counter */

	mdio_gpio_bb_pre (pd, 0, addr, reg);

	/* send the turnaround (10) */
	mdio_gpio_bb_set_mdc(pd, 0);
	mdio_gpio_bb_set_mdio(pd, 1);
	mdio_gpio_bb_delay(pd);
	mdio_gpio_bb_set_mdc(pd, 1);
	mdio_gpio_bb_delay(pd);
	mdio_gpio_bb_set_mdc(pd, 0);
	mdio_gpio_bb_set_mdio(pd, 0);
	mdio_gpio_bb_delay(pd);
	mdio_gpio_bb_set_mdc(pd, 1);
	mdio_gpio_bb_delay(pd);

	/* write 16 bits of register data, MSB first */
	for (j = 0; j < 16; j++) {
		mdio_gpio_bb_set_mdc(pd, 0);
		if ((value & 0x00008000) == 0) {
			mdio_gpio_bb_set_mdio(pd, 0);
		} else {
			mdio_gpio_bb_set_mdio(pd, 1);
		}
		mdio_gpio_bb_delay(pd);
		mdio_gpio_bb_set_mdc(pd, 1);
		mdio_gpio_bb_delay(pd);
		value <<= 1;
	}

	/*
	 * Tri-state the MDIO line.
	 */
	mdio_gpio_bb_tristate(pd);
	mdio_gpio_bb_set_mdc(pd, 0);
	mdio_gpio_bb_delay(pd);
	mdio_gpio_bb_set_mdc(pd, 1);
	mdio_gpio_bb_delay(pd);

	return 0;
}

/* -------------------------------------------------------------------------- */

static int mdio_gpio_bb_probe(struct udevice *dev)
{
	struct priv_data *pd = dev_get_priv(dev);
	struct mii_dev *mdiodev;
	int rc;

	pd->dev = dev;

	if (gpio_request_by_name(dev, "mdc-gpio", 0, &pd->mdc_gpio, GPIOD_IS_OUT | GPIOD_IS_OUT_ACTIVE))
		gpio_request_by_name(dev, "gpios", 0, &pd->mdc_gpio, GPIOD_IS_OUT | GPIOD_IS_OUT_ACTIVE);
	if (gpio_request_by_name(dev, "mdio-gpio", 0, &pd->mdio_gpio, GPIOD_IS_IN))
		gpio_request_by_name(dev, "gpios", 1, &pd->mdio_gpio, GPIOD_IS_IN);

	if (!dm_gpio_is_valid(&pd->mdc_gpio))
		dev_err(dev, "Invalid or missing MDC gpio!\n");
	if (!dm_gpio_is_valid(&pd->mdio_gpio))
		dev_err(dev, "Invalid or missing MDIO gpio!\n");

	mdiodev = mdio_alloc();
	if (!mdiodev) {
		dev_err(dev, "Allocate MDIO memory failed!\n");
		return -ENOMEM;
	}

	strncpy(mdiodev->name, dev->name, MDIO_NAME_LEN);
	mdiodev->read = mdio_gpio_bb_read;
	mdiodev->write = mdio_gpio_bb_write;
	mdiodev->priv = pd;

	rc = mdio_register(mdiodev);
	if (rc < 0) {
		dev_err(dev, "Register MDIO interface failed!\n");
		return rc;
	}

	dev_dbg(dev, "MDIO interface successfully probed!\n");

	return 0;
}

/* -------------------------------------------------------------------------- */

static struct udevice_id mdio_gpio_bb_ids[] = {
	{ .compatible = "virtual,mdio-gpio", },
	{ /* sentinel */ },
};

U_BOOT_DRIVER(mdio_gpio) = {
	.name = "mdio-gpio-bb",
	.id = UCLASS_SIMPLE_BUS,
	.of_match = mdio_gpio_bb_ids,
	.priv_auto_alloc_size = sizeof(struct priv_data),
	.probe = mdio_gpio_bb_probe,
};
