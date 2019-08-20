#include <common.h>
#include <cpu_func.h>

uint32_t get_netx4000_cpu_rate(void)
{
        volatile u32* ulRAPSysCtrlBootMode = (volatile u32*)0xf8000000;

        /* get current PLL speed */
        if ((*ulRAPSysCtrlBootMode & (1<<8)) == (1<<8))
                return (600*1000*1000);/* 600MHZ */
        else
                return (400*1000*1000);/* 400MHZ */
}

#if defined(CONFIG_DISPLAY_CPUINFO)
int print_cpuinfo(void)
{
	printf("SoC:   netX4000\n");
	return 0;
}
#endif /* CONFIG_DISPLAY_CPUINFO */

#ifndef CONFIG_SYS_DCACHE_OFF
void enable_caches(void)
{
        /* Enable D-cache. I-cache is already enabled in start.S */
        dcache_enable();
}
#endif
