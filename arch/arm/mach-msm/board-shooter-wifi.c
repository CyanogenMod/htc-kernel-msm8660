/* arch/arm/mach-msm/board-shooter-wifi.c
 *
 * Copyright (C) 2010-2011 HTC Corporation.
 * Copyright (C) 2012 The CyanogenMod Project.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/android_pmem.h>
#include <linux/mfd/pmic8058.h>
#include <linux/mfd/marimba.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <asm/mach-types.h>
#include <asm/gpio.h>
#include <asm/io.h>
#include <linux/skbuff.h>
#include <linux/wlan_plat.h>

#include "board-shooter.h"

int shooter_wifi_power(int on);
int shooter_wifi_reset(int on);
int shooter_wifi_set_carddetect(int on);
int shooter_wifi_get_mac_addr(unsigned char *buf);

#define PREALLOC_WLAN_NUMBER_OF_SECTIONS	4
#define PREALLOC_WLAN_NUMBER_OF_BUFFERS		160
#define PREALLOC_WLAN_SECTION_HEADER		24

#define WLAN_SECTION_SIZE_0	(PREALLOC_WLAN_NUMBER_OF_BUFFERS * 128)
#define WLAN_SECTION_SIZE_1	(PREALLOC_WLAN_NUMBER_OF_BUFFERS * 128)
#define WLAN_SECTION_SIZE_2	(PREALLOC_WLAN_NUMBER_OF_BUFFERS * 512)
#define WLAN_SECTION_SIZE_3	(PREALLOC_WLAN_NUMBER_OF_BUFFERS * 1024)

#define WLAN_SKB_BUF_NUM	16

#define HW_OOB 1

static struct sk_buff *wlan_static_skb[WLAN_SKB_BUF_NUM];

typedef struct wifi_mem_prealloc_struct {
	void *mem_ptr;
	unsigned long size;
} wifi_mem_prealloc_t;

static wifi_mem_prealloc_t wifi_mem_array[PREALLOC_WLAN_NUMBER_OF_SECTIONS] = {
	{ NULL, (WLAN_SECTION_SIZE_0 + PREALLOC_WLAN_SECTION_HEADER) },
	{ NULL, (WLAN_SECTION_SIZE_1 + PREALLOC_WLAN_SECTION_HEADER) },
	{ NULL, (WLAN_SECTION_SIZE_2 + PREALLOC_WLAN_SECTION_HEADER) },
	{ NULL, (WLAN_SECTION_SIZE_3 + PREALLOC_WLAN_SECTION_HEADER) }
};

static void *shooter_wifi_mem_prealloc(int section, unsigned long size)
{
	if (section == PREALLOC_WLAN_NUMBER_OF_SECTIONS)
		return wlan_static_skb;
	if ((section < 0) || (section > PREALLOC_WLAN_NUMBER_OF_SECTIONS))
		return NULL;
	if (wifi_mem_array[section].size < size)
		return NULL;
	return wifi_mem_array[section].mem_ptr;
}

int __init shooter_init_wifi_mem(void)
{
	int i;

	for (i = 0; (i < WLAN_SKB_BUF_NUM); i++) {
		if (i < (WLAN_SKB_BUF_NUM/2))
			wlan_static_skb[i] = dev_alloc_skb(4096);
		else
			wlan_static_skb[i] = dev_alloc_skb(8192);
	}
	for (i = 0; (i < PREALLOC_WLAN_NUMBER_OF_SECTIONS); i++) {
		wifi_mem_array[i].mem_ptr = kmalloc(wifi_mem_array[i].size,
							GFP_KERNEL);
		if (wifi_mem_array[i].mem_ptr == NULL)
			return -ENOMEM;
	}
	return 0;
}

static struct resource shooter_wifi_resources[] = {
	[0] = {
		.name	= "bcm4329_wlan_irq",
		.start	= MSM_GPIO_TO_INT(SHOOTER_GPIO_WIFI_IRQ),
		.end	= MSM_GPIO_TO_INT(SHOOTER_GPIO_WIFI_IRQ),
		.flags	= IORESOURCE_IRQ | IORESOURCE_IRQ_HIGHLEVEL | IORESOURCE_IRQ_SHAREABLE,
	},
};

static struct wifi_platform_data shooter_wifi_control = {
	.set_power	= shooter_wifi_power,
	.set_reset	= shooter_wifi_reset,
	.set_carddetect	= shooter_wifi_set_carddetect,
	.mem_prealloc	= shooter_wifi_mem_prealloc,
	.get_mac_addr	= shooter_wifi_get_mac_addr,
};

static struct platform_device shooter_wifi_device = {
	.name		= "bcm4329_wlan",
	.id		= 1,
	.num_resources	= ARRAY_SIZE(shooter_wifi_resources),
	.resource	= shooter_wifi_resources,
	.dev		= {
		.platform_data = &shooter_wifi_control,
	},
};

extern unsigned char *get_wifi_nvs_ram(void);

static unsigned shooter_wifi_update_nvs(char *str)
{
#define NVS_LEN_OFFSET		0x0C
#define NVS_DATA_OFFSET		0x40
	unsigned char *ptr;
	unsigned len;

	if (!str)
		return -EINVAL;
	ptr = get_wifi_nvs_ram();
	/* Size in format LE assumed */
	memcpy(&len, ptr + NVS_LEN_OFFSET, sizeof(len));

	/* the last bye in NVRAM is 0, trim it */
	if (ptr[NVS_DATA_OFFSET + len - 1] == 0)
		len -= 1;

	if (ptr[NVS_DATA_OFFSET + len -1] != '\n') {
		len += 1;
		ptr[NVS_DATA_OFFSET + len -1] = '\n';
	}

	strcpy(ptr + NVS_DATA_OFFSET + len, str);
	len += strlen(str);
	memcpy(ptr + NVS_LEN_OFFSET, &len, sizeof(len));
	return 0;
}

