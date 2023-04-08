/*
 * arch/arm/mach-tegra/board-cardhu-panel.c
 *
 * Copyright (c) 2010-2013, NVIDIA Corporation. All rights reserved.
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

#include <linux/delay.h>
#include <linux/ion.h>
#include <linux/tegra_ion.h>
#include <linux/gpio.h>
#include <linux/regulator/consumer.h>
#include <linux/resource.h>
#include <asm/mach-types.h>
#include <linux/platform_device.h>
#include <linux/pwm_backlight.h>
#include <asm/atomic.h>
#include <linux/nvhost.h>
#include <linux/nvmap.h>
#include <mach/irqs.h>
#include <mach/iomap.h>
#include <mach/dc.h>
#include <mach/fb.h>
#include <mach/gpio-tegra.h>

#include "board.h"
#include "clock.h"
#include "dvfs.h"
#include "board-cardhu.h"
#include "devices.h"
#include "gpio-names.h"
#include "tegra3_host1x_devices.h"

#define DC_CTRL_MODE	(TEGRA_DC_OUT_ONE_SHOT_MODE | \
			 TEGRA_DC_OUT_ONE_SHOT_LP_MODE)

/* E1247 cardhu default display board pins */
#define surface_rt_lvds_shutdown	TEGRA_GPIO_PB2

/* common pins( backlight ) for all display boards */
//#define cardhu_bl_enb			TEGRA_GPIO_PH2
//#define cardhu_bl_pwm			TEGRA_GPIO_PH0
//#define cardhu_hdmi_hpd		TEGRA_GPIO_PN7

#ifdef CONFIG_TEGRA_DC
//static struct regulator *cardhu_hdmi_reg = NULL;
//static struct regulator *cardhu_hdmi_pll = NULL;
//static struct regulator *cardhu_hdmi_vddio = NULL;
#endif

static atomic_t sd_brightness = ATOMIC_INIT(255);


//static struct regulator *cardhu_lvds_reg = NULL;
static struct regulator *cardhu_lvds_vdd_bl = NULL;
static struct regulator *cardhu_lvds_vdd_panel = NULL;

static tegra_dc_bl_output surface_rt_bl_output_measured = {
	0, 4, 4, 4, 4, 5, 6, 7,
	8, 9, 10, 11, 12, 13, 14, 15,
	16, 17, 18, 19, 20, 21, 22, 23,
	24, 25, 26, 27, 28, 29, 30, 31,
	32, 33, 34, 35, 36, 37, 38, 39,
	40, 41, 42, 43, 44, 45, 46, 47,
	48, 49, 49, 50, 51, 52, 53, 54,
	55, 56, 57, 58, 59, 60, 61, 62,
	63, 64, 65, 66, 67, 68, 69, 70,
	70, 72, 73, 74, 75, 76, 77, 78,
	79, 80, 81, 82, 83, 84, 85, 86,
	87, 88, 89, 90, 91, 92, 93, 94,
	95, 96, 97, 98, 99, 100, 101, 102,
	103, 104, 105, 106, 107, 108, 110, 111,
	112, 113, 114, 115, 116, 117, 118, 119,
	120, 121, 122, 123, 124, 124, 125, 126,
	127, 128, 129, 130, 131, 132, 133, 133,
	134, 135, 136, 137, 138, 139, 140, 141,
	142, 143, 144, 145, 146, 147, 148, 148,
	149, 150, 151, 152, 153, 154, 155, 156,
	157, 158, 159, 160, 161, 162, 163, 164,
	165, 166, 167, 168, 169, 170, 171, 172,
	173, 174, 175, 176, 177, 179, 180, 181,
	182, 184, 185, 186, 187, 188, 189, 190,
	191, 192, 193, 194, 195, 196, 197, 198,
	199, 200, 201, 202, 203, 204, 205, 206,
	207, 208, 209, 211, 212, 213, 214, 215,
	216, 217, 218, 219, 220, 221, 222, 223,
	224, 225, 226, 227, 228, 229, 230, 231,
	232, 233, 234, 235, 236, 237, 238, 239,
	240, 241, 242, 243, 244, 245, 246, 247,
	248, 249, 250, 251, 252, 253, 254, 255
};

