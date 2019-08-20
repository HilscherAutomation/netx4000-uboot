#include <common.h>
#include <asm/io.h>
#include <mach/netx4000_ddr.h>
#include <mach/netx4000_dma.h>

extern int ddr400_init(void);
extern int ddr600_init(void);
extern uint32_t get_netx4000_cpu_rate(void);

static uint32_t get_sdram_size(void) {
	uint32_t tmp;
	uint32_t max_row, max_col;
	uint32_t row_diff, col_diff, bank_diff;
	uint32_t dp_width;
	uint32_t size;

	tmp = readl(DENALI_CTL(1));
	max_col = (tmp & DENALI_CTL1_MAXCOL_MASK) >> DENALI_CTL1_MAXCOL_SHIFT;
	max_row = (tmp & DENALI_CTL1_MAXROW_MASK) >> DENALI_CTL1_MAXROW_SHIFT;

	tmp = readl(DENALI_CTL(53));
	col_diff  = (tmp & DENALI_CTL53_COL_DIFF_MASK) >> DENALI_CTL53_COL_DIFF_SHIFT;
	row_diff  = (tmp & DENALI_CTL53_ROW_DIFF_MASK) >> DENALI_CTL53_ROW_DIFF_SHIFT;
	bank_diff = (tmp & DENALI_CTL53_BANK_DIFF_MASK)>> DENALI_CTL53_BANK_DIFF_SHIFT;

        tmp = readl(DENALI_CTL(58));
	dp_width = (tmp & DENALI_CTL58_REDUC_MASK) ? 2: 4; /* 16 / 32 Bit */

	size =  (1 << (max_col - col_diff));
	size *= (1 << (max_row - row_diff));
	size *= (8 >> bank_diff);
	size *= dp_width;

	return size;
}

static uint64_t fill_char[2];
static void ddr_ecc_init(uint32_t sdram_size)
{
	uint32_t ram_size = sdram_size & ~0x1;
	uint32_t ram_addr = NETX4000_DDR_ADDR_SPACE_START;
	uint32_t chunk_size = ram_size / 8;
	void __iomem *dmareg = (void __iomem *)NETX4000_DMAC0_REG;
	int ch;
	void __iomem *dmach;

	/* Prepare source buffer for DMA transfer */
	memset((void*)&fill_char[0], 0, sizeof(fill_char));

	/* Setup 8 DMA channels to clear DDR */
	for(ch=0;ch<8;ch++) {
		dmach = (void __iomem *)(NETX4000_DMAC0 + ch * 0x40);

		/* Reset */
		writel(MSK_NX4000_RAP_DMAC_CH_CHCTRL_CLREN, dmach + RAP_DMAC_CH_CHCTRL);
		writel(MSK_NX4000_RAP_DMAC_CH_CHCTRL_SWRST, dmach + RAP_DMAC_CH_CHCTRL);

		/* Setup and start */
		writel((uint32_t)&fill_char[0], dmach + RAP_DMAC_CH_0_SA);
		writel(ram_addr + ch * chunk_size, dmach + RAP_DMAC_CH_0_DA);
		writel(chunk_size, dmach + RAP_DMAC_CH_0_TB);
		writel(MSK_NX4000_RAP_DMAC_CH_CHCFG_SAD |        /* no source increment */
		       MSK_NX4000_RAP_DMAC_CH_CHCFG_TM  |        /* Block transfer */
		       (4 << SRT_NX4000_RAP_DMAC_CH_CHCFG_DDS) | /* 128 bit dest size */
		       (4 << SRT_NX4000_RAP_DMAC_CH_CHCFG_SDS),  /* 128 Bit source size */
		       dmach + RAP_DMAC_CH_CHCFG);
		writel(MSK_NX4000_RAP_DMAC_CH_CHCTRL_SETEN |
		       MSK_NX4000_RAP_DMAC_CH_CHCTRL_CLRSUS,
		       dmach + RAP_DMAC_CH_CHCTRL);
		writel(MSK_NX4000_RAP_DMAC_CH_CHCTRL_STG, dmach + RAP_DMAC_CH_CHCTRL);  /* Software triggered DMA */
	}

	/* Wait for DMA to finish */
	while ((readl(dmareg + RAP_DMAC_REG_DST_END) & 0xFF) != 0xFF)
		;

	/* Reset all DMA channels */
	for(ch=0;ch<8;ch++) {
		dmach = (void __iomem *)(NETX4000_DMAC0 + ch * 0x40);
		writel(MSK_NX4000_RAP_DMAC_CH_CHCTRL_CLREN, dmach + RAP_DMAC_CH_CHCTRL);
		writel(MSK_NX4000_RAP_DMAC_CH_CHCTRL_SWRST, dmach + RAP_DMAC_CH_CHCTRL);
		writel(DFLT_VAL_NX4000_RAP_DMAC_CH_CHCFG, dmach + RAP_DMAC_CH_CHCFG);
		writel(0, dmach + RAP_DMAC_CH_0_SA);
		writel(0, dmach + RAP_DMAC_CH_0_DA);
		writel(0, dmach + RAP_DMAC_CH_0_TB);
	}
}

#define AddressFilteringStartRegister  0xFAF10C00
#define AddressFilteringEndRegister    0xFAF10C04
static void fix_l2c_address_filtering_issue(void)
{
	*(uint32_t*)AddressFilteringStartRegister = 0x0;
	*(uint32_t*)AddressFilteringEndRegister = 0xc0000000;
	*(uint32_t*)AddressFilteringStartRegister = 0x40000001;
}

uint32_t netx4000_ddr_init(void)
{
	uint32_t cpu_rate;
	int ecc;
	uint32_t sdram_size;

	fix_l2c_address_filtering_issue();

	/* Initialize DDR controller */
	cpu_rate = get_netx4000_cpu_rate();
	if (cpu_rate == 400000000)
		ecc = ddr400_init();
	else if ((cpu_rate == 600000000))
		ecc = ddr600_init();
	else
		while (1); /* FIXME */

	sdram_size = get_sdram_size();
	if (sdram_size > 0x40000000) {
		memset((void*)0x80000000-32,0x55,32);
		memset((void*)0xc0000000-32,0xaa,32);
		/* Limit sdram_size if the upper GiB is inaccessible ('netX4000 RLXD' and 'netX4000 FULL v1'). */
		if (*(volatile uint32_t*)0x7ffffffc != 0x55555555)
			sdram_size = 0x40000001;
	}

	/* initialize memory to safely enable ecc */
	if (ecc)
		ddr_ecc_init(sdram_size);

	return sdram_size & ~0x1;
}
