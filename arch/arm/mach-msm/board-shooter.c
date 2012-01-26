#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/mfd/pmic8058.h>

#include <linux/bootmem.h>
#include <linux/proc_fs.h>

#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/hardware/gic.h>


#include "devices-msm8x60.h"

#include <mach/board.h>
#include <mach/board_htc.h>
#include <mach/msm_iomap.h>
#include <mach/msm_xo.h>
#include <mach/restart.h>
#include <mach/rpm.h>
#include <mach/rpm-regulator.h>
#include <mach/socinfo.h>

#include "acpuclock.h"
#include "devices.h"
#include "rpm_resources.h"
#include "spm.h"
#include "timer.h"
#include "board-shooter.h"

unsigned engineerid, mem_size_mb;

#define MSM_SHARED_RAM_PHYS		0x40000000
#define MSM_RAM_CONSOLE_BASE		0x40300000
#define MSM_RAM_CONSOLE_SIZE		(SZ_1M - SZ_128K)

static struct resource ram_console_resources[] = {
	{
		.start  = MSM_RAM_CONSOLE_BASE,
		.end    = MSM_RAM_CONSOLE_BASE + MSM_RAM_CONSOLE_SIZE - 1,
		.flags  = IORESOURCE_MEM,
	},
};

static struct platform_device ram_console_device = {
	.name		= "ram_console",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(ram_console_resources),
	.resource	= ram_console_resources,
};

static struct msm_rpmrs_level msm_rpmrs_levels[] __initdata = {
	{
		MSM_PM_SLEEP_MODE_WAIT_FOR_INTERRUPT,
		MSM_RPMRS_LIMITS(ON, ACTIVE, MAX, ACTIVE),
		true,
		1, 8000, 100000, 1,
	},

	{
		MSM_PM_SLEEP_MODE_POWER_COLLAPSE_STANDALONE,
		MSM_RPMRS_LIMITS(ON, ACTIVE, MAX, ACTIVE),
		true,
		1500, 5000, 60100000, 3000,
	},

	{
		MSM_PM_SLEEP_MODE_POWER_COLLAPSE,
		MSM_RPMRS_LIMITS(ON, ACTIVE, MAX, ACTIVE),
		false,
		1800, 5000, 60350000, 3500,
	},
	{
		MSM_PM_SLEEP_MODE_POWER_COLLAPSE,
		MSM_RPMRS_LIMITS(OFF, ACTIVE, MAX, ACTIVE),
		false,
		3800, 4500, 65350000, 5500,
	},

	{
		MSM_PM_SLEEP_MODE_POWER_COLLAPSE,
		MSM_RPMRS_LIMITS(ON, HSFS_OPEN, MAX, ACTIVE),
		false,
		2800, 2500, 66850000, 4800,
	},

	{
		MSM_PM_SLEEP_MODE_POWER_COLLAPSE,
		MSM_RPMRS_LIMITS(OFF, HSFS_OPEN, MAX, ACTIVE),
		false,
		4800, 2000, 71850000, 6800,
	},

	{
		MSM_PM_SLEEP_MODE_POWER_COLLAPSE,
		MSM_RPMRS_LIMITS(OFF, HSFS_OPEN, ACTIVE, RET_HIGH),
		false,
		6800, 500, 75850000, 8800,
	},

	{
		MSM_PM_SLEEP_MODE_POWER_COLLAPSE,
		MSM_RPMRS_LIMITS(OFF, HSFS_OPEN, RET_HIGH, RET_LOW),
		false,
		7800, 0, 76350000, 9800,
	},
};

static struct msm_rpmrs_platform_data msm_rpmrs_data __initdata = {
	.levels = &msm_rpmrs_levels[0],
	.num_levels = ARRAY_SIZE(msm_rpmrs_levels),
	.vdd_mem_levels  = {
		[MSM_RPMRS_VDD_MEM_RET_LOW]     = 500,
		[MSM_RPMRS_VDD_MEM_RET_HIGH]    = 750,
		[MSM_RPMRS_VDD_MEM_ACTIVE]      = 1000,
		[MSM_RPMRS_VDD_MEM_MAX]         = 1250,
	},
	.vdd_dig_levels = {
		[MSM_RPMRS_VDD_DIG_RET_LOW]     = 500,
		[MSM_RPMRS_VDD_DIG_RET_HIGH]    = 750,
		[MSM_RPMRS_VDD_DIG_ACTIVE]      = 1000,
		[MSM_RPMRS_VDD_DIG_MAX]         = 1250,
	},
	.vdd_mask = 0xFFF,
	.rpmrs_target_id = {
		[MSM_RPMRS_ID_PXO_CLK]          = MSM_RPM_ID_PXO_CLK,
		[MSM_RPMRS_ID_L2_CACHE_CTL]     = MSM_RPM_ID_APPS_L2_CACHE_CTL,
		[MSM_RPMRS_ID_VDD_DIG_0]        = MSM_RPM_ID_SMPS1_0,
		[MSM_RPMRS_ID_VDD_DIG_1]        = MSM_RPM_ID_SMPS1_1,
		[MSM_RPMRS_ID_VDD_MEM_0]        = MSM_RPM_ID_SMPS0_0,
		[MSM_RPMRS_ID_VDD_MEM_1]        = MSM_RPM_ID_SMPS0_1,
		[MSM_RPMRS_ID_RPM_CTL]          = MSM_RPM_ID_TRIGGER_SET_FROM,
	},
};

