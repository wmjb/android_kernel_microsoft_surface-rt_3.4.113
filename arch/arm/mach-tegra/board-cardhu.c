/*
 * arch/arm/mach-tegra/board-cardhu.c
 *
 * Copyright (c) 2011-2013, NVIDIA CORPORATION.  All rights reserved.
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

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/ctype.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/serial_8250.h>
#include <linux/i2c.h>

#include <linux/dma-mapping.h>
#include <linux/delay.h>
#include <linux/i2c-tegra.h>
#include <linux/gpio.h>
#include <linux/input.h>
#include <linux/platform_data/tegra_usb.h>
#include <linux/spi/spi.h>


#include <linux/tegra_uart.h>
#include <linux/memblock.h>
#include <linux/spi-tegra.h>

#include <linux/rfkill-gpio.h>
#include <linux/of_platform.h>

#include <sound/wm8962.h>

#include <media/tegra_dtv.h>

#include <asm/hardware/gic.h>

#include <mach/edp.h>
#include <mach/clk.h>
#include <mach/iomap.h>
#include <mach/irqs.h>
#include <mach/pinmux.h>
#include <mach/iomap.h>
#include <mach/io_dpd.h>
#include <mach/io.h>
#include <mach/i2s.h>
#include <mach/tegra_asoc_pdata.h>

#include <mach/tegra_wm8962_pdata.h>
#include <mach/usb_phy.h>
#include <mach/pci.h>
#include <mach/gpio-tegra.h>
#include <mach/tegra_fiq_debugger.h>

#include <asm/hardware/gic.h>
#include <asm/mach-types.h>
#include <asm/mach/arch.h>

#include "board.h"
#include "board-common.h"
#include "clock.h"
#include "board-cardhu.h"

#include "devices.h"
#include "gpio-names.h"
#include "fuse.h"
#include "pm.h"

#include "wdt-recovery.h"
#include "common.h"

#define TEGRA_BATTERY_EN TEGRA_GPIO_PN3

#if defined(CONFIG_BT_BLUESLEEP) || defined(CONFIG_BT_BLUESLEEP_MODULE)
static struct rfkill_gpio_platform_data cardhu_bt_rfkill_pdata[] = {
	{
		.name           = "bt_rfkill",
		.shutdown_gpio  =  -1,//TEGRA_GPIO_PU0,
		.reset_gpio     = TEGRA_GPIO_INVALID,
		.type           = RFKILL_TYPE_BLUETOOTH,
	},
};

static struct platform_device cardhu_bt_rfkill_device = {
	.name = "rfkill_gpio",
	.id             = -1,
	.dev = {
		.platform_data = &cardhu_bt_rfkill_pdata,
	},
};

static struct resource cardhu_bluesleep_resources[] = {
	[0] = {
		.name = "gpio_host_wake",
			.start  =  -1,//TEGRA_GPIO_PU6,
			.end    =  -1,//TEGRA_GPIO_PU6,
			.flags  = IORESOURCE_IO,
	},
	[1] = {
		.name = "gpio_ext_wake",
			.start  = -1,// TEGRA_GPIO_PU1,
			.end    =  -1,//TEGRA_GPIO_PU1,
			.flags  = IORESOURCE_IO,
	},
	[2] = {
		.name = "host_wake",
			.flags  = IORESOURCE_IRQ | IORESOURCE_IRQ_HIGHEDGE,
	},
};

static struct platform_device cardhu_bluesleep_device = {
	.name           = "bluesleep",
	.id             = -1,
	.num_resources  = ARRAY_SIZE(cardhu_bluesleep_resources),
	.resource       = cardhu_bluesleep_resources,
};

static noinline void __init cardhu_setup_bluesleep(void)
{
	//cardhu_bluesleep_resources[2].start =
	//	cardhu_bluesleep_resources[2].end =
	//		gpio_to_irq(TEGRA_GPIO_PU6);
	platform_device_register(&cardhu_bluesleep_device);
	return;
}
#elif defined CONFIG_BLUEDROID_PM
static struct resource cardhu_bluedroid_pm_resources[] = {
	[0] = {
		.name   = "shutdown_gpio",
		.start  = -1,//TEGRA_GPIO_PU0,
		.end    =  -1,//TEGRA_GPIO_PU0,
		.flags  = IORESOURCE_IO,
	},
	[1] = {
		.name = "host_wake",
		.flags  = IORESOURCE_IRQ | IORESOURCE_IRQ_HIGHEDGE,
	},
	[2] = {
		.name = "gpio_ext_wake",
		.start  = -1,// TEGRA_GPIO_PU1,
		.end    =  -1,//TEGRA_GPIO_PU1,
		.flags  = IORESOURCE_IO,
	},
	[3] = {
		.name = "gpio_host_wake",
		.start  =  -1,//TEGRA_GPIO_PU6,
		.end    =  -1,//TEGRA_GPIO_PU6,
		.flags  = IORESOURCE_IO,
	},
};

static struct platform_device cardhu_bluedroid_pm_device = {
	.name = "bluedroid_pm",
	.id             = 0,
	.num_resources  = ARRAY_SIZE(cardhu_bluedroid_pm_resources),
	.resource       = cardhu_bluedroid_pm_resources,
};

static noinline void __init cardhu_setup_bluedroid_pm(void)
{
	//cardhu_bluedroid_pm_resources[1].start =
	//	cardhu_bluedroid_pm_resources[1].end =
	//			gpio_to_irq(TEGRA_GPIO_PU6);
	platform_device_register(&cardhu_bluedroid_pm_device);
	return;
}
#endif

static __initdata struct tegra_clk_init_table cardhu_clk_init_table[] = {
	/* name		parent		rate		enabled */
{"pll_m",	NULL,		0,		false},
	{"audio1",	"i2s1_sync",	0,		false},
	{"audio2",	"i2s2_sync",	0,		false},
	{"audio3",	"i2s3_sync",	0,		false},
	{"audio4",	"i2s4_sync",	0,		false},
	{"blink",	"clk_32k",	32768,		true},
	{"d_audio",	"clk_m",	12000000,	false},
	{"dam0",	"clk_m",	12000000,	false},
	{"dam1",	"clk_m",	12000000,	false},
	{"dam2",	"clk_m",	12000000,	false},
	{"hda",		"pll_p",	108000000,	false},
	{"hda2codec_2x","pll_p",	48000000,	false},
	{"i2c1",	"pll_p",	3200000,	false},
	{"i2c2",	"pll_p",	3200000,	false},
	{"i2c3",	"pll_p",	3200000,	false},
	{"i2c4",	"pll_p",	3200000,	false},
	{"i2c5",	"pll_p",	3200000,	false},
	{"i2s0",	"pll_a_out0",	0,		false},
	{"i2s1",	"pll_a_out0",	0,		false},
	{"i2s2",	"pll_a_out0",	0,		false},
	{"i2s3",	"pll_a_out0",	0,		false},
	{"i2s4",	"pll_a_out0",	0,		false},
	{"pll_p",	NULL,		0,		false},
	{"pll_c",       NULL,           0,              false},
	{"pwm",		"pll_p",	5100000,	false},
	{"spdif_out",	"pll_a_out0",	0,		false},
	{NULL,		NULL,		0,		0},


};
static struct tegra_i2c_platform_data cardhu_i2c1_platform_data = {
	.adapter_nr	= 0,
	.bus_count	= 1,
	.bus_clk_rate	= { 100000, 0 },
	.scl_gpio		= {TEGRA_GPIO_PC4, 0},
	.sda_gpio		= {TEGRA_GPIO_PC5, 0},
	.arb_recovery = arb_lost_recovery,
};

