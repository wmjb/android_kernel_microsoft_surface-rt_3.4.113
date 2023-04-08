/*
 * arch/arm/mach-tegra/panel-lgd-wxga-7-0.c
 *
 * Copyright (c) 2013, NVIDIA CORPORATION.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <mach/dc.h>
#include <mach/iomap.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/tegra_pwm_bl.h>
#include <linux/regulator/consumer.h>
#include <linux/pwm_backlight.h>
#include <linux/max8831_backlight.h>
#include <linux/leds.h>
#include <linux/ioport.h>
#include <generated/mach-types.h>
#include "board.h"
#include "board-panel.h"
#include "devices.h"
#include "gpio-names.h"
#include "tegra11_host1x_devices.h"


#define DSI_PANEL_RESET		0
#define DSI_PANEL_BL_PWM	TEGRA_GPIO_PH1
#define DC_CTRL_MODE	(TEGRA_DC_OUT_CONTINUOUS_MODE | \
			TEGRA_DC_OUT_INITIALIZED_MODE)

/*
 * TODO: This flag and continuous video clk mode
 * support will be removed in near future.
 * Tx Only video clk mode will be enabled all the time.
 */
#define VIDEO_CLK_MODE_TX_ONLY		1

static bool reg_requested;
static struct platform_device *disp_device;
static struct regulator *avdd_lcd_3v3;
static struct regulator *vdd_lcd_bl_en;

static struct tegra_dsi_cmd dsi_lgd_wxga_7_0_init_cmd[] = {
	DSI_CMD_SHORT(0x15, 0x01, 0x0),
	DSI_DLY_MS(20),
	DSI_CMD_SHORT(0x15, 0xAE, 0x0B),
	DSI_CMD_SHORT(0x15, 0xEE, 0xEA),
	DSI_CMD_SHORT(0x15, 0xEF, 0x5F),
	DSI_CMD_SHORT(0x15, 0xF2, 0x68),
	DSI_CMD_SHORT(0x15, 0xEE, 0x0),
	DSI_CMD_SHORT(0x15, 0xEF, 0x0),
};

static struct tegra_dsi_cmd dsi_lgd_wxga_7_0_late_resume_cmd[] = {
	DSI_CMD_SHORT(0x15, 0x10, 0x0),
	DSI_DLY_MS(120),
};

static struct tegra_dsi_cmd dsi_lgd_wxga_7_0_early_suspend_cmd[] = {
	DSI_CMD_SHORT(0x15, 0x11, 0x0),
	DSI_DLY_MS(160),
};

static struct tegra_dsi_cmd dsi_lgd_wxga_7_0_suspend_cmd[] = {
	DSI_CMD_SHORT(0x15, 0x11, 0x0),
	DSI_DLY_MS(160),
};

static struct tegra_dsi_out dsi_lgd_wxga_7_0_pdata = {
#ifdef CONFIG_ARCH_TEGRA_3x_SOC
	.n_data_lanes = 2,
	.controller_vs = DSI_VS_0,
#else
	.controller_vs = DSI_VS_1,
#endif

	.n_data_lanes = 4,
	.video_burst_mode = TEGRA_DSI_VIDEO_NONE_BURST_MODE,

	.pixel_format = TEGRA_DSI_PIXEL_FORMAT_24BIT_P,
	.refresh_rate = 60,
	.virtual_channel = TEGRA_DSI_VIRTUAL_CHANNEL_0,

	.dsi_instance = DSI_INSTANCE_0,

	.panel_reset = DSI_PANEL_RESET,
	.power_saving_suspend = true,
	.video_data_type = TEGRA_DSI_VIDEO_TYPE_VIDEO_MODE,
#if VIDEO_CLK_MODE_TX_ONLY
	.video_clock_mode = TEGRA_DSI_VIDEO_CLOCK_TX_ONLY,
#else
	.video_clock_mode = TEGRA_DSI_VIDEO_CLOCK_CONTINUOUS,
#endif
	.dsi_init_cmd = dsi_lgd_wxga_7_0_init_cmd,
	.n_init_cmd = ARRAY_SIZE(dsi_lgd_wxga_7_0_init_cmd),

	.dsi_early_suspend_cmd = dsi_lgd_wxga_7_0_early_suspend_cmd,
	.n_early_suspend_cmd = ARRAY_SIZE(dsi_lgd_wxga_7_0_early_suspend_cmd),

	.dsi_late_resume_cmd = dsi_lgd_wxga_7_0_late_resume_cmd,
	.n_late_resume_cmd = ARRAY_SIZE(dsi_lgd_wxga_7_0_late_resume_cmd),

	.dsi_suspend_cmd = dsi_lgd_wxga_7_0_suspend_cmd,
	.n_suspend_cmd = ARRAY_SIZE(dsi_lgd_wxga_7_0_suspend_cmd),
};