static struct msm_spm_platform_data msm_spm_data_v1[] __initdata = {
	[0] = {
		.reg_base_addr = MSM_SAW0_BASE,

#ifdef CONFIG_MSM_AVS_HW
		.reg_init_values[MSM_SPM_REG_SAW_AVS_CTL] = 0x586020FF,
#endif
		.reg_init_values[MSM_SPM_REG_SAW_CFG] = 0x0F,
		.reg_init_values[MSM_SPM_REG_SAW_SPM_CTL] = 0x68,
		.reg_init_values[MSM_SPM_REG_SAW_SPM_SLP_TMR_DLY] = 0xFFFFFFFF,
		.reg_init_values[MSM_SPM_REG_SAW_SPM_WAKE_TMR_DLY] = 0xFFFFFFFF,

		.reg_init_values[MSM_SPM_REG_SAW_SLP_CLK_EN] = 0x01,
		.reg_init_values[MSM_SPM_REG_SAW_SLP_HSFS_PRECLMP_EN] = 0x07,
		.reg_init_values[MSM_SPM_REG_SAW_SLP_HSFS_POSTCLMP_EN] = 0x00,

		.reg_init_values[MSM_SPM_REG_SAW_SLP_CLMP_EN] = 0x01,
		.reg_init_values[MSM_SPM_REG_SAW_SLP_RST_EN] = 0x00,
		.reg_init_values[MSM_SPM_REG_SAW_SPM_MPM_CFG] = 0x00,

		.awake_vlevel = 0x94,
		.retention_vlevel = 0x81,
		.collapse_vlevel = 0x20,
		.retention_mid_vlevel = 0x94,
		.collapse_mid_vlevel = 0x8C,

		.vctl_timeout_us = 50,
	},

	[1] = {
		.reg_base_addr = MSM_SAW1_BASE,

#ifdef CONFIG_MSM_AVS_HW
		.reg_init_values[MSM_SPM_REG_SAW_AVS_CTL] = 0x586020FF,
#endif
		.reg_init_values[MSM_SPM_REG_SAW_CFG] = 0x0F,
		.reg_init_values[MSM_SPM_REG_SAW_SPM_CTL] = 0x68,
		.reg_init_values[MSM_SPM_REG_SAW_SPM_SLP_TMR_DLY] = 0xFFFFFFFF,
		.reg_init_values[MSM_SPM_REG_SAW_SPM_WAKE_TMR_DLY] = 0xFFFFFFFF,

		.reg_init_values[MSM_SPM_REG_SAW_SLP_CLK_EN] = 0x13,
		.reg_init_values[MSM_SPM_REG_SAW_SLP_HSFS_PRECLMP_EN] = 0x07,
		.reg_init_values[MSM_SPM_REG_SAW_SLP_HSFS_POSTCLMP_EN] = 0x00,

		.reg_init_values[MSM_SPM_REG_SAW_SLP_CLMP_EN] = 0x01,
		.reg_init_values[MSM_SPM_REG_SAW_SLP_RST_EN] = 0x00,
		.reg_init_values[MSM_SPM_REG_SAW_SPM_MPM_CFG] = 0x00,

		.awake_vlevel = 0x94,
		.retention_vlevel = 0x81,
		.collapse_vlevel = 0x20,
		.retention_mid_vlevel = 0x94,
		.collapse_mid_vlevel = 0x8C,

		.vctl_timeout_us = 50,
	},
};

static struct msm_spm_platform_data msm_spm_data[] __initdata = {
	[0] = {
		.reg_base_addr = MSM_SAW0_BASE,

#ifdef CONFIG_MSM_AVS_HW
		.reg_init_values[MSM_SPM_REG_SAW_AVS_CTL] = 0x586020FF,
#endif
		.reg_init_values[MSM_SPM_REG_SAW_CFG] = 0x1C,
		.reg_init_values[MSM_SPM_REG_SAW_SPM_CTL] = 0x68,
		.reg_init_values[MSM_SPM_REG_SAW_SPM_SLP_TMR_DLY] = 0x0C0CFFFF,
		.reg_init_values[MSM_SPM_REG_SAW_SPM_WAKE_TMR_DLY] = 0x78780FFF,

		.reg_init_values[MSM_SPM_REG_SAW_SLP_CLK_EN] = 0x01,
		.reg_init_values[MSM_SPM_REG_SAW_SLP_HSFS_PRECLMP_EN] = 0x07,
		.reg_init_values[MSM_SPM_REG_SAW_SLP_HSFS_POSTCLMP_EN] = 0x00,

		.reg_init_values[MSM_SPM_REG_SAW_SLP_CLMP_EN] = 0x01,
		.reg_init_values[MSM_SPM_REG_SAW_SLP_RST_EN] = 0x00,
		.reg_init_values[MSM_SPM_REG_SAW_SPM_MPM_CFG] = 0x00,

		.awake_vlevel = 0xA0,
		.retention_vlevel = 0x89,
		.collapse_vlevel = 0x20,
		.retention_mid_vlevel = 0x89,
		.collapse_mid_vlevel = 0x89,

		.vctl_timeout_us = 50,
	},