static p_tegra_dc_bl_output bl_output = surface_rt_bl_output_measured;

static int cardhu_backlight_init(struct device *dev)
{

	bl_output = surface_rt_bl_output_measured;

	if (WARN_ON(ARRAY_SIZE(surface_rt_bl_output_measured) != 256))
		pr_err("%s: bl_output array does not have 256 elements\n", __func__);

	return true;


}

static int cardhu_backlight_notify(struct device *unused, int brightness)
{
	int cur_sd_brightness = atomic_read(&sd_brightness);


	/* SD brightness is a percentage, 8-bit value. */
	brightness = (brightness * cur_sd_brightness) / 255;

	/* Apply any backlight response curve */
	if (brightness > 255) {
		pr_info("Error: Brightness > 255!\n");
	} else {
		brightness = bl_output[brightness];
	}

	return brightness;
}

static int cardhu_disp1_check_fb(struct device *dev, struct fb_info *info);

static struct platform_pwm_backlight_data cardhu_backlight_data = {
	.pwm_id		= 0,
	.max_brightness	= 255,
	.dft_brightness	= 40,
	.pwm_period_ns	= 50000,
	.init		= cardhu_backlight_init,
	.notify		= cardhu_backlight_notify,
	/* Only toggle backlight on fb blank notifications for disp1 */
	.check_fb	= cardhu_disp1_check_fb,
};

static struct platform_device cardhu_backlight_device = {
	.name	= "pwm-backlight",
	.id	= -1,
	.dev	= {
		.platform_data = &cardhu_backlight_data,
	},
};

static int cardhu_panel_enable(struct device *dev)
{
//	if (cardhu_lvds_reg == NULL) {
//		cardhu_lvds_reg = regulator_get(dev, "vdd_lvds");
//		if (WARN_ON(IS_ERR(cardhu_lvds_reg)))
//			pr_err("%s: couldn't get regulator vdd_lvds: %ld\n",
//				__func__, PTR_ERR(cardhu_lvds_reg));
//		else
//			regulator_enable(cardhu_lvds_reg);
//	}

	if (cardhu_lvds_vdd_bl == NULL) {
		cardhu_lvds_vdd_bl = regulator_get(dev, "vdd_backlight");
		if (WARN_ON(IS_ERR(cardhu_lvds_vdd_bl)))
			pr_err("%s: couldn't get regulator vdd_backlight: %ld\n",
				__func__, PTR_ERR(cardhu_lvds_vdd_bl));
		else
			regulator_enable(cardhu_lvds_vdd_bl);
	}

	if (cardhu_lvds_vdd_panel == NULL) {
		cardhu_lvds_vdd_panel = regulator_get(dev, "vdd_lcd_panel");
		if (WARN_ON(IS_ERR(cardhu_lvds_vdd_panel)))
			pr_err("%s: couldn't get regulator vdd_lcd_panel: %ld\n",
				__func__, PTR_ERR(cardhu_lvds_vdd_panel));
		else
			regulator_enable(cardhu_lvds_vdd_panel);
	}

	//gpio_set_value(surface_rt_lvds_shutdown, 1);

	return 0;
}

static int cardhu_panel_disable(void)
{

	mdelay(5);

//	regulator_disable(cardhu_lvds_reg);
//	regulator_put(cardhu_lvds_reg);
//	cardhu_lvds_reg = NULL;

if (cardhu_lvds_vdd_panel) {
	regulator_disable(cardhu_lvds_vdd_bl);
	regulator_put(cardhu_lvds_vdd_bl);
	cardhu_lvds_vdd_bl = NULL;
}

if (cardhu_lvds_vdd_bl) {	
	regulator_disable(cardhu_lvds_vdd_panel);
	regulator_put(cardhu_lvds_vdd_panel);
	cardhu_lvds_vdd_panel= NULL;
}	
	//gpio_set_value(surface_rt_lvds_shutdown, 0);
	
	return 0;
}