#ifdef HW_OOB
static unsigned strip_nvs_param(char* param)
{
	unsigned char *nvs_data;

	unsigned param_len;
	int start_idx, end_idx;

	unsigned char *ptr;
	unsigned len;

	if (!param)
		return -EINVAL;
	ptr = get_wifi_nvs_ram();
	/* Size in format LE assumed */
	memcpy(&len, ptr + NVS_LEN_OFFSET, sizeof(len));

	/* the last bye in NVRAM is 0, trim it */
	if (ptr[NVS_DATA_OFFSET + len -1] == 0)
		len -= 1;

	nvs_data = ptr + NVS_DATA_OFFSET;

	param_len = strlen(param);

	/* search param */
	for (start_idx = 0; start_idx < len - param_len; start_idx++) {
		if (memcmp(&nvs_data[start_idx], param, param_len) == 0) {
			break;
		}
	}

	end_idx = 0;
	if (start_idx < len - param_len) {
		/* search end-of-line */
		for (end_idx = start_idx + param_len; end_idx < len; end_idx++) {
			if (nvs_data[end_idx] == '\n' || nvs_data[end_idx] == 0) {
				break;
			}
		}
	}

	if (start_idx < end_idx) {
		/* move the remain data forward */
		for (; end_idx + 1 < len; start_idx++, end_idx++) {
			nvs_data[start_idx] = nvs_data[end_idx+1];
		}
		len = len - (end_idx - start_idx + 1);
		memcpy(ptr + NVS_LEN_OFFSET, &len, sizeof(len));
	}
	return 0;
}
#endif

#define WIFI_MAC_PARAM_STR     "macaddr="
#define WIFI_MAX_MAC_LEN       17 /* XX:XX:XX:XX:XX:XX */

static uint
get_mac_from_wifi_nvs_ram(char* buf, unsigned int buf_len)
{
	unsigned char *nvs_ptr;
	unsigned char *mac_ptr;
	uint len = 0;

	if (!buf || !buf_len) {
		return 0;
	}

	nvs_ptr = get_wifi_nvs_ram();
	if (nvs_ptr) {
		nvs_ptr += NVS_DATA_OFFSET;
	}

	mac_ptr = strstr(nvs_ptr, WIFI_MAC_PARAM_STR);
	if (mac_ptr) {
		mac_ptr += strlen(WIFI_MAC_PARAM_STR);

		/* skip leading space */
		while (mac_ptr[0] == ' ') {
			mac_ptr++;
		}

		/* locate end-of-line */
		len = 0;
		while (mac_ptr[len] != '\r' && mac_ptr[len] != '\n' &&
			mac_ptr[len] != '\0') {
			len++;
		}

		if (len > buf_len) {
			len = buf_len;
		}
		memcpy(buf, mac_ptr, len);
	}

	return len;
}

#define ETHER_ADDR_LEN 6
int shooter_wifi_get_mac_addr(unsigned char *buf)
{
	static u8 ether_mac_addr[] = {0x00, 0x11, 0x22, 0x33, 0x44, 0xFF};
	char mac[WIFI_MAX_MAC_LEN];
	unsigned mac_len;
	unsigned int macpattern[ETHER_ADDR_LEN];
	int i;

	mac_len = get_mac_from_wifi_nvs_ram(mac, WIFI_MAX_MAC_LEN);
	if (mac_len > 0) {
		//Mac address to pattern
		sscanf( mac, "%02x:%02x:%02x:%02x:%02x:%02x",
		&macpattern[0], &macpattern[1], &macpattern[2],
		&macpattern[3], &macpattern[4], &macpattern[5]
		);

		for(i = 0; i < ETHER_ADDR_LEN; i++) {
			ether_mac_addr[i] = (u8)macpattern[i];
		}
	}

	memcpy(buf, ether_mac_addr, sizeof(ether_mac_addr));

	printk("shooter_wifi_get_mac_addr = %02x %02x %02x %02x %02x %02x \n",
		ether_mac_addr[0],ether_mac_addr[1],ether_mac_addr[2],ether_mac_addr[3],ether_mac_addr[4],ether_mac_addr[5]);

	return 0;
}

int __init shooter_wifi_init(void)
{
	int ret;

	printk(KERN_INFO "%s: start\n", __func__);
#ifdef HW_OOB
	strip_nvs_param("sd_oobonly");
#else
	shooter_wifi_update_nvs("sd_oobonly=1\n");
#endif
	shooter_wifi_update_nvs("btc_params80=0\n");
	shooter_wifi_update_nvs("btc_params6=30\n");
	shooter_init_wifi_mem();
	ret = platform_device_register(&shooter_wifi_device);
	return ret;
}