	[1] = {
		.reg_base_addr = MSM_SAW1_BASE,

#ifdef CONFIG_MSM_AVS_HW
		.reg_init_values[MSM_SPM_REG_SAW_AVS_CTL] = 0x586020FF,
#endif
		.reg_init_values[MSM_SPM_REG_SAW_CFG] = 0x1C,
		.reg_init_values[MSM_SPM_REG_SAW_SPM_CTL] = 0x68,
		.reg_init_values[MSM_SPM_REG_SAW_SPM_SLP_TMR_DLY] = 0x0C0CFFFF,
		.reg_init_values[MSM_SPM_REG_SAW_SPM_WAKE_TMR_DLY] = 0x78780FFF,

		.reg_init_values[MSM_SPM_REG_SAW_SLP_CLK_EN] = 0x13,
		.reg_init_values[MSM_SPM_REG_SAW_SLP_HSFS_PRECLMP_EN] = 0x07,
		.reg_init_values[MSM_SPM_REG_SAW_SLP_HSFS_POSTCLMP_EN] = 0x00,

		.reg_init_values[MSM_SPM_REG_SAW_SLP_CLMP_EN] = 0x01,
		.reg_init_values[MSM_SPM_REG_SAW_SLP_RST_EN] = 0x00,
		.reg_init_values[MSM_SPM_REG_SAW_SPM_MPM_CFG] = 0x00,

		.awake_vlevel = 0xA0,
		.retention_vlevel = 0x89,
		.collapse_vlevel = 0x20,
		.retention_mid_vlevel = 0x89,
		.collapse_mid_vlevel = 0x89,

		.vctl_timeout_us = 50,
	},
};

/*
 * Consumer specific regulator names:
 *			 regulator name		consumer dev_name
 */
static struct regulator_consumer_supply vreg_consumers_8901_S0[] = {
	REGULATOR_SUPPLY("8901_s0",		NULL),
};
static struct regulator_consumer_supply vreg_consumers_8901_S1[] = {
	REGULATOR_SUPPLY("8901_s1",		NULL),
};

static struct regulator_init_data saw_s0_init_data = {
		.constraints = {
			.name = "8901_s0",
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE,
			.min_uV = 800000,
			.max_uV = 1250000,
		},
		.consumer_supplies = vreg_consumers_8901_S0,
		.num_consumer_supplies = ARRAY_SIZE(vreg_consumers_8901_S0),
};

static struct regulator_init_data saw_s1_init_data = {
		.constraints = {
			.name = "8901_s1",
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE,
			.min_uV = 800000,
			.max_uV = 1250000,
		},
		.consumer_supplies = vreg_consumers_8901_S1,
		.num_consumer_supplies = ARRAY_SIZE(vreg_consumers_8901_S1),
};

static struct platform_device msm_device_saw_s0 = {
	.name          = "saw-regulator",
	.id            = 0,
	.dev           = {
		.platform_data = &saw_s0_init_data,
	},
};

static struct platform_device msm_device_saw_s1 = {
	.name          = "saw-regulator",
	.id            = 1,
	.dev           = {
		.platform_data = &saw_s1_init_data,
	},
};

/*
 * Consumer specific regulator names:
 *			 regulator name		consumer dev_name
 */