static struct tegra_i2c_platform_data cardhu_i2c2_platform_data = {
	.adapter_nr	= 1,
	.bus_count	= 1,
	.bus_clk_rate	= { 100000, 0 },
	.is_clkon_always = true,
	.scl_gpio		= {TEGRA_GPIO_PT5, 0},
	.sda_gpio		= {TEGRA_GPIO_PT6, 0},
	.arb_recovery = arb_lost_recovery,
};

static struct tegra_i2c_platform_data cardhu_i2c3_platform_data = {
	.adapter_nr	= 2,
	.bus_count	= 1,
	.bus_clk_rate	= { 100000, 0 },
	.scl_gpio		= {TEGRA_GPIO_PBB1, 0},
	.sda_gpio		= {TEGRA_GPIO_PBB2, 0},
	.arb_recovery = arb_lost_recovery,
};

static struct tegra_i2c_platform_data cardhu_i2c4_platform_data = {
	.adapter_nr	= 3,
	.bus_count	= 1,
	.bus_clk_rate	= { 10000, 0 },
	.scl_gpio		= {TEGRA_GPIO_PV4, 0},
	.sda_gpio		= {TEGRA_GPIO_PV5, 0},
	.arb_recovery = arb_lost_recovery,
};

static struct tegra_i2c_platform_data cardhu_i2c5_platform_data = {
	.adapter_nr	= 4,
	.bus_count	= 1,
	.bus_clk_rate	= { 400000, 0 },
	.scl_gpio		= {TEGRA_GPIO_PZ6, 0},
	.sda_gpio		= {TEGRA_GPIO_PZ7, 0},
	.arb_recovery = arb_lost_recovery,
};


