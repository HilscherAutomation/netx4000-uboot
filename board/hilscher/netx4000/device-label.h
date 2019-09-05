#ifndef __DEVICE_LABEL__H
#define __DEVICE_LABEL__H

extern void read_flash_device_label(void);

struct fdl_header {
	char abStartToken[12];  // Fixed String to detect the begin of the device production data 'ProductData>'
	u16 usLabelSize;      // Size of the complete Label incl. this header and the footer
	u16 usContentSize;
};

struct fdl_basic_device_data {
	u16 manufacturer;
	u16 deviceclass;
	u32 devicenumber;
	u32 serialnumber;
	u8  hwcompat;
	u8  hwrevision;
	u16 productiondate;
	u8  reserved[16];
};

struct fdl_mac_address_entry {
	u8 macaddr[6];
	u8 reserved[2];
};

struct fdl_mac_addresses_communication_side {
	struct fdl_mac_address_entry mac[8];
};

struct fdl_mac_addresses_application_side {
	struct fdl_mac_address_entry mac[4];
};

struct fdl_product_identification_information {
	u16 usb_vid;
	u16 usb_pid;
	u8 usb_vendor_name[16];
	u8 usb_product_name[16];
	u8 reserved[76];
};

#define OEM_IDENTIFICATION_FLAG_SERIALNR	0x00000001
#define OEM_IDENTIFICATION_FLAG_ORDERNR		0x00000002
#define OEM_IDENTIFICATION_FLAG_HWREV		0x00000004
#define OEM_IDENTIFICATION_FLAG_PRODDATA	0x00000008

struct fdl_oem_identification {
	u32 oem_option_flags;
	char serialnr[28];
	char ordernr[32];
	char hwrevision[16];
	char production_date[32];
	u8 reserved[12];
	u8 vendor_data[112];
};

struct fdl_flash_layout {
	u8 dummy[488];
};

struct fdl_footer {
	u32 ulCRC;          // CRC-32 (IEEE 802.3) of Content
	char abEndToken[12];  //Fixed String to detect the end of the device production data: '<ProductData'
};

struct fdl_content{
	struct fdl_basic_device_data bdd;
	struct fdl_mac_addresses_communication_side mac_com;
	struct fdl_mac_addresses_application_side  mac_app;
	struct fdl_product_identification_information pii;
	struct fdl_oem_identification oi;
	struct fdl_flash_layout fl;
};

#endif /* __DEVICE_LABEL__H */