static struct regulator_consumer_supply vreg_consumers_PM8058_L0[] = {
	REGULATOR_SUPPLY("8058_l0",		NULL),
};
static struct regulator_consumer_supply vreg_consumers_PM8058_L1[] = {
	REGULATOR_SUPPLY("8058_l1",		NULL),
};
static struct regulator_consumer_supply vreg_consumers_PM8058_L2[] = {
	REGULATOR_SUPPLY("8058_l2",		NULL),
};
static struct regulator_consumer_supply vreg_consumers_PM8058_L3[] = {
	REGULATOR_SUPPLY("8058_l3",		NULL),
};
static struct regulator_consumer_supply vreg_consumers_PM8058_L4[] = {
	REGULATOR_SUPPLY("8058_l4",		NULL),
};
static struct regulator_consumer_supply vreg_consumers_PM8058_L5[] = {
	REGULATOR_SUPPLY("8058_l5",		NULL),
};
static struct regulator_consumer_supply vreg_consumers_PM8058_L6[] = {
	REGULATOR_SUPPLY("8058_l6",		NULL),
};
static struct regulator_consumer_supply vreg_consumers_PM8058_L7[] = {
	REGULATOR_SUPPLY("8058_l7",		NULL),
};
static struct regulator_consumer_supply vreg_consumers_PM8058_L8[] = {
	REGULATOR_SUPPLY("8058_l8",		NULL),
};
static struct regulator_consumer_supply vreg_consumers_PM8058_L9[] = {
	REGULATOR_SUPPLY("8058_l9",		NULL),
};
static struct regulator_consumer_supply vreg_consumers_PM8058_L10[] = {
	REGULATOR_SUPPLY("8058_l10",		NULL),
};
static struct regulator_consumer_supply vreg_consumers_PM8058_L11[] = {
	REGULATOR_SUPPLY("8058_l11",		NULL),
};
static struct regulator_consumer_supply vreg_consumers_PM8058_L12[] = {
	REGULATOR_SUPPLY("8058_l12",		NULL),
};
static struct regulator_consumer_supply vreg_consumers_PM8058_L13[] = {
	REGULATOR_SUPPLY("8058_l13",		NULL),
};
static struct regulator_consumer_supply vreg_consumers_PM8058_L14[] = {
	REGULATOR_SUPPLY("8058_l14",		NULL),
};
static struct regulator_consumer_supply vreg_consumers_PM8058_L15[] = {
	REGULATOR_SUPPLY("8058_l15",		NULL),
};
static struct regulator_consumer_supply vreg_consumers_PM8058_L16[] = {
	REGULATOR_SUPPLY("8058_l16",		NULL),
};
static struct regulator_consumer_supply vreg_consumers_PM8058_L17[] = {
	REGULATOR_SUPPLY("8058_l17",		NULL),
};
static struct regulator_consumer_supply vreg_consumers_PM8058_L18[] = {
	REGULATOR_SUPPLY("8058_l18",		NULL),
};
static struct regulator_consumer_supply vreg_consumers_PM8058_L19[] = {
	REGULATOR_SUPPLY("8058_l19",		NULL),
};
static struct regulator_consumer_supply vreg_consumers_PM8058_L20[] = {
	REGULATOR_SUPPLY("8058_l20",		NULL),
};
static struct regulator_consumer_supply vreg_consumers_PM8058_L21[] = {
	REGULATOR_SUPPLY("8058_l21",		NULL),
};
static struct regulator_consumer_supply vreg_consumers_PM8058_L22[] = {
	REGULATOR_SUPPLY("8058_l22",		NULL),
};
static struct regulator_consumer_supply vreg_consumers_PM8058_L23[] = {
	REGULATOR_SUPPLY("8058_l23",		NULL),
};
static struct regulator_consumer_supply vreg_consumers_PM8058_L24[] = {
	REGULATOR_SUPPLY("8058_l24",		NULL),
};
static struct regulator_consumer_supply vreg_consumers_PM8058_L25[] = {
	REGULATOR_SUPPLY("8058_l25",		NULL),
};
static struct regulator_consumer_supply vreg_consumers_PM8058_S0[] = {
	REGULATOR_SUPPLY("8058_s0",		NULL),
};
static struct regulator_consumer_supply vreg_consumers_PM8058_S1[] = {
	REGULATOR_SUPPLY("8058_s1",		NULL),
};
static struct regulator_consumer_supply vreg_consumers_PM8058_S2[] = {
	REGULATOR_SUPPLY("8058_s2",		NULL),
};
static struct regulator_consumer_supply vreg_consumers_PM8058_S3[] = {
	REGULATOR_SUPPLY("8058_s3",		NULL),
};
static struct regulator_consumer_supply vreg_consumers_PM8058_S4[] = {
	REGULATOR_SUPPLY("8058_s4",		NULL),
};
static struct regulator_consumer_supply vreg_consumers_PM8058_LVS0[] = {
	REGULATOR_SUPPLY("8058_lvs0",		NULL),
};
static struct regulator_consumer_supply vreg_consumers_PM8058_LVS1[] = {
	REGULATOR_SUPPLY("8058_lvs1",		NULL),
};
static struct regulator_consumer_supply vreg_consumers_PM8058_NCP[] = {
	REGULATOR_SUPPLY("8058_ncp",		NULL),
};