static struct wm8962_pdata cardhu_codec_wm8962_pdata = {
	.irq_active_low = 0,
	.mic_cfg = 0,
	.gpio_base = CARDHU_GPIO_WM8962(0),
	.gpio_init = {
		/* Needs to be filled for digital Mic's */
	},
};

static struct i2c_board_info __initdata cardhu_codec_wm8962_info = {
	I2C_BOARD_INFO("wm8962", 0x1a),
	.platform_data = &cardhu_codec_wm8962_pdata,
};

static void cardhu_i2c_init(void)
{

	tegra_i2c_device1.dev.platform_data = &cardhu_i2c1_platform_data;
	tegra_i2c_device2.dev.platform_data = &cardhu_i2c2_platform_data;
	tegra_i2c_device3.dev.platform_data = &cardhu_i2c3_platform_data;
	tegra_i2c_device4.dev.platform_data = &cardhu_i2c4_platform_data;
	tegra_i2c_device5.dev.platform_data = &cardhu_i2c5_platform_data;

	platform_device_register(&tegra_i2c_device5);
	platform_device_register(&tegra_i2c_device4);
	platform_device_register(&tegra_i2c_device3);
	platform_device_register(&tegra_i2c_device2);
	platform_device_register(&tegra_i2c_device1);

	cardhu_codec_wm8962_info.irq = gpio_to_irq(TEGRA_GPIO_CDC_IRQ);

	i2c_register_board_info(4, &cardhu_codec_wm8962_info, 1);

}

static struct platform_device *cardhu_uart_devices[] __initdata = {
	&tegra_uarta_device,
	&tegra_uartb_device,
	&tegra_uartc_device,
	&tegra_uartd_device,
	&tegra_uarte_device,
};
static struct uart_clk_parent uart_parent_clk[] = {
	[0] = {.name = "clk_m"},
	[1] = {.name = "pll_p"},
#ifndef CONFIG_TEGRA_PLLM_RESTRICTED
	[2] = {.name = "pll_m"},
#endif
};

static struct tegra_uart_platform_data cardhu_uart_pdata;
static struct tegra_uart_platform_data cardhu_loopback_uart_pdata;