static int surface_rt_panel_prepoweroff(void)
{
	gpio_set_value(surface_rt_lvds_shutdown, 0);

	return 0;
}

static int surface_rt_panel_postpoweron(void)
{
	gpio_set_value(surface_rt_lvds_shutdown, 1);

	mdelay(50);

	return 0;
}

//#ifdef CONFIG_TEGRA_DC
//static int cardhu_hdmi_vddio_enable(struct device *dev)
//{
//	int ret =0;
//	if (!cardhu_hdmi_vddio) {
//		cardhu_hdmi_vddio = regulator_get(dev, "vdd_hdmi_con");
//		if (IS_ERR_OR_NULL(cardhu_hdmi_vddio)) {
//			ret = PTR_ERR(cardhu_hdmi_vddio);
//			pr_err("hdmi: couldn't get regulator vdd_hdmi_con\n");
//			cardhu_hdmi_vddio = NULL;
//			return ret;
//		}
//	}
//	ret = regulator_enable(cardhu_hdmi_vddio);
//	if (ret < 0) {
//		pr_err("hdmi: couldn't enable regulator vdd_hdmi_con\n");
//		regulator_put(cardhu_hdmi_vddio);
//		cardhu_hdmi_vddio = NULL;
//		return ret;
//	}
//	return ret;
//}

//static int cardhu_hdmi_vddio_disable(void)
//{
//	if (cardhu_hdmi_vddio) {
//		regulator_disable(cardhu_hdmi_vddio);
//		regulator_put(cardhu_hdmi_vddio);
//		cardhu_hdmi_vddio = NULL;
//	}
//	return 0;
//}

//static int cardhu_hdmi_enable(struct device *dev)
//{
//	int ret;
//	if (!cardhu_hdmi_reg) {
//		cardhu_hdmi_reg = regulator_get(dev, "avdd_hdmi");
//		if (IS_ERR_OR_NULL(cardhu_hdmi_reg)) {
//			pr_err("hdmi: couldn't get regulator avdd_hdmi\n");
//			cardhu_hdmi_reg = NULL;
//			return PTR_ERR(cardhu_hdmi_reg);
//		}
//	}
//	ret = regulator_enable(cardhu_hdmi_reg);
//	if (ret < 0) {
//		pr_err("hdmi: couldn't enable regulator avdd_hdmi\n");
//		return ret;
//	}
//	if (!cardhu_hdmi_pll) {
//		cardhu_hdmi_pll = regulator_get(dev, "avdd_hdmi_pll");
//		if (IS_ERR_OR_NULL(cardhu_hdmi_pll)) {
//			pr_err("hdmi: couldn't get regulator avdd_hdmi_pll\n");
//			cardhu_hdmi_pll = NULL;
//			regulator_put(cardhu_hdmi_reg);
//			cardhu_hdmi_reg = NULL;
//			return PTR_ERR(cardhu_hdmi_pll);
//		}
//	}
//	ret = regulator_enable(cardhu_hdmi_pll);
//	if (ret < 0) {
//		pr_err("hdmi: couldn't enable regulator avdd_hdmi_pll\n");
//		return ret;
//	}
//	return 0;
//}

//static int cardhu_hdmi_disable(void)
//{
//	regulator_disable(cardhu_hdmi_reg);
//	regulator_put(cardhu_hdmi_reg);
//	cardhu_hdmi_reg = NULL;
//	regulator_disable(cardhu_hdmi_pll);
//	regulator_put(cardhu_hdmi_pll);
//	cardhu_hdmi_pll = NULL;
//	return 0;
//}