static struct regulator_consumer_supply vreg_consumers_PM8901_L0[] = {
	REGULATOR_SUPPLY("8901_l0",		NULL),
};
static struct regulator_consumer_supply vreg_consumers_PM8901_L1[] = {
	REGULATOR_SUPPLY("8901_l1",		NULL),
};
static struct regulator_consumer_supply vreg_consumers_PM8901_L2[] = {
	REGULATOR_SUPPLY("8901_l2",		NULL),
};
static struct regulator_consumer_supply vreg_consumers_PM8901_L3[] = {
	REGULATOR_SUPPLY("8901_l3",		NULL),
};
static struct regulator_consumer_supply vreg_consumers_PM8901_L4[] = {
	REGULATOR_SUPPLY("8901_l4",		NULL),
};
static struct regulator_consumer_supply vreg_consumers_PM8901_L5[] = {
	REGULATOR_SUPPLY("8901_l5",		NULL),
};
static struct regulator_consumer_supply vreg_consumers_PM8901_L6[] = {
	REGULATOR_SUPPLY("8901_l6",		NULL),
};
static struct regulator_consumer_supply vreg_consumers_PM8901_S2[] = {
	REGULATOR_SUPPLY("8901_s2",		NULL),
};
static struct regulator_consumer_supply vreg_consumers_PM8901_S3[] = {
	REGULATOR_SUPPLY("8901_s3",		NULL),
};
static struct regulator_consumer_supply vreg_consumers_PM8901_S4[] = {
	REGULATOR_SUPPLY("8901_s4",		NULL),
};
static struct regulator_consumer_supply vreg_consumers_PM8901_LVS0[] = {
	REGULATOR_SUPPLY("8901_lvs0",		NULL),
};
static struct regulator_consumer_supply vreg_consumers_PM8901_LVS1[] = {
	REGULATOR_SUPPLY("8901_lvs1",		NULL),
};
static struct regulator_consumer_supply vreg_consumers_PM8901_LVS2[] = {
	REGULATOR_SUPPLY("8901_lvs2",		NULL),
};
static struct regulator_consumer_supply vreg_consumers_PM8901_LVS3[] = {
	REGULATOR_SUPPLY("8901_lvs3",		NULL),
};
static struct regulator_consumer_supply vreg_consumers_PM8901_MVS0[] = {
	REGULATOR_SUPPLY("8901_mvs0",		NULL),
};

/* Pin control regulators */
static struct regulator_consumer_supply vreg_consumers_PM8058_L8_PC[] = {
	REGULATOR_SUPPLY("8058_l8_pc",		NULL),
};
static struct regulator_consumer_supply vreg_consumers_PM8058_L20_PC[] = {
	REGULATOR_SUPPLY("8058_l20_pc",		NULL),
};
static struct regulator_consumer_supply vreg_consumers_PM8058_L21_PC[] = {
	REGULATOR_SUPPLY("8058_l21_pc",		NULL),
};
static struct regulator_consumer_supply vreg_consumers_PM8058_S2_PC[] = {
	REGULATOR_SUPPLY("8058_s2_pc",		NULL),
};
static struct regulator_consumer_supply vreg_consumers_PM8901_L0_PC[] = {
	REGULATOR_SUPPLY("8901_l0_pc",		NULL),
};
static struct regulator_consumer_supply vreg_consumers_PM8901_S4_PC[] = {
	REGULATOR_SUPPLY("8901_s4_pc",		NULL),
};

#define RPM_VREG_INIT(_id, _min_uV, _max_uV, _modes, _ops, _apply_uV, \
		      _default_uV, _peak_uA, _avg_uA, _pull_down, _pin_ctrl, \
		      _freq, _pin_fn, _force_mode, _state, _sleep_selectable, \
		      _always_on) \
	{ \
		.init_data = { \
			.constraints = { \
				.valid_modes_mask	= _modes, \
				.valid_ops_mask		= _ops, \
				.min_uV			= _min_uV, \
				.max_uV			= _max_uV, \
				.input_uV		= _min_uV, \
				.apply_uV		= _apply_uV, \
				.always_on		= _always_on, \
			}, \
			.consumer_supplies	= vreg_consumers_##_id, \
			.num_consumer_supplies	= \
				ARRAY_SIZE(vreg_consumers_##_id), \
		}, \
		.id			= RPM_VREG_ID_##_id, \
		.default_uV		= _default_uV, \
		.peak_uA		= _peak_uA, \
		.avg_uA			= _avg_uA, \
		.pull_down_enable	= _pull_down, \
		.pin_ctrl		= _pin_ctrl, \
		.freq			= RPM_VREG_FREQ_##_freq, \
		.pin_fn			= _pin_fn, \
		.force_mode		= _force_mode, \
		.state			= _state, \
		.sleep_selectable	= _sleep_selectable, \
	}

/* Pin control initialization */
#define RPM_PC(_id, _always_on, _pin_fn, _pin_ctrl) \
	{ \
		.init_data = { \
			.constraints = { \
				.valid_ops_mask	= REGULATOR_CHANGE_STATUS, \
				.always_on	= _always_on, \
			}, \
			.num_consumer_supplies	= \
					ARRAY_SIZE(vreg_consumers_##_id##_PC), \
			.consumer_supplies	= vreg_consumers_##_id##_PC, \
		}, \
		.id	  = RPM_VREG_ID_##_id##_PC, \
		.pin_fn	  = RPM_VREG_PIN_FN_8660_##_pin_fn, \
		.pin_ctrl = _pin_ctrl, \
	}

/*
 * The default LPM/HPM state of an RPM controlled regulator can be controlled
 * via the peak_uA value specified in the table below.  If the value is less
 * than the high power min threshold for the regulator, then the regulator will
 * be set to LPM.  Otherwise, it will be set to HPM.
 *
 * This value can be further overridden by specifying an initial mode via
 * .init_data.constraints.initial_mode.
 */

#define RPM_LDO(_id, _always_on, _pd, _sleep_selectable, _min_uV, _max_uV, \
		_init_peak_uA) \
	RPM_VREG_INIT(_id, _min_uV, _max_uV, REGULATOR_MODE_FAST | \
		      REGULATOR_MODE_NORMAL | REGULATOR_MODE_IDLE | \
		      REGULATOR_MODE_STANDBY, REGULATOR_CHANGE_VOLTAGE | \
		      REGULATOR_CHANGE_STATUS | REGULATOR_CHANGE_MODE | \
		      REGULATOR_CHANGE_DRMS, 0, _min_uV, _init_peak_uA, \
		      _init_peak_uA, _pd, RPM_VREG_PIN_CTRL_NONE, NONE, \
		      RPM_VREG_PIN_FN_8660_ENABLE, \
		      RPM_VREG_FORCE_MODE_8660_NONE, RPM_VREG_STATE_OFF, \
		      _sleep_selectable, _always_on)

#define RPM_SMPS(_id, _always_on, _pd, _sleep_selectable, _min_uV, _max_uV, \
		 _init_peak_uA, _freq) \
	RPM_VREG_INIT(_id, _min_uV, _max_uV, REGULATOR_MODE_FAST | \
		      REGULATOR_MODE_NORMAL | REGULATOR_MODE_IDLE | \
		      REGULATOR_MODE_STANDBY, REGULATOR_CHANGE_VOLTAGE | \
		      REGULATOR_CHANGE_STATUS | REGULATOR_CHANGE_MODE | \
		      REGULATOR_CHANGE_DRMS, 0, _min_uV, _init_peak_uA, \
		      _init_peak_uA, _pd, RPM_VREG_PIN_CTRL_NONE, _freq, \
		      RPM_VREG_PIN_FN_8660_ENABLE, \
		      RPM_VREG_FORCE_MODE_8660_NONE, RPM_VREG_STATE_OFF, \
		      _sleep_selectable, _always_on)

