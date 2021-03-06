// SPDX-License-Identifier: GPL-2.0+
/*
 * Quad Serial Peripheral Interface (QSPI) driver for Hilscher netX4000 based platforms
 *
 * Copyright 2019 Hilscher Gesellschaft fuer Systemautomation mbH
 */

#include <clk.h>
#include <common.h>
#include <malloc.h>
#include <spi.h>
#include <linux/io.h>
#include <dm.h>
#include <dm/device_compat.h>

#define NUM_CHIPSELECT  1
#define FIFO_DEPTH      16

#define setBF(val,regpos)  (((val) & regpos##_mask) << regpos##_shift)
#define getBF(val,regpos)  (((val) >> regpos##_shift) & regpos##_mask)

/*  Regdef */

#define SQI_CR0             0x00
#define CR0_FilterIn_mask           0x1
#define CR0_FilterIn_shift          27
#define CR0_FilterIn(val)           setBF(val,CR0_FilterIn)
#define CR0_SioCfg_mask             0x3
#define CR0_SioCfg_shift            22
#define CR0_SioCfg(val)             setBF(val,CR0_SioCfg)
#define CR0_SckMuladd_mask          0xfff
#define CR0_SckMuladd_shift         8
#define CR0_SckMuladd(val)          setBF(val,CR0_SckMuladd)
#define CR0_SckPhase_mask           0x1
#define CR0_SckPhase_shift          7
#define CR0_SckPhase(val)           setBF(val,CR0_SckPhase)
#define CR0_SckPol_mask             0x1
#define CR0_SckPol_shift            6
#define CR0_SckPol(val)             setBF(val,CR0_SckPol)
#define CR0_Datasize_mask           0xf
#define CR0_Datasize_shift          0
#define CR0_Datasize(val)           setBF(val,CR0_Datasize)

#define SQI_CR1             0x04
#define CR1_RxFifoClr_mask          0x1
#define CR1_RxFifoClr_shift         28
#define CR1_RxFifoClr(val)          setBF(val,CR1_RxFifoClr)
#define CR1_RxFifoWm_mask           0xf
#define CR1_RxFifoWm_shift          24
#define CR1_RxFifoWm(val)           setBF(val,CR1_RxFifoWm)
#define CR1_TxFifoClr_mask          0x1
#define CR1_TxFifoClr_shift         20
#define CR1_TxFifoClr(val)          setBF(val,CR1_TxFifoClr)
#define CR1_TxFifoWm_mask           0xf
#define CR1_TxFifoWm_shift          16
#define CR1_TxFifoWm(val)           setBF(val,CR1_TxFifoWm)
#define CR1_SpiTransCtrl_mask       0x1
#define CR1_SpiTransCtrl_shift      12
#define CR1_SpiTransCtrl(val)       setBF(val,CR1_SpiTransCtrl)
#define CR1_FssStatic_mask          0x1
#define CR1_FssStatic_shift         11
#define CR1_FssStatic(val)          setBF(val,CR1_FssStatic)
#define CR1_Fss_mask                0x7
#define CR1_Fss_shift               8
#define CR1_Fss(val)                setBF(val,CR1_Fss)
#define CR1_SqiEn_mask              0x1
#define CR1_SqiEn_shift             1
#define CR1_SqiEn(val)              setBF(val,CR1_SqiEn)

#define SQI_DR              0x08

#define SQI_SR              0x0c
#define SR_RxFifoLevel_mask         0x1f
#define SR_RxFifoLevel_shift        24
#define SR_TxFifoLevel_mask         0x1f
#define SR_TxFifoLevel_shift        16
#define SR_Busy_mask                0x1
#define SR_Busy_shift               4
#define SR_Busy(val)                setBF(val,SR_Busy)
#define SR_RxFifoNotEmpty_mask      0x1
#define SR_RxFifoNotEmpty_shift     2
#define SR_RxFifoNotEmpty(val)      setBF(val,SR_RxFifoNotEmpty)
#define SR_TxFifoNotFull_mask       0x1
#define SR_TxFifoNotFull_shift      1
#define SR_TxFifoNotFull(val)       setBF(val,SR_TxFifoNotFull)


