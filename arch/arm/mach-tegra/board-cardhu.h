/*
 * arch/arm/mach-tegra/board-cardhu.h
 *
 * Copyright (c) 2011-2013, NVIDIA Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef _MACH_TEGRA_BOARD_CARDHU_H
#define _MACH_TEGRA_BOARD_CARDHU_H

#include <mach/gpio.h>
#include <mach/irqs.h>
#include <linux/mfd/tps6591x.h>
#include <linux/mfd/ricoh583.h>


/* External peripheral act as gpio */
/* TPS6591x GPIOs */
#define TPS6591X_GPIO_BASE	TEGRA_NR_GPIOS
#define TPS6591X_GPIO_0		(TPS6591X_GPIO_BASE + TPS6591X_GPIO_GP0)
//#define TPS6591X_GPIO_1		(TPS6591X_GPIO_BASE + TPS6591X_GPIO_GP1)
//#define TPS6591X_GPIO_2		(TPS6591X_GPIO_BASE + TPS6591X_GPIO_GP2)
//#define TPS6591X_GPIO_3		(TPS6591X_GPIO_BASE + TPS6591X_GPIO_GP3)
//#define TPS6591X_GPIO_4		(TPS6591X_GPIO_BASE + TPS6591X_GPIO_GP4)
//#define TPS6591X_GPIO_5		(TPS6591X_GPIO_BASE + TPS6591X_GPIO_GP5)
#define TPS6591X_GPIO_6		(TPS6591X_GPIO_BASE + TPS6591X_GPIO_GP6)
#define TPS6591X_GPIO_7		(TPS6591X_GPIO_BASE + TPS6591X_GPIO_GP7)
#define TPS6591X_GPIO_8		(TPS6591X_GPIO_BASE + TPS6591X_GPIO_GP8)
#define TPS6591X_GPIO_END	(TPS6591X_GPIO_BASE + TPS6591X_GPIO_NR)


/* WM8903 GPIOs */
#define CARDHU_GPIO_WM8962(_x_)		(TPS6591X_GPIO_END + (_x_))
#define CARDHU_GPIO_WM8962_END		CARDHU_GPIO_WM8962(6)

/* Audio-related GPIOs */
#define TEGRA_GPIO_CDC_IRQ		TEGRA_GPIO_PW3
#define TEGRA_GPIO_SPKR_EN		-1
#define TEGRA_GPIO_HP_DET		TEGRA_GPIO_PW2
#define TEGRA_GPIO_INT_MIC_EN		TEGRA_GPIO_PK3
#define TEGRA_GPIO_EXT_MIC_EN		TEGRA_GPIO_PK4


/*****************Interrupt tables ******************/
/* External peripheral act as interrupt controller */
/* TPS6591x IRQs */
#define TPS6591X_IRQ_BASE	TEGRA_NR_IRQS
#define TPS6591X_IRQ_END	(TPS6591X_IRQ_BASE + 18)
//#define DOCK_DETECT_GPIO TEGRA_GPIO_PU4


int cardhu_regulator_init(void);
int cardhu_suspend_init(void);
int cardhu_sdhci_init(void);
int cardhu_pinmux_init(void);
int cardhu_gpio_init(void);
int cardhu_panel_init(void);
int cardhu_sensors_init(void);
int cardhu_keys_init(void);
int cardhu_pins_state_init(void);
int cardhu_emc_init(void);
int cardhu_edp_init(void);
int cardhu_pmon_init(void);
void __init surface_rt_i2c_hid_init(void);

/* UART port which is used by bluetooth */
#define BLUETOOTH_UART_DEV_NAME "/dev/ttyHS2"

#define SURFACE_RT_TEMP_ALERT_GPIO		TEGRA_GPIO_PS3

#define TDIODE_OFFSET	(10000)	/* in millicelsius */

enum tegra_bb_type {
	TEGRA_BB_TANGO = 1,
};

#ifdef CONFIG_TEGRA_DC
	extern atomic_t display_ready;
#else
	static __maybe_unused atomic_t display_ready = ATOMIC_INIT(1);
#endif

#endif