static int __init uart_debug_init(void)
{

	int debug_port_id;
	int default_debug_port = 0;


	default_debug_port = 2;

	debug_port_id = uart_console_debug_init(default_debug_port);
	if (debug_port_id < 0)
		return debug_port_id;


	cardhu_uart_devices[debug_port_id] = uart_console_debug_device;
	return debug_port_id;
}

static void __init cardhu_uart_init(void)
{
	struct clk *c;
	int i;
	struct board_info board_info;

	tegra_get_board_info(&board_info);

	for (i = 0; i < ARRAY_SIZE(uart_parent_clk); ++i) {
		c = tegra_get_clock_by_name(uart_parent_clk[i].name);
		if (IS_ERR_OR_NULL(c)) {
			pr_err("Not able to get the clock for %s\n",
						uart_parent_clk[i].name);
			continue;
		}
		uart_parent_clk[i].parent_clk = c;
		uart_parent_clk[i].fixed_clk_rate = clk_get_rate(c);
	}
	cardhu_uart_pdata.parent_clk_list = uart_parent_clk;
	cardhu_uart_pdata.parent_clk_count = ARRAY_SIZE(uart_parent_clk);
	cardhu_loopback_uart_pdata.parent_clk_list = uart_parent_clk;
	cardhu_loopback_uart_pdata.parent_clk_count =
						ARRAY_SIZE(uart_parent_clk);
	cardhu_loopback_uart_pdata.is_loopback = true;
	tegra_uarta_device.dev.platform_data = &cardhu_uart_pdata;
	tegra_uartb_device.dev.platform_data = &cardhu_uart_pdata;
	tegra_uartc_device.dev.platform_data = &cardhu_uart_pdata;
	tegra_uartd_device.dev.platform_data = &cardhu_uart_pdata;
	/* UARTE is used for loopback test purpose */
	tegra_uarte_device.dev.platform_data = &cardhu_loopback_uart_pdata;

	/* Register low speed only if it is selected */
	if (!is_tegra_debug_uartport_hs())
		uart_debug_init();

	platform_add_devices(cardhu_uart_devices,
				ARRAY_SIZE(cardhu_uart_devices));
}

static struct platform_device *cardhu_spi_devices[] __initdata = {
	&tegra_spi_device4,
};



struct spi_clk_parent spi_parent_clk[] = {
	[0] = {.name = "pll_p"},
#ifndef CONFIG_TEGRA_PLLM_RESTRICTED
	[1] = {.name = "pll_m"},
	[2] = {.name = "clk_m"},
#else
	[1] = {.name = "clk_m"},
#endif
};

static struct tegra_spi_platform_data cardhu_spi_pdata = {
	.is_dma_based		= true,
	.max_dma_buffer		= (16 * 1024),
	.is_clkon_always	= false,
	.max_rate		= 100000000,
};

static void __init cardhu_spi_init(void)
{
	int i;
	struct clk *c;
	struct board_info board_info, display_board_info;

	tegra_get_board_info(&board_info);
	tegra_get_display_board_info(&display_board_info);

	for (i = 0; i < ARRAY_SIZE(spi_parent_clk); ++i) {
		c = tegra_get_clock_by_name(spi_parent_clk[i].name);
		if (IS_ERR_OR_NULL(c)) {
			pr_err("Not able to get the clock for %s\n",
						spi_parent_clk[i].name);
			continue;
		}
		spi_parent_clk[i].parent_clk = c;
		spi_parent_clk[i].fixed_clk_rate = clk_get_rate(c);
	}
	cardhu_spi_pdata.parent_clk_list = spi_parent_clk;
	cardhu_spi_pdata.parent_clk_count = ARRAY_SIZE(spi_parent_clk);
	tegra_spi_device4.dev.platform_data = &cardhu_spi_pdata;
	platform_add_devices(cardhu_spi_devices,
				ARRAY_SIZE(cardhu_spi_devices));

}