static struct resource cardhu_disp1_resources[] = {
	{
		.name	= "irq",
		.start	= INT_DISPLAY_GENERAL,
		.end	= INT_DISPLAY_GENERAL,
		.flags	= IORESOURCE_IRQ,
	},
	{
		.name	= "regs",
		.start	= TEGRA_DISPLAY_BASE,
		.end	= TEGRA_DISPLAY_BASE + TEGRA_DISPLAY_SIZE-1,
		.flags	= IORESOURCE_MEM,
	},
	{
		.name	= "fbmem",
		.start	= 0,	/* Filled in by cardhu_panel_init() */
		.end	= 0,	/* Filled in by cardhu_panel_init() */
		.flags	= IORESOURCE_MEM,
	},

};
/*
static struct resource cardhu_disp2_resources[] = {
	{
		.name	= "irq",
		.start	= INT_DISPLAY_B_GENERAL,
		.end	= INT_DISPLAY_B_GENERAL,
		.flags	= IORESOURCE_IRQ,
	},
	{
		.name	= "regs",
		.start	= TEGRA_DISPLAY2_BASE,
		.end	= TEGRA_DISPLAY2_BASE + TEGRA_DISPLAY2_SIZE - 1,
		.flags	= IORESOURCE_MEM,
	},
	{
		.name	= "fbmem",
		.flags	= IORESOURCE_MEM,
		.start	= 0,
		.end	= 0,
	},
};

 */

static struct tegra_dc_mode surface_rt_panel_modes[] = {
	{
		.pclk 		= 71980000,
		.h_ref_to_sync	= 1,
		.v_ref_to_sync	= 1,
		.h_sync_width 	= 14,
		.v_sync_width 	= 1,
		.h_back_porch 	= 106,
		.v_back_porch 	= 6,
		.h_active 	= 1366,
		.v_active 	= 768,
		.h_front_porch	= 56,
		.v_front_porch	= 3,
	},

};


#ifdef CONFIG_TEGRA_DC
static struct tegra_fb_data cardhu_fb_data = {
	.win		= 0,
	.xres		= 1366,
	.yres		= 768,

	.bits_per_pixel	= 32,

//	.flags		= TEGRA_FB_FLIP_ON_PROBE,
};
/*
static struct tegra_fb_data cardhu_hdmi_fb_data = {
	.win		= 0,
	.xres		= 1920,
	.yres		= 1200,

	.bits_per_pixel	= 32,

	.flags		= TEGRA_FB_FLIP_ON_PROBE,
};

static struct tegra_dc_out cardhu_disp2_out = {
	.type		= TEGRA_DC_OUT_HDMI,
	.flags		= TEGRA_DC_OUT_HOTPLUG_HIGH,
	.parent_clk	= "pll_d2_out0",
	.dcc_bus	= 3,
	.hotplug_gpio	= cardhu_hdmi_hpd,
	.max_pixclock	= KHZ2PICOS(148500),
	.align		= TEGRA_DC_ALIGN_MSB,
	.order		= TEGRA_DC_ORDER_RED_BLUE,
	.height = 135,
	.width = 217,
	.enable		= cardhu_hdmi_enable,
	.disable	= cardhu_hdmi_disable,
	.postsuspend	= cardhu_hdmi_vddio_disable,
	.hotplug_init	= cardhu_hdmi_vddio_enable,
};

static struct tegra_dc_platform_data cardhu_disp2_pdata = {
	.flags		= TEGRA_DC_FLAG_ENABLED,
	.default_out	= &cardhu_disp2_out,
	.fb		= &cardhu_hdmi_fb_data,
	.emc_clk_rate	= 300000000,
};
*/
#endif

static struct tegra_dc_out cardhu_disp1_out = {
	.align			= TEGRA_DC_ALIGN_MSB,
	.order			= TEGRA_DC_ORDER_RED_BLUE,
	.parent_clk		= "pll_p",
	.parent_clk_backup 	= "pll_d_out0",

	.type			= TEGRA_DC_OUT_RGB,
	.depth			= 16,
	.dither		= TEGRA_DC_DISABLE_DITHER,

	.modes			= surface_rt_panel_modes,
	.n_modes		= ARRAY_SIZE(surface_rt_panel_modes),

	.enable			= cardhu_panel_enable,
	.postpoweron		= surface_rt_panel_postpoweron,
	.prepoweroff		= surface_rt_panel_prepoweroff,
	.disable		= cardhu_panel_disable,

	.height			= 132,
	.width			= 235,

};