static int tegratab_dsi_regulator_get(struct device *dev)
{
	int err = 0;

	if (reg_requested)
		return 0;
	avdd_lcd_3v3 = regulator_get(dev, "avdd_lcd");
	if (IS_ERR_OR_NULL(avdd_lcd_3v3)) {
		pr_err("avdd_lcd regulator get failed\n");
		err = PTR_ERR(avdd_lcd_3v3);
		avdd_lcd_3v3 = NULL;
		goto fail;
	}

	vdd_lcd_bl_en = regulator_get(dev, "vdd_lcd_bl_en");
	if (IS_ERR_OR_NULL(vdd_lcd_bl_en)) {
		pr_err("vdd_lcd_bl_en regulator get failed\n");
		err = PTR_ERR(vdd_lcd_bl_en);
		vdd_lcd_bl_en = NULL;
		goto fail;
	}
	reg_requested = true;
	return 0;
fail:
	return err;
}

static int dsi_lgd_wxga_7_0_enable(struct device *dev)
{
	int err = 0;

	err = tegratab_dsi_regulator_get(dev);

	if (err < 0) {
		pr_err("dsi regulator get failed\n");
		goto fail;
	}

	/*
	 * Turning on 1.8V then AVDD after 5ms is required per spec.
	 */
	msleep(20);

	if (avdd_lcd_3v3) {
		err = regulator_enable(avdd_lcd_3v3);
		if (err < 0) {
			pr_err("avdd_lcd regulator enable failed\n");
			goto fail;
		}
		regulator_set_voltage(avdd_lcd_3v3, 3300000, 3300000);
	}

	msleep(150);
	if (vdd_lcd_bl_en) {
		err = regulator_enable(vdd_lcd_bl_en);
		if (err < 0) {
			pr_err("vdd_lcd_bl_en regulator enable failed\n");
			goto fail;
		}
	}

	msleep(100);
#if DSI_PANEL_RESET
/*
 * Nothing is requested.
 */
#endif

	return 0;
fail:
	return err;
}

static int dsi_lgd_wxga_7_0_disable(void)
{
	if (vdd_lcd_bl_en)
		regulator_disable(vdd_lcd_bl_en);

	if (avdd_lcd_3v3)
		regulator_disable(avdd_lcd_3v3);

	return 0;
}

static int dsi_lgd_wxga_7_0_postsuspend(void)
{
	return 0;
}

/*
 * See display standard timings and a few constraints underneath
 * \vendor\nvidia\tegra\core\drivers\hwinc
 *
 * Class: Display Standard Timings
 *
 * Programming of display timing registers must meet these restrictions:
 * Constraint 1: H_REF_TO_SYNC + H_SYNC_WIDTH + H_BACK_PORCH > 11.
 * Constraint 2: V_REF_TO_SYNC + V_SYNC_WIDTH + V_BACK_PORCH > 1.
 * Constraint 3: V_FRONT_PORCH + V_SYNC_WIDTH +
				V_BACK_PORCH > 1 (vertical blank).
 * Constraint 4: V_SYNC_WIDTH >= 1, H_SYNC_WIDTH >= 1
 * Constraint 5: V_REF_TO_SYNC >= 1, H_REF_TO_SYNC >= 0
 * Constraint 6: V_FRONT_PORT >= (V_REF_TO_SYNC + 1),
				H_FRONT_PORT >= (H_REF_TO_SYNC + 1)
 * Constraint 7: H_DISP_ACTIVE >= 16, V_DISP_ACTIVE >= 16
 */

/*
 * how to determine pclk
 * h_total =
 * Horiz_BackPorch + Horiz_SyncWidth + Horiz_DispActive + Horiz_FrontPorch;
 *
 * v_total =
 * Vert_BackPorch + Vert_SyncWidth + Vert_DispActive + Vert_FrontPorch;
 * panel_freq = ( h_total * v_total * refresh_freq );
 */
#if VIDEO_CLK_MODE_TX_ONLY
static struct tegra_dc_mode dsi_lgd_wxga_7_0_modes[] = {
	{
		.pclk = 71000000, /* 890 * 1323 * 60 = 70648200 */
		.h_ref_to_sync = 10,
		.v_ref_to_sync = 1,
		.h_sync_width = 1,
		.v_sync_width = 1,
		.h_back_porch = 57,
		.v_back_porch = 14,
		.h_active = 800,
		.v_active = 1280,
		.h_front_porch = 32,
		.v_front_porch = 28,
	},
};
#else
static struct tegra_dc_mode dsi_lgd_wxga_7_0_modes[] = {
	{
		.pclk = 68600000, /* 890 * 1284 * 60 = 68565600 */
		.h_ref_to_sync = 10,
		.v_ref_to_sync = 1,
		.h_sync_width = 1,
		.v_sync_width = 1,
		.h_back_porch = 57,
		.v_back_porch = 2,
		.h_active = 800,
		.v_active = 1280,
		.h_front_porch = 32,
		.v_front_porch = 1,
	},
};
#endif

static int dsi_lgd_wxga_7_0_bl_notify(struct device *unused, int brightness)
{
	/*
	 * In early panel bring-up, we will
	 * not enable PRISM.
	 * Just use same brightness that is delivered from user side.
	 * TODO...
	 * use PRSIM brightness later.
	 */
	if (brightness > 255) {
		pr_info("Error: Brightness > 255!\n");
		brightness = 255;
	}
	return brightness;
}

