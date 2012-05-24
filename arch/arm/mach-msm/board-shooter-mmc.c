/* arch/arm/mach-msm/board-shooter-mmc.c
 *
 * Copyright (C) 2008 HTC Corporation.
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
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/mmc/host.h>
#include <linux/mmc/sdio_ids.h>
#include <linux/err.h>
#include <linux/debugfs.h>
#include <linux/gpio.h>

#include <asm/gpio.h>
#include <asm/io.h>

#include <mach/vreg.h>
#include <mach/htc_pwrsink.h>

#include <asm/mach/mmc.h>

#include "devices.h"
#include "board-shooter.h"
#include "proc_comm.h"
#include <mach/msm_iomap.h>
#include <linux/mfd/pmic8058.h>
#include "mpm.h"
#include <linux/irq.h>

int msm_proc_comm(unsigned cmd, unsigned *data1, unsigned *data2);

extern int msm_add_sdcc(unsigned int controller, struct mmc_platform_data *plat);

static uint32_t wifi_on_gpio_table[] = {
	GPIO_CFG(SHOOTER_GPIO_WIFI_IRQ, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_4MA), /* WLAN IRQ */
};

static uint32_t wifi_off_gpio_table[] = {
	GPIO_CFG(SHOOTER_GPIO_WIFI_IRQ, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_4MA), /* WLAN IRQ */
};

static void config_gpio_table(uint32_t *table, int len)
{
	int n, rc;
	for (n = 0; n < len; n++) {
		rc = gpio_tlmm_config(table[n], GPIO_CFG_ENABLE);
		if (rc) {
			pr_err("%s: gpio_tlmm_config(%#x)=%d\n",
				__func__, table[n], rc);
			break;
		}
	}
}

/* BCM4329 returns wrong sdio_vsn(1) when we read cccr,
   we use predefined value (sdio_vsn=2) here to initial sdio driver well */
static struct embedded_sdio_data shooter_wifi_emb_data = {
	.cccr	= {
		.sdio_vsn	= 2,
		.multi_block	= 1,
		.low_speed	= 0,
		.wide_bus	= 0,
		.high_power	= 1,
		.high_speed	= 1,
	}
};

static void (*wifi_status_cb)(int card_present, void *dev_id);
static void *wifi_status_cb_devid;

static int
shooter_wifi_status_register(void (*callback)(int card_present, void *dev_id),
				void *dev_id)
{
	if (wifi_status_cb)
		return -EAGAIN;

	wifi_status_cb = callback;
	wifi_status_cb_devid = dev_id;
	return 0;
}

static int shooter_wifi_cd;	/* WiFi virtual 'card detect' status */

static unsigned int shooter_wifi_status(struct device *dev)
{
	return shooter_wifi_cd;
}

static unsigned int shooter_wifislot_type = MMC_TYPE_SDIO_WIFI;
static struct mmc_platform_data shooter_wifi_data = {
	.ocr_mask	= MMC_VDD_28_29,
	.status		= shooter_wifi_status,
	.register_status_notify = shooter_wifi_status_register,
	.embedded_sdio	= &shooter_wifi_emb_data,
	.mmc_bus_width	= MMC_CAP_4_BIT_DATA,
	.slot_type	= &shooter_wifislot_type,
	.msmsdcc_fmin	= 400000,
	.msmsdcc_fmid	= 24000000,
	.msmsdcc_fmax	= 48000000,
	.nonremovable	= 0,
	.pclk_src_dfab	= 1,
};

int shooter_wifi_set_carddetect(int val)
{
	printk(KERN_INFO "%s: %d\n", __func__, val);
	shooter_wifi_cd = val;
	if (wifi_status_cb)
		wifi_status_cb(val, wifi_status_cb_devid);
	else
		printk(KERN_WARNING "%s: Nobody to notify\n", __func__);
	return 0;
}
EXPORT_SYMBOL(shooter_wifi_set_carddetect);

int shooter_wifi_power(int on)
{
	const unsigned SDC4_HDRV_PULL_CTL_ADDR = (unsigned) MSM_TLMM_BASE + 0x20A0;

	printk(KERN_INFO "%s: %d\n", __func__, on);

	if (on) {
		//SDC4_CMD_PULL = Pull Up, SDC4_DATA_PULL = Pull up
		writel(0x1FDB, SDC4_HDRV_PULL_CTL_ADDR);
		config_gpio_table(wifi_on_gpio_table,
				  ARRAY_SIZE(wifi_on_gpio_table));
	} else {
		//SDC4_CMD_PULL = Pull Down, SDC4_DATA_PULL = Pull Down
		writel(0x0BDB, SDC4_HDRV_PULL_CTL_ADDR);
		config_gpio_table(wifi_off_gpio_table,
				  ARRAY_SIZE(wifi_off_gpio_table));
	}
	mdelay(1); //Delay 1 ms, Recommand by Hardware
	gpio_set_value(SHOOTER_GPIO_WIFI_SHUTDOWN_N, on); /* WIFI_SHUTDOWN */

	mdelay(120);
	return 0;
}
EXPORT_SYMBOL(shooter_wifi_power);

int shooter_wifi_reset(int on)
{
	printk(KERN_INFO "%s: do nothing\n", __func__);
	return 0;
}

void __init shooter_init_mmc()
{
	uint32_t id;
	wifi_status_cb = NULL;

	printk(KERN_INFO "shooter: %s\n", __func__);

	/* initial WIFI_SHUTDOWN# */
	id = GPIO_CFG(SHOOTER_GPIO_WIFI_SHUTDOWN_N, 0, GPIO_CFG_OUTPUT,
		GPIO_CFG_NO_PULL, GPIO_CFG_2MA);
	gpio_tlmm_config(id, 0);
	gpio_set_value(SHOOTER_GPIO_WIFI_SHUTDOWN_N, 0);

	msm_add_sdcc(4, &shooter_wifi_data);
}