#define SQI_TCR             0x10
#define TCR_MsByteFirst_mask        0x1
#define TCR_MsByteFirst_shift       29
#define TCR_MsByteFirst(val)        setBF(val,TCR_MsByteFirst)
#define TCR_MsBitFirst_mask         0x1
#define TCR_MsBitFirst_shift        28
#define TCR_MsBitFirst(val)         setBF(val,TCR_MsBitFirst)
#define TCR_Duplex_mask             0x3
#define TCR_Duplex_shift            26
#define TCR_Duplex(val)             setBF(val,TCR_Duplex)
#define TCR_Mode_mask               0x3
#define TCR_Mode_shift              24
#define TCR_Mode(val)               setBF(val,TCR_Mode)
#define TCR_StartTransfer_mask      0x1
#define TCR_StartTransfer_shift     23
#define TCR_StartTransfer(val)      setBF(val,TCR_StartTransfer)
#define TCR_TxOe_mask               0x1
#define TCR_TxOe_shift              22
#define TCR_TxOe(val)               setBF(val,TCR_TxOe)
#define TCR_TxOut_mask              0x1
#define TCR_TxOut_shift             21
#define TCR_TxOut(val)              setBF(val,TCR_TxOut)
#define TCR_TransferSize_mask       0x7ffff
#define TCR_TransferSize_shift      0
#define TCR_TransferSize(val)       setBF(val,TCR_TransferSize)

#define SQI_IRQ_MASK        0x14
#define SQI_IRQ_RAW         0x18
#define SQI_IRQ_MASKED      0x1c
#define SQI_IRQ_CLEAR       0x20
#define IRQ_SqiRomErr_mask          0x1
#define IRQ_SqiRomErr_shift         8
#define IRQ_SqiRomErr(val)          setBF(val,IRQ_SqiRomErr) // SQIROM error interrupt mask
#define IRQ_TransferEnd_mask        0x1
#define IRQ_TransferEnd_shift       7
#define IRQ_TransferEnd(val)        setBF(val,IRQ_TransferEnd) // Transfer end interrupt mask
#define IRQ_TxFifoEmpty_mask        0x1
#define IRQ_TxFifoEmpty_shift       6
#define IRQ_TxFifoEmpty(val)        setBF(val,IRQ_TxFifoEmpty) // Transmit FIFO empty interrupt mask (for netx100/500 compliance)
#define IRQ_RxFifoFull_mask         0x1
#define IRQ_RxFifoFull_shift        5
#define IRQ_RxFifoFull(val)         setBF(val,IRQ_RxFifoFull) // Receive FIFO full interrupt mask (for netx100/500 compliance)
#define IRQ_RxFifoNotEmpty_mask     0x1
#define IRQ_RxFifoNotEmpty_shift    4
#define IRQ_RxFifoNotEmpty(val)     setBF(val,IRQ_RxFifoNotEmpty) // Receive FIFO not empty interrupt mask (for netx100/500 compliance)
#define IRQ_TxFifoWm_mask           0x1
#define IRQ_TxFifoWm_shift          3
#define IRQ_TxFifoWm(val)           setBF(val,IRQ_TxFifoWm) // Transmit FIFO interrupt mask
#define IRQ_RxFifoWm_mask           0x1
#define IRQ_RxFifoWm_shift          2
#define IRQ_RxFifoWm(val)           setBF(val,IRQ_RxFifoWm) // Receive FIFO interrupt mask
#define IRQ_RxFifoTimeout_mask      0x1
#define IRQ_RxFifoTimeout_shift     1
#define IRQ_RxFifoTimeout(val)      setBF(val,IRQ_RxFifoTimeout) // Receive timeout interrupt mask
#define IRQ_RxFifoOverrun_mask      0x1
#define IRQ_RxFifoOverrun_shift     0
#define IRQ_RxFifoOverrun(val)      setBF(val,IRQ_RxFifoOverrun) // Receive FIFO overrun interrupt mask

#define SQI_DMACR           0x24
#define DMACR_TxDmaEn_mask          0x1
#define DMACR_TxDmaEn_shift         1
#define DMACR_TxDmaEn(val)          setBF(val,DMACR_TxDmaEn)
#define DMACR_RxDmaEn_mask          0x1
#define DMACR_RxDmaEn_shift         0
#define DMACR_RxDmaEn(val)          setBF(val,DMACR_RxDmaEn)