static void __init cardhu_dtv_init(void)
{

}

static struct resource tegra_rtc_resources[] = {
	[0] = {
		.start = TEGRA_RTC_BASE,
		.end = TEGRA_RTC_BASE + TEGRA_RTC_SIZE - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = INT_RTC,
		.end = INT_RTC,
		.flags = IORESOURCE_IRQ,
	},
};

static struct platform_device tegra_rtc_device = {
	.name = "tegra_rtc",
	.id   = -1,
	.resource = tegra_rtc_resources,
	.num_resources = ARRAY_SIZE(tegra_rtc_resources),
};


static struct tegra_asoc_platform_data cardhu_audio_wm8962_pdata = {
        .gpio_spkr_en           = TEGRA_GPIO_SPKR_EN,
        .gpio_hp_det            = TEGRA_GPIO_HP_DET,
        .gpio_hp_mute           = -1,
        .gpio_int_mic_en        = -1,
        .gpio_ext_mic_en        = -1,
//      .gpio_bypass_switch_en  = TEGRA_GPIO_BYPASS_SWITCH_EN,
//      .gpio_debug_switch_en   = TEGRA_GPIO_DEBUG_SWITCH_EN,
        .i2s_param[HIFI_CODEC]  = {
                .audio_port_id  = 0,
                .is_i2s_master  = 1,
                .i2s_mode       = TEGRA_DAIFMT_I2S,
        },
//      .i2s_param[BASEBAND]    = {
//              .audio_port_id  = -1,
//      },
//      .i2s_param[BT_SCO]      = {
//              .audio_port_id  = 3,
//              .is_i2s_master  = 1,
//              .i2s_mode       = TEGRA_DAIFMT_DSP_A,
//      },
};

static struct platform_device cardhu_audio_wm8962_device = {
        .name   = "tegra-snd-wm8962",
        .id     = 0,
        .dev    = {
                .platform_data = &cardhu_audio_wm8962_pdata,
        },
};



static struct platform_device *cardhu_devices[] __initdata = {
	&tegra_pmu_device,
	&tegra_rtc_device,
	&tegra_udc_device,
	&tegra_wdt0_device,
#if defined(CONFIG_TEGRA_AVP)
	&tegra_avp_device,
#endif
#if defined(CONFIG_CRYPTO_DEV_TEGRA_SE)
	&tegra_se_device,
#endif
	&tegra_ahub_device,
	&tegra_dam_device0,
	&tegra_dam_device1,
	&tegra_dam_device2,
	&tegra_i2s_device0,
	&tegra_i2s_device1,
	&tegra_i2s_device2,
	&tegra_i2s_device3,
	&tegra_spdif_device,
	&spdif_dit_device,
	&bluetooth_dit_device,
	&baseband_dit_device,
#if defined(CONFIG_BT_BLUESLEEP) || defined(CONFIG_BT_BLUESLEEP_MODULE)
	&cardhu_bt_rfkill_device,
#endif
	&tegra_pcm_device,

	&tegra_hda_device,
	&cardhu_audio_wm8962_device,
	&tegra_cec_device,
#if defined(CONFIG_CRYPTO_DEV_TEGRA_AES)
	&tegra_aes_device,
#endif
};


#if defined(CONFIG_USB_SUPPORT)
//static struct tegra_usb_phy_platform_ops hsic_plat_ops = {
//	.open = hsic_platform_open,
//	.close = hsic_platform_close,
//	.pre_phy_on = hsic_power_on,
//	.post_phy_off = hsic_power_off,
//};

//static struct tegra_usb_platform_data tegra_ehci2_hsic_pdata = {
//	.port_otg = false,
//	.has_hostpc = true,
//	.phy_intf = TEGRA_USB_PHY_INTF_HSIC,
//	.op_mode	= TEGRA_USB_OPMODE_HOST,
//	.u_data.host = {
//		.vbus_gpio = -1,
//		.hot_plug = false,
//		.remote_wakeup_supported = false,
//		.power_off_on_suspend = false,
//	},
//	.ops = &hsic_plat_ops,
//};