#define RPM_VS(_id, _always_on, _pd, _sleep_selectable) \
	RPM_VREG_INIT(_id, 0, 0, REGULATOR_MODE_NORMAL | REGULATOR_MODE_IDLE, \
		      REGULATOR_CHANGE_STATUS | REGULATOR_CHANGE_MODE, 0, 0, \
		      1000, 1000, _pd, RPM_VREG_PIN_CTRL_NONE, NONE, \
		      RPM_VREG_PIN_FN_8660_ENABLE, \
		      RPM_VREG_FORCE_MODE_8660_NONE, RPM_VREG_STATE_OFF, \
		      _sleep_selectable, _always_on)

#define RPM_NCP(_id, _always_on, _pd, _sleep_selectable, _min_uV, _max_uV) \
	RPM_VREG_INIT(_id, _min_uV, _max_uV, REGULATOR_MODE_NORMAL, \
		      REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS, 0, \
		      _min_uV, 1000, 1000, _pd, RPM_VREG_PIN_CTRL_NONE, NONE, \
		      RPM_VREG_PIN_FN_8660_ENABLE, \
		      RPM_VREG_FORCE_MODE_8660_NONE, RPM_VREG_STATE_OFF, \
		      _sleep_selectable, _always_on)

#define LDO50HMIN	RPM_VREG_8660_LDO_50_HPM_MIN_LOAD
#define LDO150HMIN	RPM_VREG_8660_LDO_150_HPM_MIN_LOAD
#define LDO300HMIN	RPM_VREG_8660_LDO_300_HPM_MIN_LOAD
#define SMPS_HMIN	RPM_VREG_8660_SMPS_HPM_MIN_LOAD
#define FTS_HMIN	RPM_VREG_8660_FTSMPS_HPM_MIN_LOAD

/* RPM early regulator constraints */
static struct rpm_regulator_init_data rpm_regulator_early_init_data[] = {
	/*	 ID       a_on pd ss min_uV   max_uV   init_ip    freq */
	RPM_SMPS(PM8058_S0, 0, 1, 1,  500000, 1250000, SMPS_HMIN, 1p60),
	RPM_SMPS(PM8058_S1, 0, 1, 1,  500000, 1250000, SMPS_HMIN, 1p60),
};