#ifdef CONFIG_TEGRA_DC
static struct tegra_dc_platform_data cardhu_disp1_pdata = {
	.flags		= TEGRA_DC_FLAG_ENABLED,
	.default_out	= &cardhu_disp1_out,
	.emc_clk_rate	= 300000000,
	.fb		= &cardhu_fb_data,
};

static struct platform_device cardhu_disp1_device = {
	.name		= "tegradc",
	.id		= 0,
	.resource	= cardhu_disp1_resources,
	.num_resources	= ARRAY_SIZE(cardhu_disp1_resources),
	.dev = {
		.platform_data = &cardhu_disp1_pdata,
	},
};

static int cardhu_disp1_check_fb(struct device *dev, struct fb_info *info)
{
	return info->device == &cardhu_disp1_device.dev;
}
/*
static struct platform_device cardhu_disp2_device = {
	.name		= "tegradc",
	.id		= 1,
	.resource	= cardhu_disp2_resources,
	.num_resources	= ARRAY_SIZE(cardhu_disp2_resources),
	.dev = {
		.platform_data = &cardhu_disp2_pdata,
	},
};
*/
#else
static int cardhu_disp1_check_fb(struct device *dev, struct fb_info *info)
{
	return 0;
}
#endif

#if defined(CONFIG_TEGRA_NVMAP)
static struct nvmap_platform_carveout cardhu_carveouts[] = {
	[0] = NVMAP_HEAP_CARVEOUT_IRAM_INIT,
	[1] = {
		.name		= "generic-0",
		.usage_mask	= NVMAP_HEAP_CARVEOUT_GENERIC,
		.base		= 0,	/* Filled in by cardhu_panel_init() */
		.size		= 0,	/* Filled in by cardhu_panel_init() */
		.buddy_size	= SZ_32K,
	},
};

static struct nvmap_platform_data cardhu_nvmap_data = {
	.carveouts	= cardhu_carveouts,
	.nr_carveouts	= ARRAY_SIZE(cardhu_carveouts),
};

static struct platform_device cardhu_nvmap_device = {
	.name	= "tegra-nvmap",
	.id	= -1,
	.dev	= {
		.platform_data = &cardhu_nvmap_data,
	},
};
#endif

#if defined(CONFIG_ION_TEGRA)

static struct platform_device tegra_iommu_device = {
	.name = "tegra_iommu_device",
	.id = -1,
	.dev = {
		.platform_data = (void *)((1 << HWGRP_COUNT) - 1),
	},
};

static struct ion_platform_data tegra_ion_data = {
	.nr = 4,
	.heaps = {
		{
			.type = ION_HEAP_TYPE_CARVEOUT,
			.id = TEGRA_ION_HEAP_CARVEOUT,
			.name = "carveout",
			.base = 0,
			.size = 0,
		},
		{
			.type = ION_HEAP_TYPE_CARVEOUT,
			.id = TEGRA_ION_HEAP_IRAM,
			.name = "iram",
			.base = TEGRA_IRAM_BASE + TEGRA_RESET_HANDLER_SIZE,
			.size = TEGRA_IRAM_SIZE - TEGRA_RESET_HANDLER_SIZE,
		},
		{
			.type = ION_HEAP_TYPE_CARVEOUT,
			.id = TEGRA_ION_HEAP_VPR,
			.name = "vpr",
			.base = 0,
			.size = 0,
		},
		{
			.type = ION_HEAP_TYPE_IOMMU,
			.id = TEGRA_ION_HEAP_IOMMU,
			.name = "iommu",
			.base = TEGRA_SMMU_BASE,
			.size = TEGRA_SMMU_SIZE,
			.priv = &tegra_iommu_device.dev,
		},
	},
};

static struct platform_device tegra_ion_device = {
	.name = "ion-tegra",
	.id = -1,
	.dev = {
		.platform_data = &tegra_ion_data,
	},
};
#endif

static struct platform_device *cardhu_gfx_devices[] __initdata = {
#if defined(CONFIG_TEGRA_NVMAP)
	&cardhu_nvmap_device,
#endif
#if defined(CONFIG_ION_TEGRA)
	&tegra_ion_device,
#endif
	&tegra_pwfm0_device,
	&cardhu_backlight_device,
};