static struct tegra_usb_platform_data tegra_udc_pdata = {
	.port_otg = true,
	.has_hostpc = true,
	.phy_intf = TEGRA_USB_PHY_INTF_UTMI,
	.op_mode = TEGRA_USB_OPMODE_DEVICE,
	.u_data.dev = {
		.vbus_pmu_irq = 0,
		.vbus_gpio = -1,
		.charging_supported = false,
		.remote_wakeup_supported = false,
	},
	.u_cfg.utmi = {
		.hssync_start_delay = 0,
		.elastic_limit = 16,
		.idle_wait_delay = 17,
		.term_range_adj = 6,
		.xcvr_setup = 8,
		.xcvr_lsfslew = 2,
		.xcvr_lsrslew = 2,
		.xcvr_setup_offset = 0,
		.xcvr_use_fuses = 1,
	},
};

//static struct tegra_usb_platform_data tegra_ehci3_utmi_pdata = {
//	.port_otg = false,
//	.has_hostpc = true,
//	.phy_intf = TEGRA_USB_PHY_INTF_UTMI,
//	.op_mode = TEGRA_USB_OPMODE_HOST,
//	.u_data.host = {
//		.vbus_gpio = -1,
//		.hot_plug = true,
//		.remote_wakeup_supported = true,
//		.power_off_on_suspend = true,
//	},
//	.u_cfg.utmi = {
//		.hssync_start_delay = 0,
//		.elastic_limit = 16,
//		.idle_wait_delay = 17,
//		.term_range_adj = 6,
//		.xcvr_setup = 8,
//		.xcvr_lsfslew = 2,
//		.xcvr_lsrslew = 2,
//		.xcvr_setup_offset = 0,
//		.xcvr_use_fuses = 1,
//	},
//};

static struct tegra_usb_platform_data tegra_ehci1_utmi_pdata = {
	.port_otg = true,
	.has_hostpc = true,
	.phy_intf = TEGRA_USB_PHY_INTF_UTMI,
	.op_mode = TEGRA_USB_OPMODE_HOST,
	.u_data.host = {
		.vbus_gpio = -1,
		.hot_plug = true,
		.remote_wakeup_supported = true,
		.power_off_on_suspend = true,
	},
	.u_cfg.utmi = {
		.hssync_start_delay = 0,
		.elastic_limit = 16,
		.idle_wait_delay = 17,
		.term_range_adj = 6,
		.xcvr_setup = 15,
		.xcvr_lsfslew = 2,
		.xcvr_lsrslew = 2,
		.xcvr_setup_offset = 0,
		.xcvr_use_fuses = 1,
	},
};

static struct tegra_usb_otg_data tegra_otg_pdata = {
	.ehci_device = &tegra_ehci1_device,
	.ehci_pdata = &tegra_ehci1_utmi_pdata,
};
#endif

#if defined(CONFIG_USB_SUPPORT)
static void cardhu_usb_init(void)
{
	struct board_info bi;

	tegra_get_board_info(&bi);

	/* OTG should be the first to be registered */
	tegra_otg_device.dev.platform_data = &tegra_otg_pdata;
	platform_device_register(&tegra_otg_device);

	/* setup the udc platform data */
	tegra_udc_device.dev.platform_data = &tegra_udc_pdata;

//	hsic_enable_gpio = EN_HSIC_GPIO;
//	hsic_reset_gpio = PM267_SMSC4640_HSIC_HUB_RESET_GPIO;
//	tegra_ehci2_device.dev.platform_data = &tegra_ehci2_hsic_pdata;
//	platform_device_register(&tegra_ehci2_device);

//	tegra_ehci3_device.dev.platform_data = &tegra_ehci3_utmi_pdata;
//	platform_device_register(&tegra_ehci3_device);

}
#else
static void cardhu_usb_init(void) { }
#endif