/* RPM regulator constraints */
static struct rpm_regulator_init_data rpm_regulator_init_data[] = {
	/*	ID        a_on pd ss min_uV   max_uV   init_ip */
	RPM_LDO(PM8058_L0,  0, 1, 0, 1200000, 1200000, LDO150HMIN),
	RPM_LDO(PM8058_L1,  0, 1, 0, 1200000, 1200000, LDO300HMIN),
	RPM_LDO(PM8058_L2,  0, 1, 0, 1800000, 2600000, LDO300HMIN),
	RPM_LDO(PM8058_L3,  0, 1, 0, 1800000, 1800000, LDO150HMIN),
	RPM_LDO(PM8058_L4,  0, 1, 0, 2850000, 2850000,  LDO50HMIN),
	RPM_LDO(PM8058_L5,  0, 1, 0, 2850000, 2850000, LDO300HMIN),
	RPM_LDO(PM8058_L6,  0, 1, 0, 3000000, 3600000,  LDO50HMIN),
	RPM_LDO(PM8058_L7,  0, 1, 0, 1800000, 1800000,  LDO50HMIN),
	RPM_LDO(PM8058_L8,  0, 1, 0, 2900000, 3050000, LDO300HMIN),
	RPM_LDO(PM8058_L9,  0, 1, 0, 1800000, 1800000, LDO300HMIN),
	RPM_LDO(PM8058_L10, 0, 1, 0, 2600000, 2600000, LDO300HMIN),
	RPM_LDO(PM8058_L11, 0, 1, 0, 1500000, 1500000, LDO150HMIN),
	RPM_LDO(PM8058_L12, 0, 1, 0, 2900000, 2900000, LDO150HMIN),
	RPM_LDO(PM8058_L13, 0, 1, 0, 2050000, 2050000, LDO300HMIN),
	RPM_LDO(PM8058_L14, 0, 0, 0, 2850000, 2850000, LDO300HMIN),
	RPM_LDO(PM8058_L15, 0, 1, 0, 2850000, 2850000, LDO300HMIN),
	RPM_LDO(PM8058_L16, 1, 1, 0, 1800000, 1800000, LDO300HMIN),
	RPM_LDO(PM8058_L17, 0, 1, 0, 2600000, 2600000, LDO150HMIN),
	RPM_LDO(PM8058_L18, 0, 1, 0, 2200000, 2200000, LDO150HMIN),
	RPM_LDO(PM8058_L19, 0, 1, 0, 2500000, 2500000, LDO150HMIN),
	RPM_LDO(PM8058_L20, 0, 1, 0, 1800000, 1800000, LDO150HMIN),
	RPM_LDO(PM8058_L21, 1, 1, 0, 1200000, 1200000, LDO150HMIN),
	RPM_LDO(PM8058_L22, 0, 1, 0, 1150000, 1150000, LDO300HMIN),
	RPM_LDO(PM8058_L23, 0, 1, 0, 1200000, 1200000, LDO300HMIN),
	RPM_LDO(PM8058_L24, 0, 1, 0, 1200000, 1200000, LDO150HMIN),
	RPM_LDO(PM8058_L25, 0, 1, 0, 1200000, 1200000, LDO150HMIN),

	/*	 ID       a_on pd ss min_uV   max_uV   init_ip    freq */
	RPM_SMPS(PM8058_S2, 0, 1, 1, 1200000, 1400000, SMPS_HMIN, 1p60),
	RPM_SMPS(PM8058_S3, 1, 1, 0, 1800000, 1800000, SMPS_HMIN, 1p60),
	RPM_SMPS(PM8058_S4, 1, 1, 0, 2200000, 2200000, SMPS_HMIN, 1p60),

	/*     ID         a_on pd ss */
	RPM_VS(PM8058_LVS0, 0, 1, 0),
	RPM_VS(PM8058_LVS1, 0, 1, 0),

	/*	ID        a_on pd ss min_uV   max_uV */
	RPM_NCP(PM8058_NCP, 0, 1, 0, 1800000, 1800000),

	/*	ID        a_on pd ss min_uV   max_uV   init_ip */
	RPM_LDO(PM8901_L0,  0, 1, 0, 1200000, 1200000, LDO300HMIN),
	RPM_LDO(PM8901_L1,  0, 1, 0, 3300000, 3300000, LDO300HMIN),
	RPM_LDO(PM8901_L2,  0, 1, 0, 2850000, 3300000, LDO300HMIN),
	RPM_LDO(PM8901_L3,  0, 1, 0, 3300000, 3300000, LDO300HMIN),
	RPM_LDO(PM8901_L4,  0, 1, 0, 2600000, 2600000, LDO300HMIN),
	RPM_LDO(PM8901_L5,  0, 1, 0, 2850000, 2850000, LDO300HMIN),
	RPM_LDO(PM8901_L6,  0, 1, 0, 2200000, 2200000, LDO300HMIN),

	/*	 ID       a_on pd ss min_uV   max_uV   init_ip   freq */
	RPM_SMPS(PM8901_S2, 0, 1, 0, 1300000, 1300000, FTS_HMIN, 1p60),
	RPM_SMPS(PM8901_S3, 0, 1, 0, 1100000, 1100000, FTS_HMIN, 1p60),
	RPM_SMPS(PM8901_S4, 0, 1, 0, 1225000, 1225000, FTS_HMIN, 1p60),

	/*	ID        a_on pd ss */
	RPM_VS(PM8901_LVS0, 1, 1, 0),
	RPM_VS(PM8901_LVS1, 0, 1, 0),
	RPM_VS(PM8901_LVS2, 0, 1, 0),
	RPM_VS(PM8901_LVS3, 0, 1, 0),
	RPM_VS(PM8901_MVS0, 0, 1, 0),