//static void cardhu_panel_preinit(void)
//{
//	int ret;
//
//		/* Enable back light */
//		ret = gpio_request(cardhu_bl_enb, "backlight_enb");
//		if (!ret) {
//			ret = gpio_direction_output(cardhu_bl_enb, 1);
//			if (ret < 0) {
//				gpio_free(cardhu_bl_enb);
//				pr_err("Error in setting backlight_enb\n");
//			}
//		} else {
//			pr_err("Error in gpio request for backlight_enb\n");
//			}
//}

int __init cardhu_panel_init(void)
{
	int err;
	struct resource __maybe_unused *res;
	struct platform_device *phost1x;

#if defined(CONFIG_TEGRA_NVMAP)
	cardhu_carveouts[1].base = tegra_carveout_start;
	cardhu_carveouts[1].size = tegra_carveout_size;
#endif

#if defined(CONFIG_ION_TEGRA)
	tegra_ion_data.heaps[0].base = tegra_carveout_start;
	tegra_ion_data.heaps[0].size = tegra_carveout_size;
#endif

//	cardhu_panel_preinit();

#if defined(CONFIG_TEGRA_DC)

//	gpio_request(surface_rt_lvds_shutdown, "lvds_shutdown");
//	gpio_direction_output(surface_rt_lvds_shutdown, 1);
	
#endif

//	gpio_request(cardhu_hdmi_hpd, "hdmi_hpd");
//	gpio_direction_input(cardhu_hdmi_hpd);

	err = platform_add_devices(cardhu_gfx_devices,
				ARRAY_SIZE(cardhu_gfx_devices));

#ifdef CONFIG_TEGRA_GRHOST
	phost1x = tegra3_register_host1x_devices();
	if (!phost1x)
		return -EINVAL;
#endif

#if defined(CONFIG_TEGRA_GRHOST) && defined(CONFIG_TEGRA_DC)
	res = platform_get_resource_byname(&cardhu_disp1_device,
					 IORESOURCE_MEM, "fbmem");
	res->start = tegra_fb_start;
	res->end = tegra_fb_start + tegra_fb_size - 1;
#endif

	/* Copy the bootloader fb to the fb. */
	__tegra_move_framebuffer(&cardhu_nvmap_device,
			       tegra_fb_start, tegra_bootloader_fb_start,
				min(tegra_fb_size, tegra_bootloader_fb_size));

#if defined(CONFIG_TEGRA_GRHOST) && defined(CONFIG_TEGRA_DC)
	if (!err) {
		cardhu_disp1_device.dev.parent = &phost1x->dev;
		err = platform_device_register(&cardhu_disp1_device);
	}

///	res = platform_get_resource_byname(&cardhu_disp2_device,
//					 IORESOURCE_MEM, "fbmem");
//	res->start = tegra_fb2_start;
//	res->end = tegra_fb2_start + tegra_fb2_size - 1;

	/*
	 * If the bootloader fb2 is valid, copy it to the fb2, or else
	 * clear fb2 to avoid garbage on dispaly2.
	 */
//	if (tegra_bootloader_fb2_size)
//		__tegra_move_framebuffer(&cardhu_nvmap_device,
//			tegra_fb2_start, tegra_bootloader_fb2_start,
//			min(tegra_fb2_size, tegra_bootloader_fb2_size));
//	else
//		__tegra_clear_framebuffer(&cardhu_nvmap_device,
//					  tegra_fb2_start, tegra_fb2_size);

//	if (!err) {
//		cardhu_disp2_device.dev.parent = &phost1x->dev;
//		err = platform_device_register(&cardhu_disp2_device);
//	}
#endif

#if defined(CONFIG_TEGRA_GRHOST) && defined(CONFIG_TEGRA_NVAVP)
	if (!err) {
		nvavp_device.dev.parent = &phost1x->dev;
		err = platform_device_register(&nvavp_device);
	}
#endif
	return err;
}
