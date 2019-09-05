#include <common.h>
#include <linux/io.h>

#define RAP_SYSCTRL_RSTCTRL 0xf8000050
#define RAP_SYSCTRL_RSTMASK 0xf8000054

void reset_cpu(unsigned long ignored)
{
	uint32_t reset_mask = 0;

	reset_mask = readl(RAP_SYSCTRL_RSTMASK);
	writel((reset_mask | 1), RAP_SYSCTRL_RSTMASK);
	writel(1, RAP_SYSCTRL_RSTCTRL);
	while (1) ;
}