static void surface_rt_ec_init(void)

{
	int ret;
	ret = gpio_request(TEGRA_BATTERY_EN, "ec_gpio");
	if (ret)
	{
		pr_err("EC gpio request failed:%d\n", ret);
		return ;
	}
	else
	{
		gpio_direction_output(TEGRA_BATTERY_EN, 1);
		mdelay(100);
	        gpio_set_value(TEGRA_BATTERY_EN, 1);
		mdelay(100);
		ret = gpio_get_value(TEGRA_BATTERY_EN);

		printk(KERN_INFO "Set Surface EC GPIO : %d\n",ret);
	}
}

static void __init tegra_cardhu_init(void)
{
	tegra_clk_init_from_table(cardhu_clk_init_table);
	tegra_enable_pinmux();
	tegra_smmu_init();
	tegra_soc_device_init("cardhu");
	cardhu_pinmux_init();
	//cardhu_gpio_init();
	cardhu_i2c_init();
	cardhu_spi_init();
	cardhu_usb_init();
#ifdef CONFIG_TEGRA_EDP_LIMITS
	cardhu_edp_init();
#endif
	cardhu_uart_init();
	platform_add_devices(cardhu_devices, ARRAY_SIZE(cardhu_devices));
	//platform_add_devices(cardhu_audio_devices, ARRAY_SIZE(cardhu_audio_devices));
	tegra_ram_console_debug_init();
	tegra_io_dpd_init();
	cardhu_sdhci_init();
	cardhu_regulator_init();
	cardhu_dtv_init();
	cardhu_suspend_init();

	cardhu_keys_init();
	cardhu_panel_init();
//	cardhu_pmon_init();
	surface_rt_ec_init();
	//cardhu_sensors_init();
#if defined(CONFIG_BT_BLUESLEEP) || defined(CONFIG_BT_BLUESLEEP_MODULE)
	cardhu_setup_bluesleep();
#elif defined CONFIG_BLUEDROID_PM
	cardhu_setup_bluedroid_pm();
#endif
	cardhu_pins_state_init();
	cardhu_emc_init();

#ifdef CONFIG_TEGRA_WDT_RECOVERY
	tegra_wdt_recovery_init();
#endif
	tegra_serial_debug_init(TEGRA_UARTD_BASE, INT_WDT_CPU, NULL, -1, -1);
	//tegra_vibrator_init();
	tegra_register_fuse();
	surface_rt_i2c_hid_init();
}

static void __init tegra_cardhu_dt_init(void)
{
	tegra_cardhu_init();

#ifdef CONFIG_USE_OF
	of_platform_populate(NULL,
		of_default_bus_match_table, NULL, NULL);
#endif
}

static void __init tegra_cardhu_reserve(void)
{
#if defined(CONFIG_NVMAP_CONVERT_CARVEOUT_TO_IOVMM)
	/* support 1920X1200 with 24bpp */
	tegra_reserve(0, SZ_8M + SZ_1M, SZ_16M);
#else
	tegra_reserve(SZ_128M, SZ_8M, SZ_16M);
#endif
}

static const char *cardhu_dt_board_compat[] = {
	"nvidia,cardhu",
	NULL
};

MACHINE_START(CARDHU, "cardhu")
	.atag_offset    = 0x100,
	.soc		= &tegra_soc_desc,
	.map_io         = tegra_map_common_io,
	.reserve        = tegra_cardhu_reserve,
	.init_early	= tegra30_init_early,
	.init_irq       = tegra_init_irq,
	.handle_irq	= gic_handle_irq,
	.timer          = &tegra_timer,
	.init_machine   = tegra_cardhu_dt_init,
	.restart	= tegra_assert_system_reset,
	.dt_compat	= cardhu_dt_board_compat,
MACHINE_END
