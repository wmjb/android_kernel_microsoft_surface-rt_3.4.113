/*
 * arch/arm/mach-tegra/board-cardhu-kbc.c
 * Keys configuration for Nvidia tegra3 cardhu platform.
 *
 * Copyright (C) 2011-2013 NVIDIA, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 * 02111-1307, USA
 */

#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/input.h>
#include <linux/device.h>
#include <linux/gpio.h>
#include <linux/gpio_keys.h>
#include <linux/mfd/tps6591x.h>
#include <linux/mfd/max77663-core.h>
#include <linux/gpio_scrollwheel.h>

#include <mach/irqs.h>
#include <mach/io.h>
#include <mach/iomap.h>
#include <mach/kbc.h>
#include <mach/gpio-tegra.h>

#include "board.h"
#include "board-cardhu.h"

#include "gpio-names.h"
#include "devices.h"


#define GPIO_KEY(_id, _gpio, _iswake)		\
	{					\
		.code = _id,			\
		.gpio = TEGRA_GPIO_##_gpio,	\
		.active_low = 1,		\
		.desc = #_id,			\
		.type = EV_KEY,			\
		.wakeup = _iswake,		\
		.debounce_interval = 10,	\
	}

#define GPIO_SW_KEY(_id, _gpio, _iswake)	\
	{					\
		.code = _id,			\
		.gpio = _gpio,			\
		.active_low = 1,		\
		.desc = #_id,			\
		.type = EV_SW,			\
		.wakeup = _iswake,		\
		.debounce_interval = 1,		\
	}

#define GPIO_IKEY(_id, _irq, _iswake, _deb)	\
	{					\
		.code = _id,			\
		.gpio = -1,			\
		.irq = _irq,			\
		.desc = #_id,			\
		.type = EV_KEY,			\
		.wakeup = _iswake,		\
		.debounce_interval = _deb,	\
	}

static struct gpio_keys_button cardhu_int_keys[] = {
	[0] = GPIO_KEY(KEY_POWER, PV0, 1),
	[1] = GPIO_KEY(KEY_VOLUMEUP, PS7, 0),
	[2] = GPIO_KEY(KEY_VOLUMEDOWN, PS6, 0),
	[3] = GPIO_KEY(KEY_HOME, PS5, 1),
};

static struct gpio_keys_platform_data cardhu_int_keys_keys_platform_data = {
	.buttons	= cardhu_int_keys,
	.nbuttons	= ARRAY_SIZE(cardhu_int_keys),
};

static struct platform_device cardhu_int_keys_device = {
	.name	= "gpio-keys",
	.id		= 0,
	.dev	= {
		.platform_data  = &cardhu_int_keys_keys_platform_data,
	},
};

int __init cardhu_keys_init(void)
{

		platform_device_register(&cardhu_int_keys_device);
	
	return 0;
}