#define SQI_PIO_OUT         0x28
//#define reserved            0x2c
#define SQI_PIO_OE          0x30
#define SQI_PIO_IN          0x34
#define SQI_SQIROM_CFG      0x38

#define SQI_SQIROM_CFG_ENABLE 0x00000001
//#define reserved            0x3c

struct netx4000_qspi_priv {
	struct spi_slave slave;
	struct udevice *dev;
	void *base;

	uint mode;
	uint speed;
};

static void netx4000_qspi_set_cs(struct netx4000_qspi_priv *priv, int active)
{
	uint32_t val32, cs_val;

	/* qspi cs output is inverted, so 0 means a 1 on the pins */
	if(priv->mode & SPI_CS_HIGH)
		cs_val = active ? 0 : 1;
	else
		cs_val = active ? 1 : 0;

	val32 = ioread32(priv->base + SQI_CR1);
	val32 &= ~(CR1_FssStatic(-1) | CR1_Fss(-1)); /* clear affected bits */
	iowrite32(CR1_FssStatic(1) | CR1_Fss(cs_val) | val32, priv->base + SQI_CR1);

	return;
}

static int netx4000_qspi_claim_bus(struct udevice *dev)
{
	struct udevice *bus = dev->parent;
	struct netx4000_qspi_priv *priv = dev_get_priv(bus);
	uint32_t val32;

	/* Enable SQI unit */
	val32 = ioread32(priv->base + SQI_CR0);
	val32 &= ~(CR0_SioCfg(-1));
	iowrite32(val32|CR0_SioCfg(1 /* all IO pins */), priv->base + SQI_CR0);
	iowrite32(CR1_SqiEn(1), priv->base + SQI_CR1);

	return 0;
}

static int netx4000_qspi_release_bus(struct udevice *dev)
{
	struct udevice *bus = dev->parent;
	struct netx4000_qspi_priv *priv = dev_get_priv(bus);

	iowrite32(CR1_SqiEn(0), priv->base + SQI_CR1);

	return 0;
}

static int netx4000_qspi_set_speed(struct udevice *dev, uint speed)
{
	struct netx4000_qspi_priv *priv = dev_get_priv(dev);
	uint32_t val32, sck_muladd;

	sck_muladd = max( (u32)((speed / 100 * 4096) / 1000000), (u32)1);

	val32 = ioread32(priv->base + SQI_CR0);
	val32 &= ~(CR0_SckMuladd(-1));
	iowrite32(CR0_SckMuladd(sck_muladd) | val32,  priv->base + SQI_CR0);

	priv->speed = sck_muladd * 1000000 / 4096 * 100;

	return 0;
}

static int netx4000_qspi_set_mode(struct udevice *dev, uint mode)
{
	struct netx4000_qspi_priv *priv = dev_get_priv(dev);
	uint32_t val32;

	val32 = ioread32(priv->base + SQI_CR0);
	val32 &= ~(CR0_SckPhase(-1) & CR0_SckPol(-1)); /* clear affected bits */
	iowrite32(CR0_SckPhase(!!(mode & SPI_CPHA)) | CR0_SckPol(!!(mode & SPI_CPOL)) | val32,  priv->base + SQI_CR0);

	priv->mode = mode;

	return 0;
}

