#include <common.h>
#include <net.h>
#include <linux/ctype.h>
#include <asm/io.h>
#include <u-boot/crc.h>
#include <u-boot/sha256.h>
#include "device-label.h"

static struct fdl_content const* netx4000_check_fdl(u32 fdl_addr)
{
	struct fdl_header const *header = (struct fdl_header const*)fdl_addr;
	struct fdl_content const *content = (void*)header + sizeof(*header);
	struct fdl_footer const *footer = (void*)content + header->usContentSize;

	pr_info("FDL: Searching for FDL at 0x%p ...", header);

	while (1) {
		u32 crc;

		/* Verify header, footer, checksum */
		if (strncmp(header->abStartToken, "ProductData>", sizeof(header->abStartToken)) != 0) {
			u8 buf[sizeof(header->abStartToken)], i;

			for(i = 0; i < sizeof(header->abStartToken); i++) {
				buf[i] = (isprint(header->abStartToken[i])) ? header->abStartToken[i] : '.';
			}
			pr_err("FDL: Start token mismatch ('%.*s' instead of '%s')\n", sizeof(header->abStartToken), buf, "ProductData>");
			break;
		}
		if (header->usLabelSize > 4096 ) {
			pr_err("FDL:  Length exceeds 4KiB range (%d)\n", header->usLabelSize);
			break;
		}
		if (header->usContentSize > (4096 - sizeof(*header) - sizeof(*footer))) {
			pr_err("FDL:  Length exceeds 4KiB-32B range (%d)\n", header->usContentSize);
			break;
		}
		if (strncmp(footer->abEndToken, "<ProductData", sizeof(footer->abEndToken)) != 0) {
			u8 buf[sizeof(header->abStartToken)], i;

			for(i = 0; i < sizeof(header->abStartToken); i++) {
				buf[i] = (isprint(header->abStartToken[i])) ? header->abStartToken[i] : '.';
			}
			pr_err("FDL:  End token mismatch ('%.*s' instead of '%s')\n", sizeof(footer->abEndToken), buf, "<ProductData");
			break;
		}
		crc = crc32(0, (void*)content, header->usContentSize);
		if (footer->ulCRC != crc) {
			pr_err("FDL: CRC mismatch (0x%08x instead of 0x%08x)\n", footer->ulCRC, crc);
			break;
		}

		pr_info(" ... found\n");
		return content;
	}

	return NULL;
}

static void set_eth_mac(int eth_idx, u8 const* mac)
{
	char eth_if[9];
	char eth_mac[18];

	if(eth_idx == 0)
		sprintf(eth_if, "ethaddr");
	else
		sprintf(eth_if, "eth%daddr", eth_idx);

	sprintf(eth_mac, "%02X:%02X:%02X:%02X:%02X:%02X",
		mac[0], mac[1], mac[2],
		mac[3], mac[4], mac[5]);
	if(!env_get(eth_if))
		env_set(eth_if, eth_mac);
}

static void netx4000_create_local_mac_address(int eth_idx)
{
	u32 chipid[4];
	sha256_context ctx;
	u8 hash[SHA256_SUM_LEN];

	chipid[0] = readl(0xf80000b0);
	chipid[1] = readl(0xf80000b4);
	chipid[2] = readl(0xf80000b8);
	chipid[3] = readl(0xf80000bc);
	pr_debug("chipid: %08x-%08x-%08x-%08x\n", chipid[0], chipid[1], chipid[2], chipid[3]);

	sha256_starts(&ctx);
	sha256_update(&ctx, (void*)&chipid[0], sizeof(chipid[0]) * 4);
	sha256_finish(&ctx, hash);

	hash[5] += eth_idx;

	hash[0] &= ~0x1; /* mark it as individual mac address */
	hash[0] |= 0x2;  /* mark it as local mac address */

	set_eth_mac(eth_idx, hash);
}

void read_flash_device_label(void)
{
	struct fdl_content const* fdl;
	int i;

	/* Parse flash-device-label stored in SDRAM and initialize device label. */
	fdl = netx4000_check_fdl(CONFIG_FDL_ADDRESS);
	if (!fdl) {
		pr_err("FDL: !! Invalid or missing Flash-Device-Label @0x%08X !!!\n", CONFIG_FDL_ADDRESS);

		for(i=0;i<4;i++)
			netx4000_create_local_mac_address(i);

	} else {
		for(i=0;i<4;i++) {
			/* Check for valid MAC addresses */
			if (!is_valid_ethaddr(fdl->mac_app.mac[i].macaddr))
				netx4000_create_local_mac_address(i);
			else
				set_eth_mac(i, fdl->mac_app.mac[i].macaddr);
		}
	}
}