	/*     ID         a_on pin_func pin_ctrl */
	RPM_PC(PM8058_L8,   0, SLEEP_B, RPM_VREG_PIN_CTRL_NONE),
	RPM_PC(PM8058_L20,  0, SLEEP_B, RPM_VREG_PIN_CTRL_NONE),
	RPM_PC(PM8058_L21,  1, SLEEP_B, RPM_VREG_PIN_CTRL_NONE),
	RPM_PC(PM8058_S2,   0, ENABLE,  RPM_VREG_PIN_CTRL_PM8058_A0),
	RPM_PC(PM8901_L0,   0, ENABLE,  RPM_VREG_PIN_CTRL_PM8901_A0),
	RPM_PC(PM8901_S4,   0, ENABLE,  RPM_VREG_PIN_CTRL_PM8901_A0),
};

static struct rpm_regulator_platform_data rpm_regulator_early_pdata = {
	.init_data		= rpm_regulator_early_init_data,
	.num_regulators		= ARRAY_SIZE(rpm_regulator_early_init_data),
	.version		= RPM_VREG_VERSION_8660,
	.vreg_id_vdd_mem	= RPM_VREG_ID_PM8058_S0,
	.vreg_id_vdd_dig	= RPM_VREG_ID_PM8058_S1,
};

static struct rpm_regulator_platform_data rpm_regulator_pdata = {
	.init_data		= rpm_regulator_init_data,
	.num_regulators		= ARRAY_SIZE(rpm_regulator_init_data),
	.version		= RPM_VREG_VERSION_8660,
};

static struct platform_device rpm_regulator_early_device = {
	.name	= "rpm-regulator",
	.id	= 0,
	.dev	= {
		.platform_data = &rpm_regulator_early_pdata,
	},
};

static struct platform_device rpm_regulator_device = {
	.name	= "rpm-regulator",
	.id	= 1,
	.dev	= {
		.platform_data = &rpm_regulator_pdata,
	},
};

static struct platform_device *early_regulators[] __initdata = {
	&msm_device_saw_s0,
	&msm_device_saw_s1,
	&rpm_regulator_early_device,
};

static struct platform_device *devices[] __initdata = {
	&ram_console_device,
	&rpm_regulator_device,

};

static void __init msm8x60_map_io(void)
{
	msm_shared_ram_phys = MSM_SHARED_RAM_PHYS;
	msm_map_msm8x60_io();

	if (socinfo_init() < 0)
		printk(KERN_ERR "%s: socinfo_init() failed!",   __func__);
}

static void __init msm8x60_init(void)
{
	pmic_reset_irq = PM8058_IRQ_BASE + PM8058_RESOUT_IRQ;

	/*
	 * Initialize RPM first as other drivers and devices may need
	 * it for their initialization.
	 */
	BUG_ON(msm_rpm_init(&msm8660_rpm_data));
	BUG_ON(msm_rpmrs_levels_init(&msm_rpmrs_data));
	if (msm_xo_init())
		pr_err("Failed to initialize XO votes\n");

	msm8x60_check_2d_hardware();

	/*
	 * Initialize SPM before acpuclock as the latter calls into SPM
	 * driver to set ACPU voltages.
	 */
	if (SOCINFO_VERSION_MAJOR(socinfo_get_version()) != 1)
		msm_spm_init(msm_spm_data, ARRAY_SIZE(msm_spm_data));
	else
		msm_spm_init(msm_spm_data_v1, ARRAY_SIZE(msm_spm_data_v1));

	/*
	 * Disable regulator info printing so that regulator registration
	 * messages do not enter the kmsg log.
	 */
	regulator_suppress_info_printing();

	/* Initialize regulators needed for clock_init. */
	platform_add_devices(early_regulators, ARRAY_SIZE(early_regulators));

	msm_clock_init(&msm8x60_clock_init_data);

	platform_add_devices(devices, ARRAY_SIZE(devices));
}

static void __init shooter_fixup(struct machine_desc *desc, struct tag *tags,
				char **cmdline, struct meminfo *mi)
{
	mem_size_mb = parse_tag_memsize((const struct tag *)tags);
	printk(KERN_DEBUG "%s: mem_size_mb=%u\n", __func__, mem_size_mb);
	engineerid = parse_tag_engineerid(tags);
	mi->nr_banks = 1;
	mi->bank[0].start = CONFIG_PHYS_OFFSET; //0x48000000
	mi->bank[0].size = 0x23800000;
	if (mem_size_mb == 1024)
		mi->bank[0].size += 0x10000000;
}

MACHINE_START(SHOOTER, "HTC Evo 3D CDMA")
	.fixup = shooter_fixup,
	.map_io = msm8x60_map_io,
	.init_irq = msm8x60_init_irq,
	.handle_irq = gic_handle_irq,
	.init_machine = msm8x60_init,
	.timer = &msm_timer,
//	.init_early = msm8x60_init_early,
MACHINE_END

MACHINE_START(SHOOTER_U, "HTC Evo 3D GSM")
	.fixup = shooter_fixup,
	.map_io = msm8x60_map_io,
	.init_irq = msm8x60_init_irq,
	.handle_irq = gic_handle_irq,
	.init_machine = msm8x60_init,
	.timer = &msm_timer,
//	.init_early = msm8x60_init_early,
MACHINE_END