static int dsi_lgd_wxga_7_0_check_fb(struct device *dev, struct fb_info *info)
{
	return info->device == &disp_device->dev;
}

static struct platform_pwm_backlight_data dsi_lgd_wxga_7_0_bl_data = {
	.pwm_id		= 1,
	.max_brightness	= 255,
	.dft_brightness	= 224,
	.pwm_period_ns	= 1000000,
	.notify		= dsi_lgd_wxga_7_0_bl_notify,
	/* Only toggle backlight on fb blank notifications for disp1 */
	.check_fb	= dsi_lgd_wxga_7_0_check_fb,
	.pwm_gpio	= DSI_PANEL_BL_PWM,
};

static struct platform_device __maybe_unused
		dsi_lgd_wxga_7_0_bl_device = {
	.name	= "pwm-backlight",
	.id	= -1,
	.dev	= {
		.platform_data = &dsi_lgd_wxga_7_0_bl_data,
	},
};

static struct platform_device __maybe_unused
			*dsi_lgd_wxga_7_0_bl_devices[] __initdata = {
	&tegra_pwfm1_device,
	&dsi_lgd_wxga_7_0_bl_device,
};

static int  __init dsi_lgd_wxga_7_0_register_bl_dev(void)
{
	int err = 0;

#ifdef CONFIG_ANDROID
	if (get_androidboot_mode_charger())
		dsi_lgd_wxga_7_0_bl_data.dft_brightness = 60;
#endif
	err = platform_add_devices(dsi_lgd_wxga_7_0_bl_devices,
				ARRAY_SIZE(dsi_lgd_wxga_7_0_bl_devices));
	if (err) {
		pr_err("disp1 bl device registration failed");
		return err;
	}
	return err;
}

static void dsi_lgd_wxga_7_0_set_disp_device(
	struct platform_device *display_device)
{
	disp_device = display_device;
}

static void dsi_lgd_wxga_7_0_resources_init(struct resource *
resources, int n_resources)
{
	int i;
	for (i = 0; i < n_resources; i++) {
		struct resource *r = &resources[i];
		if (resource_type(r) == IORESOURCE_MEM &&
			!strcmp(r->name, "dsi_regs")) {
			r->start = TEGRA_DSI_BASE;
			r->end = TEGRA_DSI_BASE + TEGRA_DSI_SIZE - 1;
		}
	}
}

static void dsi_lgd_wxga_7_0_dc_out_init(struct tegra_dc_out *dc)
{
	/*
	 * Values to meet D-Phy timings,
	 * These values are get from measurement with target panel.
	 */
	dsi_lgd_wxga_7_0_pdata.phy_timing.t_clkprepare_ns = 27;
	dsi_lgd_wxga_7_0_pdata.phy_timing.t_clkzero_ns = 330;
	dsi_lgd_wxga_7_0_pdata.phy_timing.t_hsprepare_ns = 30;
	dsi_lgd_wxga_7_0_pdata.phy_timing.t_datzero_ns = 270;

	dc->dsi = &dsi_lgd_wxga_7_0_pdata;
	dc->parent_clk = "pll_d_out0";
	dc->modes = dsi_lgd_wxga_7_0_modes;
	dc->n_modes = ARRAY_SIZE(dsi_lgd_wxga_7_0_modes);
	dc->enable = dsi_lgd_wxga_7_0_enable;
	dc->disable = dsi_lgd_wxga_7_0_disable;
	dc->postsuspend = dsi_lgd_wxga_7_0_postsuspend,
	dc->width = 94;
	dc->height = 150;
	dc->flags = DC_CTRL_MODE;
}

static void dsi_lgd_wxga_7_0_fb_data_init(struct tegra_fb_data *fb)
{
	fb->xres = dsi_lgd_wxga_7_0_modes[0].h_active;
	fb->yres = dsi_lgd_wxga_7_0_modes[0].v_active;
}

static void
dsi_lgd_wxga_7_0_sd_settings_init(struct tegra_dc_sd_settings *settings)
{
	settings->bl_device_name = "pwm-backlight";
}

static void dsi_lgd_wxga_7_0_cmu_init(struct tegra_dc_platform_data *pdata)
{
	pdata->cmu = NULL; /* will write CMU stuff after calibration */
}

struct tegra_panel __initdata dsi_lgd_wxga_7_0 = {
	.init_sd_settings = dsi_lgd_wxga_7_0_sd_settings_init,
	.init_dc_out = dsi_lgd_wxga_7_0_dc_out_init,
	.init_fb_data = dsi_lgd_wxga_7_0_fb_data_init,
	.init_resources = dsi_lgd_wxga_7_0_resources_init,
	.register_bl_dev = dsi_lgd_wxga_7_0_register_bl_dev,
	.init_cmu_data = dsi_lgd_wxga_7_0_cmu_init,
	.set_disp_device = dsi_lgd_wxga_7_0_set_disp_device,
};
EXPORT_SYMBOL(dsi_lgd_wxga_7_0);