static int netx4000_qspi_xfer(struct udevice *dev, unsigned int bitlen,
				const void *dout, void* din, unsigned long flags)
{
	struct udevice *bus = dev->parent;
	struct netx4000_qspi_priv *priv = dev_get_priv(bus);
	int rc = 0;
	uint32_t val32;
	uint32_t bytes = bitlen / 8;
	uint32_t offset = 0;

	/* Setup bitlen, frequency has been set by set_speed already */
	val32 = ioread32(priv->base + SQI_CR0);
	val32 &= ~(CR0_Datasize(-1)); /* clear affected bits */
	iowrite32(CR0_Datasize(8 - 1) | val32,  priv->base + SQI_CR0);

	val32 = ioread32(priv->base + SQI_SR);
	if (getBF(val32, SR_RxFifoLevel) || getBF(val32, SR_TxFifoLevel)) {
		dev_warn(priv->dev, "%s: unexpected fifo level found => clearing rx and tx fifo\n", __func__);
		val32 = ioread32(priv->base + SQI_CR1);
		iowrite32(val32 | CR1_RxFifoClr(1) | CR1_TxFifoClr(1), priv->base + SQI_CR1);
	}

	/* Setup mode */
	val32 = TCR_MsBitFirst(1);
	val32 |= TCR_Mode(0/*1 Bit Mode*/) | TCR_Duplex(3/*FDX*/);
	iowrite32(val32, priv->base + SQI_TCR);

	if(flags & SPI_XFER_BEGIN)
		netx4000_qspi_set_cs(priv, 1);

	while(bytes > 0) {
		/* FIFO is always empty, so fill it up to max and read out RX */
		uint32_t chunk_size = min((u32)FIFO_DEPTH, (u32)(bytes - offset));

		for(val32 = 0; val32 < chunk_size; val32++) {
			if(dout)
				iowrite32(*((u8*)dout + offset + val32), priv->base + SQI_DR);
			else
				iowrite32(0xFF, priv->base + SQI_DR);
		}

		for(val32 = 0; val32 < chunk_size; val32++) {
			while(getBF(ioread32(priv->base + SQI_SR), SR_RxFifoNotEmpty) == 0) ;
			if(din)
				*((u8*)din + offset + val32) = ioread32(priv->base + SQI_DR) & 0xFF;
			else
				ioread32(priv->base + SQI_DR);
		}

		offset += chunk_size;
		bytes  -= chunk_size;
	}

	if(flags & SPI_XFER_END)
		netx4000_qspi_set_cs(priv, 0);

	return rc;
}

static void netx4000_qspi_reset_controller(struct netx4000_qspi_priv *priv)
{
	u32 cr1, tcr;

	if (!ioread32(priv->base + SQI_SQIROM_CFG) & SQI_SQIROM_CFG_ENABLE)
		return;

	/* Reset in case ROM Loader left flash in 4 Bit mode */
	dev_info(priv->dev, "SQIROM was enabled at startup. Disabling now!");
	iowrite32(0, priv->base + SQI_SQIROM_CFG);

	/* ROM Loader keeps it in 4 Bit TX mode, so use it as it is */

	/* Assert CS */
	cr1 = ioread32(priv->base + SQI_CR1);
	iowrite32(cr1 | CR1_FssStatic(1) | CR1_Fss(1), priv->base + SQI_CR1);

	/* Send 8 clocks with 0xf to exit fast read mode */
	iowrite32(0xFFFFFFFF, priv->base + SQI_DR);

	tcr = ioread32(priv->base + SQI_TCR);
	tcr &= ~(TCR_TransferSize(-1));
	tcr |= (TCR_TransferSize(3) | TCR_StartTransfer(1));
	iowrite32(tcr, priv->base + SQI_TCR);
	while(ioread32(priv->base + SQI_SR) & SR_Busy(1)) ;

	/* De-assert CS */
	cr1 &= ~(CR1_Fss(-1));
	iowrite32(cr1, priv->base + SQI_CR1);

	iowrite32(CR1_SqiEn(0), priv->base + SQI_CR1);
}

static int netx4000_qspi_probe(struct udevice *dev)
{
	struct netx4000_qspi_priv *priv = dev_get_priv(dev);

	priv->dev = dev;

	/* Set some sane defaults */
	priv->speed = 100000;

	/* Read the register base address from DT and map it */
	priv->base = dev_remap_addr(dev);
	if (!priv->base)
		return -ENODEV;

	netx4000_qspi_reset_controller(priv);

	return 0;
}

/* ------------------------------------------------------------------------- */

static const struct dm_spi_ops netx4000_qspi_ops = {
	.claim_bus	= netx4000_qspi_claim_bus,
	.release_bus	= netx4000_qspi_release_bus,
	.xfer		= netx4000_qspi_xfer,
	.set_speed	= netx4000_qspi_set_speed,
	.set_mode	= netx4000_qspi_set_mode,
};

static struct udevice_id netx4000_qspi_ids[] = {
	{ .compatible = "hilscher,qspi-netx4000", },
	{ /* sentinel */ },
};

U_BOOT_DRIVER(netx4000_qspi) = {
        .name   = "netx4000_qspi",
        .id     = UCLASS_SPI,
        .of_match = netx4000_qspi_ids,
        .ops    = &netx4000_qspi_ops,
        .priv_auto_alloc_size = sizeof(struct netx4000_qspi_priv),
        .probe  = netx4000_qspi_probe,
};
