/* arch/arm/mach-msm/board-shooter-keypad.c
 *
 * Copyright (C) 2008 Google, Inc.
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

#include <linux/platform_device.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/gpio_event.h>
#include <linux/keyreset.h>
#include <asm/mach-types.h>
#include <linux/gpio.h>
#include <mach/gpio.h>

#include "board-shooter.h"

#include <linux/mfd/pmic8058.h>
#include <linux/input/pmic8xxx-keypad.h>

static char *keycaps = "--qwerty";
#undef MODULE_PARAM_PREFIX
#ifdef CONFIG_MACH_SHOOTER
#define MODULE_PARAM_PREFIX "board_shooter."
#else
#define MODULE_PARAM_PREFIX "board_shooter_u."
#endif
module_param_named(keycaps, keycaps, charp, 0);

static struct gpio_event_direct_entry shooter_keypad_switch_map[] = {
	{ SHOOTER_GPIO_KEY_POWER, 	KEY_POWER		},
	{ SHOOTER_GPIO_KEY_VOL_UP,	KEY_VOLUMEUP		},
	{ SHOOTER_GPIO_KEY_VOL_DOWN,	KEY_VOLUMEDOWN		},
	{ SHOOTER_GPIO_KEY_CAM_STEP1,	KEY_HP			},
	{ SHOOTER_GPIO_KEY_CAM_STEP2,	KEY_CAMERA		},
};

static void shooter_gpio_event_input_init(void)
{
	gpio_tlmm_config(GPIO_CFG(SHOOTER_GPIO_KEY_POWER, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP,
				GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	gpio_tlmm_config(GPIO_CFG(SHOOTER_GPIO_KEY_VOL_UP, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP,
				GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	gpio_tlmm_config(GPIO_CFG(SHOOTER_GPIO_KEY_VOL_DOWN, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP,
				GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	gpio_tlmm_config(GPIO_CFG(SHOOTER_GPIO_KEY_CAM_STEP1, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP,
				GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	gpio_tlmm_config(GPIO_CFG(SHOOTER_GPIO_KEY_CAM_STEP2, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP,
				GPIO_CFG_2MA), GPIO_CFG_ENABLE);

	enable_irq_wake(MSM_GPIO_TO_INT(SHOOTER_GPIO_KEY_VOL_UP));
	enable_irq_wake(MSM_GPIO_TO_INT(SHOOTER_GPIO_KEY_VOL_DOWN));
	enable_irq_wake(MSM_GPIO_TO_INT(SHOOTER_GPIO_KEY_POWER));
};

static struct gpio_event_input_info shooter_keypad_switch_info = {
	.info.func = gpio_event_input_func,
	.info.no_suspend = true,
	.flags = 0,
	.type = EV_KEY,
	.keymap = shooter_keypad_switch_map,
	.keymap_size = ARRAY_SIZE(shooter_keypad_switch_map)
};

static struct gpio_event_info *shooter_keypad_info[] = {
	&shooter_keypad_switch_info.info,
};

static int shooter_gpio_keypad_power(
		const struct gpio_event_platform_data *pdata, bool on)
{
	return 0;
};

static struct gpio_event_platform_data shooter_keypad_data = {
	.name = "gpio-side-keypad",
	.info = shooter_keypad_info,
	.info_count = ARRAY_SIZE(shooter_keypad_info),
	.power = shooter_gpio_keypad_power,
};

static struct platform_device shooter_gpio_keypad_device = {
	.name = GPIO_EVENT_DEV_NAME,
	.id = 1,
	.dev        = {
		.platform_data  = &shooter_keypad_data,
	},
};

static struct platform_device *shooter_input_devices[] __initdata = {
	&shooter_gpio_keypad_device,
};

void __init shooter_add_input_devices(void)
{
	shooter_gpio_event_input_init();
	platform_add_devices(shooter_input_devices, ARRAY_SIZE(shooter_input_devices));
};
