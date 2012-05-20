#include <linux/gpio.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/mfd/pmic8058.h>
#include <linux/mfd/pmic8901.h>

#ifdef CONFIG_ANDROID_PMEM
#include <linux/android_pmem.h>
#endif

#include <linux/atmel_qt602240.h>
#include <linux/bootmem.h>
#include <linux/dma-mapping.h>
#include <linux/i2c.h>
#include <linux/isl29028.h>
#include <linux/isl29029.h>
#include <linux/leds-pm8058.h>
#include <linux/mpu.h>
#include <linux/msm_adc.h>
#include <linux/msm-charger.h>
#include <linux/m_adcproc.h>
#include <linux/proc_fs.h>
#include <linux/tps65200.h>
#include <linux/spi/spi.h>

#ifdef CONFIG_USB_G_ANDROID
#include <linux/usb/android.h>
#include <linux/usb/msm_hsusb.h>
#include <mach/usbdiag.h>
#endif

#include <asm/mach/arch.h>
#include <asm/mach/mmc.h>
#include <asm/mach-types.h>
#include <asm/hardware/gic.h>

#include <mach/board.h>
#include <mach/board_htc.h>
#include <mach/board-msm8660.h>
#include <mach/cable_detect.h>
#include <mach/cpuidle.h>
#include <mach/gpiomux.h>
#include <mach/htc_battery_core.h>
#include <mach/htc_battery_8x60.h>
#include <mach/htc_bdaddress.h>
#include <mach/htc_headset_8x60.h>
#include <mach/htc_headset_gpio.h>
#include <mach/htc_headset_mgr.h>
#include <mach/htc_headset_pmic.h>
#include <mach/mpp.h>
#include <mach/msm_bus_board.h>
#include <mach/msm_dsps.h>
#include <mach/msm_iomap.h>
#include <mach/msm_memtypes.h>
#include <mach/msm_serial_hs.h>
#include <mach/msm_spi.h>
#include <mach/msm_xo.h>
#include <mach/restart.h>
#include <mach/rpm.h>
#include <mach/rpm-regulator.h>
#include <mach/socinfo.h>
#include <mach/tpa2051d3.h>

#include "acpuclock.h"
#include "devices.h"
#include "devices-msm8x60.h"
#include "gpiomux-8x60.h"
#include "pm-boot.h"
#include "pm.h"
#include "rpm_log.h"
#include "rpm_resources.h"
#include "rpm_stats.h"
#include "spm.h"
#include "timer.h"
#include "board-shooter.h"

#include <linux/ion.h>
#include <mach/ion.h>

#ifdef CONFIG_ION_MSM
static struct platform_device ion_dev;
#endif

struct pm8xxx_mpp_init_info {
	unsigned			mpp;
	struct pm8xxx_mpp_config_data	config;
};

#define PM8058_MPP_INIT(_mpp, _type, _level, _control) \
{ \
	.mpp	= PM8058_MPP_PM_TO_SYS(_mpp), \
	.config	= { \
		.type		= PM8XXX_MPP_TYPE_##_type, \
		.level		= _level, \
		.control	= PM8XXX_MPP_##_control, \
	} \
}

#define PM8901_MPP_INIT(_mpp, _type, _level, _control) \
{ \
	.mpp	= PM8901_MPP_PM_TO_SYS(_mpp), \
	.config	= { \
		.type		= PM8XXX_MPP_TYPE_##_type, \
		.level		= _level, \
		.control	= PM8XXX_MPP_##_control, \
	} \
}

unsigned engineerid, mem_size_mb;

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

static struct platform_device msm_tsens_device = {
	.name   = "tsens-tm",
	.id = -1,
};

#ifdef CONFIG_MSM_VPE
static struct resource msm_vpe_resources[] = {
	{
		.start	= 0x05300000,
		.end	= 0x05300000 + SZ_1M - 1,
		.flags	= IORESOURCE_MEM,
	},
	{
		.start	= INT_VPE,
		.end	= INT_VPE,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device msm_vpe_device = {
	.name = "msm_vpe",
	.id   = 0,
	.num_resources = ARRAY_SIZE(msm_vpe_resources),
	.resource = msm_vpe_resources,
};
#endif

#ifdef CONFIG_MSM_GEMINI
static struct resource msm_gemini_resources[] = {
	{
		.start  = 0x04600000,
		.end    = 0x04600000 + SZ_1M - 1,
		.flags  = IORESOURCE_MEM,
	},
	{
		.start  = INT_JPEG,
		.end    = INT_JPEG,
		.flags  = IORESOURCE_IRQ,
	},
};

static struct platform_device msm_gemini_device = {
	.name           = "msm_gemini",
	.resource       = msm_gemini_resources,
	.num_resources  = ARRAY_SIZE(msm_gemini_resources),
};
#endif

#ifdef CONFIG_BT
static struct platform_device shooter_rfkill = {
	.name = "shooter_rfkill",
	.id = -1,
};
#endif

#if defined(CONFIG_MSM_RPM_LOG) || defined(CONFIG_MSM_RPM_LOG_MODULE)
static struct msm_rpm_log_platform_data msm_rpm_log_pdata = {
	.phys_addr_base = 0x00106000,
	.reg_offsets = {
		[MSM_RPM_LOG_PAGE_INDICES] = 0x00000C80,
		[MSM_RPM_LOG_PAGE_BUFFER]  = 0x00000CA0,
	},
	.phys_size = SZ_8K,
	.log_len = 4096,                  /* log's buffer length in bytes */
	.log_len_mask = (4096 >> 2) - 1,  /* length mask in units of u32 */
};

static struct platform_device msm_rpm_log_device = {
	.name   = "msm_rpm_log",
	.id     = -1,
	.dev    = {
		.platform_data = &msm_rpm_log_pdata,
	},
};
#endif


#ifdef CONFIG_HTC_BATT8x60
static struct htc_battery_platform_data htc_battery_pdev_data = {
	.guage_driver = GUAGE_NONE,
	.gpio_mbat_in = MSM_GPIO_TO_INT(SHOOTER_GPIO_MBAT_IN),
#ifdef CONFIG_MACH_SHOOTER
	.gpio_mbat_in_trigger_level = MBAT_IN_HIGH_TRIGGER,
#else
	.gpio_mbat_in_trigger_level = MBAT_IN_LOW_TRIGGER,
#endif
	.charger = SWITCH_CHARGER_TPS65200,
	.mpp_data = {
		{PM8058_MPP_PM_TO_SYS(XOADC_MPP_3), PM8XXX_MPP_AIN_AMUX_CH6},
		{PM8058_MPP_PM_TO_SYS(XOADC_MPP_5), PM8XXX_MPP_AIN_AMUX_CH6},
		{PM8058_MPP_PM_TO_SYS(XOADC_MPP_7), PM8XXX_MPP_AIN_AMUX_CH6},
		{PM8058_MPP_PM_TO_SYS(XOADC_MPP_8), PM8XXX_MPP_AIN_AMUX_CH6},
	}
};

static struct platform_device shooter_3Dpanel_device = {
	.name = "panel_3d",
	.id = -1,
};

static struct platform_device htc_battery_pdev = {
	.name = "htc_battery",
	.id = -1,
	.dev    = {
		.platform_data = &htc_battery_pdev_data,
	},
};
#endif

static unsigned fb_size;
static int __init fb_size_setup(char *p)
{
	fb_size = memparse(p, NULL);
	return 0;
}
early_param("fb_size", fb_size_setup);

static unsigned pmem_kernel_ebi1_size = MSM_PMEM_KERNEL_EBI1_SIZE;
static int __init pmem_kernel_ebi1_size_setup(char *p)
{
	pmem_kernel_ebi1_size = memparse(p, NULL);
	return 0;
}
early_param("pmem_kernel_ebi1_size", pmem_kernel_ebi1_size_setup);

#ifdef CONFIG_ANDROID_PMEM
static unsigned pmem_sf_size = MSM_PMEM_SF_SIZE;
static int __init pmem_sf_size_setup(char *p)
{
	pmem_sf_size = memparse(p, NULL);
	return 0;
}
early_param("pmem_sf_size", pmem_sf_size_setup);

static unsigned pmem_adsp_size = MSM_PMEM_ADSP_SIZE;

static int __init pmem_adsp_size_setup(char *p)
{
	pmem_adsp_size = memparse(p, NULL);
	return 0;
}
early_param("pmem_adsp_size", pmem_adsp_size_setup);

static unsigned pmem_audio_size = MSM_PMEM_AUDIO_SIZE;

static int __init pmem_audio_size_setup(char *p)
{
	pmem_audio_size = memparse(p, NULL);
	return 0;
}
early_param("pmem_audio_size", pmem_audio_size_setup);
#endif

#ifdef CONFIG_ANDROID_PMEM
#ifndef CONFIG_MSM_MULTIMEDIA_USE_ION
static struct android_pmem_platform_data android_pmem_pdata = {
	.name = "pmem",
	.allocator_type = PMEM_ALLOCATORTYPE_ALLORNOTHING,
	.cached = 1,
	.memory_type = MEMTYPE_EBI1,
};

static struct platform_device android_pmem_device = {
	.name = "android_pmem",
	.id = 0,
	.dev = {.platform_data = &android_pmem_pdata},
};

static struct android_pmem_platform_data android_pmem_adsp_pdata = {
	.name = "pmem_adsp",
	.allocator_type = PMEM_ALLOCATORTYPE_BITMAP,
	.cached = 0,
	.memory_type = MEMTYPE_EBI1,
};

static struct platform_device android_pmem_adsp_device = {
	.name = "android_pmem",
	.id = 2,
	.dev = { .platform_data = &android_pmem_adsp_pdata },
};
#endif
static struct android_pmem_platform_data android_pmem_audio_pdata = {
	.name = "pmem_audio",
	.allocator_type = PMEM_ALLOCATORTYPE_BITMAP,
	.cached = 0,
	.memory_type = MEMTYPE_EBI1,
};

static struct platform_device android_pmem_audio_device = {
	.name = "android_pmem",
	.id = 4,
	.dev = { .platform_data = &android_pmem_audio_pdata },
};

#define PMEM_BUS_WIDTH(_bw) \
	{ \
		.vectors = &(struct msm_bus_vectors){ \
			.src = MSM_BUS_MASTER_AMPSS_M0, \
			.dst = MSM_BUS_SLAVE_SMI, \
			.ib = (_bw), \
			.ab = 0, \
		}, \
	.num_paths = 1, \
	}

static struct msm_bus_paths mem_smi_table[] = {
	[0] = PMEM_BUS_WIDTH(0), /* Off */
	[1] = PMEM_BUS_WIDTH(1), /* On */
};

static struct msm_bus_scale_pdata smi_client_pdata = {
	.usecase = mem_smi_table,
	.num_usecases = ARRAY_SIZE(mem_smi_table),
	.name = "mem_smi",
};

int request_smi_region(void *data)
{
	int bus_id = (int) data;

	msm_bus_scale_client_update_request(bus_id, 1);
	return 0;
}

int release_smi_region(void *data)
{
	int bus_id = (int) data;

	msm_bus_scale_client_update_request(bus_id, 0);
	return 0;
}

void *setup_smi_region(void)
{
	return (void *)msm_bus_scale_register_client(&smi_client_pdata);
}
#ifndef CONFIG_MSM_MULTIMEDIA_USE_ION
static struct android_pmem_platform_data android_pmem_smipool_pdata = {
	.name = "pmem_smipool",
	.allocator_type = PMEM_ALLOCATORTYPE_BITMAP,
	.cached = 0,
	.memory_type = MEMTYPE_SMI,
	.request_region = request_smi_region,
	.release_region = release_smi_region,
	.setup_region = setup_smi_region,
	.map_on_demand = 1,
};
static struct platform_device android_pmem_smipool_device = {
	.name = "android_pmem",
	.id = 7,
	.dev = { .platform_data = &android_pmem_smipool_pdata },
};
#endif
#endif

static struct msm_pm_platform_data msm_pm_data[MSM_PM_SLEEP_MODE_NR * 2] = {
	[MSM_PM_MODE(0, MSM_PM_SLEEP_MODE_POWER_COLLAPSE)] = {
		.idle_supported = 1,
		.suspend_supported = 1,
		.idle_enabled = 1,
		.suspend_enabled = 1,
		.latency = 4000,
		.residency = 13000,
	},

	[MSM_PM_MODE(0, MSM_PM_SLEEP_MODE_POWER_COLLAPSE_STANDALONE)] = {
		.idle_supported = 1,
		.suspend_supported = 1,
		.idle_enabled = 1,
		.suspend_enabled = 1,
		.latency = 500,
		.residency = 6000,
	},

	[MSM_PM_MODE(0, MSM_PM_SLEEP_MODE_WAIT_FOR_INTERRUPT)] = {
		.idle_supported = 1,
		.suspend_supported = 1,
		.idle_enabled = 1,
		.suspend_enabled = 1,
		.latency = 2,
		.residency = 0,
	},

	[MSM_PM_MODE(1, MSM_PM_SLEEP_MODE_POWER_COLLAPSE)] = {
		.idle_supported = 1,
		.suspend_supported = 1,
		.idle_enabled = 1,
		.suspend_enabled = 1,
		.latency = 600,
		.residency = 7200,
	},

	[MSM_PM_MODE(1, MSM_PM_SLEEP_MODE_POWER_COLLAPSE_STANDALONE)] = {
		.idle_supported = 1,
		.suspend_supported = 1,
		.idle_enabled = 1,
		.suspend_enabled = 1,
		.latency = 500,
		.residency = 6000,
	},

	[MSM_PM_MODE(1, MSM_PM_SLEEP_MODE_WAIT_FOR_INTERRUPT)] = {
		.idle_supported = 1,
		.suspend_supported = 1,
		.idle_enabled = 1,
		.suspend_enabled = 1,
		.latency = 2,
		.residency = 0,
	},
};

static struct msm_cpuidle_state msm_cstates[] __initdata = {
	{0, 0, "C0", "WFI",
		MSM_PM_SLEEP_MODE_WAIT_FOR_INTERRUPT},

	{0, 1, "C1", "STANDALONE_POWER_COLLAPSE",
		MSM_PM_SLEEP_MODE_POWER_COLLAPSE_STANDALONE},

	{0, 2, "C2", "POWER_COLLAPSE",
		MSM_PM_SLEEP_MODE_POWER_COLLAPSE},

	{1, 0, "C0", "WFI",
		MSM_PM_SLEEP_MODE_WAIT_FOR_INTERRUPT},

	{1, 1, "C1", "STANDALONE_POWER_COLLAPSE",
		MSM_PM_SLEEP_MODE_POWER_COLLAPSE_STANDALONE},
};

static void msm_hsusb_vbus_power(bool on)
{
	static struct regulator *votg_5v_switch;
	static struct regulator *ext_5v_reg;
	static int vbus_is_on;

	/* If VBUS is already on (or off), do nothing. */
	if (on == vbus_is_on)
		return;

	if (!votg_5v_switch) {
		votg_5v_switch = regulator_get(NULL, "8901_usb_otg");
		if (IS_ERR(votg_5v_switch)) {
			pr_err("%s: unable to get votg_5v_switch\n", __func__);
			return;
		}
	}
	if (!ext_5v_reg) {
		ext_5v_reg = regulator_get(NULL, "8901_mpp0");
		if (IS_ERR(ext_5v_reg)) {
			pr_err("%s: unable to get ext_5v_reg\n", __func__);
			return;
		}
	}
	if (on) {
		if (regulator_enable(ext_5v_reg)) {
			pr_err("%s: Unable to enable the regulator:"
					" ext_5v_reg\n", __func__);
			return;
		}
		if (regulator_enable(votg_5v_switch)) {
			pr_err("%s: Unable to enable the regulator:"
					" votg_5v_switch\n", __func__);
			return;
		}
	} else {
		if (regulator_disable(votg_5v_switch))
			pr_err("%s: Unable to enable the regulator:"
				" votg_5v_switch\n", __func__);
		if (regulator_disable(ext_5v_reg))
			pr_err("%s: Unable to enable the regulator:"
				" ext_5v_reg\n", __func__);
	}

	vbus_is_on = on;
}

static int phy_init_seq[] = { 0x06, 0x36, 0x0C, 0x31, 0x31, 0x32, 0x1, 0x0E, 0x1, 0x11, -1 };
static struct msm_otg_platform_data msm_otg_pdata = {
	.phy_init_seq 	= phy_init_seq,
	.vbus_power	= msm_hsusb_vbus_power,
	.power_budget	= 750,
	.mode		= USB_PERIPHERAL,
	.otg_control	= OTG_PMIC_CONTROL,
	.phy_type	= CI_45NM_INTEGRATED_PHY,
	 /* TODO: find shooter regulators */
	.ldo_3v3_name	= "8058_l6",
	.ldo_1v8_name	= "8058_l7",
	.vddcx_name	= "8058_s1",
};

#ifdef CONFIG_USB_G_ANDROID

#define PID_MAGIC_ID		0x71432909
#define SERIAL_NUM_MAGIC_ID	0x61945374
#define SERIAL_NUMBER_LENGTH	127
#define DLOAD_USB_BASE_ADD	0x2A05F0C8

struct magic_num_struct {
	uint32_t pid;
	uint32_t serial_num;
};

struct dload_struct {
	uint32_t	reserved1;
	uint32_t	reserved2;
	uint32_t	reserved3;
	uint16_t	reserved4;
	uint16_t	pid;
	char		serial_number[SERIAL_NUMBER_LENGTH];
	uint16_t	reserved5;
	struct magic_num_struct
			magic_struct;
};

static int usb_diag_update_pid_and_serial_num(uint32_t pid, const char *snum)
{
	struct dload_struct __iomem *dload = 0;

	dload = ioremap(DLOAD_USB_BASE_ADD, sizeof(*dload));
	if (!dload) {
		pr_err("%s: cannot remap I/O memory region: %08x\n",
					__func__, DLOAD_USB_BASE_ADD);
		return -ENXIO;
	}

	pr_debug("%s: dload:%p pid:%x serial_num:%s\n",
				__func__, dload, pid, snum);
	/* update pid */
	dload->magic_struct.pid = PID_MAGIC_ID;
	dload->pid = pid;

	/* update serial number */
	dload->magic_struct.serial_num = 0;
	if (!snum)
		return 0;

	dload->magic_struct.serial_num = SERIAL_NUM_MAGIC_ID;
	strncpy(dload->serial_number, snum, SERIAL_NUMBER_LENGTH);
	dload->serial_number[SERIAL_NUMBER_LENGTH - 1] = '\0';

	iounmap(dload);

	return 0;
}

static struct android_usb_platform_data android_usb_pdata = {
	.update_pid_and_serial_num = usb_diag_update_pid_and_serial_num,
};

static struct platform_device android_usb_device = {
	.name	= "android_usb",
	.id	= -1,
	.dev	= {
		.platform_data = &android_usb_pdata,
	},
};
#endif

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

static struct msm_pm_boot_platform_data msm_pm_boot_pdata __initdata = {
	.mode = MSM_PM_BOOT_CONFIG_TZ,
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

		.reg_init_values[MSM_SPM_REG_SAW_SLP_CLK_EN] = 0x11,
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

		.reg_init_values[MSM_SPM_REG_SAW_SLP_CLK_EN] = 0x11,
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

#ifdef CONFIG_PMIC8901
#define PM8901_GPIO_INT           91
/*
 * Consumer specific regulator names:
 *			 regulator name		consumer dev_name
 */
static struct regulator_consumer_supply vreg_consumers_8901_USB_OTG[] = {
	REGULATOR_SUPPLY("8901_usb_otg",	NULL),
};
static struct regulator_consumer_supply vreg_consumers_8901_HDMI_MVS[] = {
	REGULATOR_SUPPLY("8901_hdmi_mvs",	NULL),
};

#define PM8901_VREG_INIT(_id, _min_uV, _max_uV, _modes, _ops, _apply_uV, \
			 _always_on) \
	{ \
		.init_data = { \
			.constraints = { \
				.valid_modes_mask = _modes, \
				.valid_ops_mask = _ops, \
				.min_uV = _min_uV, \
				.max_uV = _max_uV, \
				.input_uV = _min_uV, \
				.apply_uV = _apply_uV, \
				.always_on = _always_on, \
			}, \
			.consumer_supplies = vreg_consumers_8901_##_id, \
			.num_consumer_supplies = \
				ARRAY_SIZE(vreg_consumers_8901_##_id), \
		}, \
		.id = PM8901_VREG_ID_##_id, \
	}

#define PM8901_VREG_INIT_VS(_id) \
	PM8901_VREG_INIT(_id, 0, 0, REGULATOR_MODE_NORMAL, \
			REGULATOR_CHANGE_STATUS, 0, 0)

static struct pm8901_vreg_pdata pm8901_vreg_init[] = {
	PM8901_VREG_INIT_VS(USB_OTG),
	PM8901_VREG_INIT_VS(HDMI_MVS),
};

static struct pm8xxx_misc_platform_data pm8901_misc_pdata = {
	.priority		= 1,
};

static struct pm8xxx_irq_platform_data pm8901_irq_pdata = {
	.irq_base		= PM8901_IRQ_BASE,
	.devirq			= MSM_GPIO_TO_INT(PM8901_GPIO_INT),
	.irq_trigger_flag	= IRQF_TRIGGER_LOW,
};

static struct pm8xxx_mpp_platform_data pm8901_mpp_pdata = {
	.mpp_base		= PM8901_MPP_PM_TO_SYS(0),
};

static struct pm8901_platform_data pm8901_platform_data = {
	.irq_pdata		= &pm8901_irq_pdata,
	.mpp_pdata		= &pm8901_mpp_pdata,
	.regulator_pdatas	= pm8901_vreg_init,
	.num_regulators		= ARRAY_SIZE(pm8901_vreg_init),
	.misc_pdata		= &pm8901_misc_pdata,
};

static struct msm_ssbi_platform_data msm8x60_ssbi_pm8901_pdata __devinitdata = {
	.controller_type = MSM_SBI_CTRL_PMIC_ARBITER,
	.slave	= {
		.name = "pm8901-core",
		.platform_data = &pm8901_platform_data,
	},
};
#endif /* CONFIG_PMIC8901 */

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
			.max_uV = 1325000,
		},
		.consumer_supplies = vreg_consumers_8901_S0,
		.num_consumer_supplies = ARRAY_SIZE(vreg_consumers_8901_S0),
};

static struct regulator_init_data saw_s1_init_data = {
		.constraints = {
			.name = "8901_s1",
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE,
			.min_uV = 800000,
			.max_uV = 1325000,
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
	RPM_SMPS(PM8058_S0, 0, 1, 1,  500000, 1325000, SMPS_HMIN, 1p92),
	RPM_SMPS(PM8058_S1, 0, 1, 1,  500000, 1250000, SMPS_HMIN, 1p92),
};

/* RPM regulator constraints */
static struct rpm_regulator_init_data rpm_regulator_init_data[] = {
	/*	ID        a_on pd ss min_uV   max_uV   init_ip */
	RPM_LDO(PM8058_L0,  0, 1, 0, 1200000, 1200000, LDO150HMIN),
	RPM_LDO(PM8058_L1,  0, 1, 0, 1350000, 1350000, LDO300HMIN),
	RPM_LDO(PM8058_L2,  0, 1, 0, 1800000, 2600000, LDO300HMIN),
	RPM_LDO(PM8058_L3,  0, 1, 0, 1800000, 1800000, LDO150HMIN),
	RPM_LDO(PM8058_L4,  0, 1, 0, 2850000, 2850000,  LDO50HMIN),
	RPM_LDO(PM8058_L5,  0, 1, 0, 2850000, 2850000, LDO300HMIN),
	RPM_LDO(PM8058_L6,  0, 1, 0, 3000000, 3600000,  LDO50HMIN),
	RPM_LDO(PM8058_L7,  0, 1, 0, 1800000, 1800000,  LDO50HMIN),
	RPM_LDO(PM8058_L8,  0, 1, 0, 1800000, 1800000, LDO300HMIN),
	RPM_LDO(PM8058_L9,  0, 1, 0, 1800000, 1800000, LDO300HMIN),
	RPM_LDO(PM8058_L10, 0, 1, 0, 2800000, 2800000, LDO300HMIN),
	RPM_LDO(PM8058_L11, 0, 1, 0, 1800000, 1800000, LDO150HMIN),
	RPM_LDO(PM8058_L12, 0, 1, 0, 3000000, 3200000, LDO150HMIN),
	RPM_LDO(PM8058_L13, 0, 1, 0, 2050000, 2050000, LDO300HMIN),
	RPM_LDO(PM8058_L14, 0, 1, 0, 2850000, 2850000, LDO300HMIN),
	RPM_LDO(PM8058_L15, 0, 1, 0, 2800000, 2800000, LDO300HMIN),
	RPM_LDO(PM8058_L16, 1, 1, 1, 1800000, 1800000, LDO300HMIN),
	RPM_LDO(PM8058_L17, 0, 1, 0, 2600000, 2600000, LDO150HMIN),
	RPM_LDO(PM8058_L18, 0, 1, 1, 2200000, 2200000, LDO150HMIN),
	RPM_LDO(PM8058_L19, 0, 1, 0, 1800000, 1800000, LDO150HMIN),
	RPM_LDO(PM8058_L20, 0, 1, 0, 1800000, 1800000, LDO150HMIN),
	RPM_LDO(PM8058_L21, 1, 1, 0, 1200000, 1200000, LDO150HMIN),
	RPM_LDO(PM8058_L22, 0, 0, 0, 1200000, 1200000, LDO300HMIN),
	RPM_LDO(PM8058_L23, 0, 0, 0, 1200000, 1200000, LDO300HMIN),
	RPM_LDO(PM8058_L24, 0, 1, 0, 1200000, 1200000, LDO150HMIN),
	RPM_LDO(PM8058_L25, 0, 1, 0, 1200000, 1200000, LDO150HMIN),

	/*	 ID       a_on pd ss min_uV   max_uV   init_ip    freq */
	RPM_SMPS(PM8058_S2, 0, 1, 0, 1200000, 1400000, SMPS_HMIN, 1p92),
	RPM_SMPS(PM8058_S3, 1, 1, 0, 1800000, 1800000, SMPS_HMIN, 1p92),
	RPM_SMPS(PM8058_S4, 1, 1, 0, 2200000, 2200000, SMPS_HMIN, 1p92),

	/*     ID         a_on pd ss */
	RPM_VS(PM8058_LVS0, 0, 1, 0),
	RPM_VS(PM8058_LVS1, 0, 1, 0),

	/*	ID        a_on pd ss min_uV   max_uV */
	RPM_NCP(PM8058_NCP, 0, 1, 0, 1800000, 1800000),

	/*	ID        a_on pd ss min_uV   max_uV   init_ip */
	RPM_LDO(PM8901_L0,  0, 1, 0, 1200000, 1200000, LDO300HMIN),
	RPM_LDO(PM8901_L1,  0, 1, 0, 3200000, 3200000, LDO300HMIN),
	RPM_LDO(PM8901_L2,  0, 1, 0, 2850000, 3300000, LDO300HMIN),
	RPM_LDO(PM8901_L3,  0, 1, 0, 3300000, 3300000, LDO300HMIN),
	RPM_LDO(PM8901_L4,  0, 1, 0, 2850000, 2850000, LDO300HMIN),
	RPM_LDO(PM8901_L5,  0, 1, 0, 2850000, 2850000, LDO300HMIN),
	RPM_LDO(PM8901_L6,  0, 0, 0, 2200000, 2200000, LDO300HMIN),

	/*	 ID       a_on pd ss min_uV   max_uV   init_ip   freq */
	RPM_SMPS(PM8901_S2, 0, 1, 0, 1200000, 1200000, FTS_HMIN, 1p60),
	RPM_SMPS(PM8901_S3, 0, 1, 0, 1100000, 1100000, FTS_HMIN, 1p60),
	RPM_SMPS(PM8901_S4, 0, 1, 0, 1200000, 1200000, FTS_HMIN, 1p60),

	/*	ID        a_on pd ss */
	RPM_VS(PM8901_LVS0, 0, 1, 0),
	RPM_VS(PM8901_LVS1, 0, 1, 0),
	RPM_VS(PM8901_LVS2, 0, 0, 0),
	RPM_VS(PM8901_LVS3, 0, 0, 0),
	RPM_VS(PM8901_MVS0, 0, 0, 0),

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

#ifdef CONFIG_SERIAL_MSM_HS
static int configure_uart_gpios(int on)
{
	int ret = 0, i;
	int uart_gpios[] = {
		SHOOTER_GPIO_BT_UART1_TX,
		SHOOTER_GPIO_BT_UART1_RX,
		SHOOTER_GPIO_BT_UART1_CTS,
		SHOOTER_GPIO_BT_UART1_RTS,
	};
	for (i = 0; i < ARRAY_SIZE(uart_gpios); i++) {
		if (on) {
			ret = msm_gpiomux_get(uart_gpios[i]);
			if (unlikely(ret))
				break;
		} else {
			ret = msm_gpiomux_put(uart_gpios[i]);
			if (unlikely(ret))
				return ret;
		}
	}
	if (ret)
		for (; i >= 0; i--)
			msm_gpiomux_put(uart_gpios[i]);
	return ret;
}

static struct msm_serial_hs_platform_data msm_uart_dm1_pdata = {
       .inject_rx_on_wakeup = 1,
       .rx_to_inject = 0xFD,
       .gpio_config = configure_uart_gpios,
};
#endif

static struct platform_device *early_regulators[] __initdata = {
	&msm_device_saw_s0,
	&msm_device_saw_s1,
	&rpm_regulator_early_device,
};

static struct platform_device *early_devices[] __initdata = {
#ifdef CONFIG_MSM_BUS_SCALING
	&msm_bus_apps_fabric,
	&msm_bus_sys_fabric,
	&msm_bus_mm_fabric,
	&msm_bus_sys_fpb,
	&msm_bus_cpss_fpb,
#endif
	&msm_device_dmov_adm0,
	&msm_device_dmov_adm1,
};

#ifdef CONFIG_SENSORS_MSM_ADC
static struct adc_access_fn xoadc_fn = {
	pm8058_xoadc_select_chan_and_start_conv,
	pm8058_xoadc_read_adc_code,
	pm8058_xoadc_get_properties,
	pm8058_xoadc_slot_request,
	pm8058_xoadc_restore_slot,
	pm8058_xoadc_calibrate,
};

static struct msm_adc_channels msm_adc_channels_data[] = {
	{"vbatt", CHANNEL_ADC_VBATT, 0, &xoadc_fn, CHAN_PATH_TYPE_NONE,
		ADC_CONFIG_TYPE2, ADC_CALIB_CONFIG_TYPE3, scale_default},
	{"vcoin", CHANNEL_ADC_VCOIN, 0, &xoadc_fn, CHAN_PATH_TYPE_NONE,
		ADC_CONFIG_TYPE2, ADC_CALIB_CONFIG_TYPE2, scale_default},
	{"vcharger_channel", CHANNEL_ADC_VCHG, 0, &xoadc_fn, CHAN_PATH_TYPE13,
		ADC_CONFIG_TYPE2, ADC_CALIB_CONFIG_TYPE4, scale_default},
	{"charger_current_monitor", CHANNEL_ADC_CHG_MONITOR, 0, &xoadc_fn,
		CHAN_PATH_TYPE_NONE,
		ADC_CONFIG_TYPE2, ADC_CALIB_CONFIG_TYPE1, scale_default},
	{"vph_pwr", CHANNEL_ADC_VPH_PWR, 0, &xoadc_fn, CHAN_PATH_TYPE_NONE,
		ADC_CONFIG_TYPE2, ADC_CALIB_CONFIG_TYPE3, scale_default},
	{"usb_vbus", CHANNEL_ADC_USB_VBUS, 0, &xoadc_fn, CHAN_PATH_TYPE_NONE,
		ADC_CONFIG_TYPE2, ADC_CALIB_CONFIG_TYPE3, scale_default},
	{"pmic_therm", CHANNEL_ADC_DIE_TEMP, 0, &xoadc_fn, CHAN_PATH_TYPE_NONE,
		ADC_CONFIG_TYPE2, ADC_CALIB_CONFIG_TYPE1, scale_pmic_therm},
	{"pmic_therm_4K", CHANNEL_ADC_DIE_TEMP_4K, 0, &xoadc_fn,
		CHAN_PATH_TYPE12,
		ADC_CONFIG_TYPE1, ADC_CALIB_CONFIG_TYPE7, scale_pmic_therm},
	{"xo_therm", CHANNEL_ADC_XOTHERM, 0, &xoadc_fn, CHAN_PATH_TYPE_NONE,
		ADC_CONFIG_TYPE2, ADC_CALIB_CONFIG_TYPE5, tdkntcgtherm},
	{"xo_therm_4K", CHANNEL_ADC_XOTHERM_4K, 0, &xoadc_fn,
		CHAN_PATH_TYPE_NONE,
		ADC_CONFIG_TYPE1, ADC_CALIB_CONFIG_TYPE6, tdkntcgtherm},
	{"hdset_detect", CHANNEL_ADC_HDSET, 0, &xoadc_fn, CHAN_PATH_TYPE6,
		ADC_CONFIG_TYPE2, ADC_CALIB_CONFIG_TYPE1, scale_default},
	{"chg_batt_amon", CHANNEL_ADC_BATT_AMON, 0, &xoadc_fn, CHAN_PATH_TYPE7,
		ADC_CONFIG_TYPE2, ADC_CALIB_CONFIG_TYPE1,
		scale_default},
	{"msm_therm", CHANNEL_ADC_MSM_THERM, 0, &xoadc_fn, CHAN_PATH_TYPE8,
		ADC_CONFIG_TYPE2, ADC_CALIB_CONFIG_TYPE2, scale_msm_therm},
	{"batt_therm", CHANNEL_ADC_BATT_THERM, 0, &xoadc_fn, CHAN_PATH_TYPE6,
		ADC_CONFIG_TYPE2, ADC_CALIB_CONFIG_TYPE2, scale_default},
	{"batt_id", CHANNEL_ADC_BATT_ID, 0, &xoadc_fn, CHAN_PATH_TYPE_NONE,
		ADC_CONFIG_TYPE2, ADC_CALIB_CONFIG_TYPE2, scale_default},
	{"ref_625mv", CHANNEL_ADC_625_REF, 0, &xoadc_fn, CHAN_PATH_TYPE15,
		ADC_CONFIG_TYPE2, ADC_CALIB_CONFIG_TYPE2, scale_default},
	{"ref_1250mv", CHANNEL_ADC_1250_REF, 0, &xoadc_fn, CHAN_PATH_TYPE13,
		ADC_CONFIG_TYPE2, ADC_CALIB_CONFIG_TYPE2, scale_default},
	{"ref_325mv", CHANNEL_ADC_325_REF, 0, &xoadc_fn, CHAN_PATH_TYPE14,
		ADC_CONFIG_TYPE2, ADC_CALIB_CONFIG_TYPE2, scale_default},
};

static struct msm_adc_platform_data msm_adc_pdata = {
	.channel = msm_adc_channels_data,
	.num_chan_supported = ARRAY_SIZE(msm_adc_channels_data),
};

static struct platform_device msm_adc_device = {
	.name   = "msm_adc",
	.id = -1,
	.dev = {
		.platform_data = &msm_adc_pdata,
	},
};

/* HTC_HEADSET_GPIO Driver */
static struct htc_headset_gpio_platform_data htc_headset_gpio_data = {
	.hpin_gpio		= SHOOTER_GPIO_AUD_HP_DET,
	.key_enable_gpio	= 0,
	.mic_select_gpio	= 0,
};

static struct platform_device htc_headset_gpio = {
	.name   = "HTC_HEADSET_GPIO",
	.id	= -1,
	.dev	= {
		.platform_data = &htc_headset_gpio_data,
	},
};

/* HTC_HEADSET_PMIC Driver */
static struct htc_headset_pmic_platform_data htc_headset_pmic_data = {
	.driver_flag		= 0,
	.hpin_gpio		= 0,
	.hpin_irq		= 0,
	.key_gpio		= 0,
	.key_irq		= 0,
	.key_enable_gpio	= 0,
	.adc_mic_bias		= {0, 0},
	.adc_remote		= {0, 0, 0, 0, 0, 0},
};

static struct platform_device htc_headset_pmic = {
	.name	= "HTC_HEADSET_PMIC",
	.id	= -1,
	.dev	= {
		.platform_data = &htc_headset_pmic_data,
	},
};

/* HTC_HEADSET_8X60 Driver */
static struct htc_headset_8x60_platform_data htc_headset_8x60_data = {
	.adc_mpp	= PM8058_MPP_PM_TO_SYS(XOADC_MPP_10),
	.adc_amux	= PM_MPP_AIN_AMUX_CH5,
	.adc_mic_bias	= {HS_DEF_MIC_ADC_15_BIT_MIN,
			   HS_DEF_MIC_ADC_15_BIT_MAX},
	.adc_remote	= {0, 1251, 1430, 3411, 4543, 6807},
};

static struct htc_headset_8x60_platform_data htc_headset_8x60_data_xb = {
	.adc_mpp	= PM8058_MPP_PM_TO_SYS(XOADC_MPP_10),
	.adc_amux	= PM_MPP_AIN_AMUX_CH5,
	.adc_mic_bias	= {14375, 26643},
	.adc_remote	= {0, 1219, 1440, 3862, 4231, 6783},
};

static struct platform_device htc_headset_8x60 = {
	.name	= "HTC_HEADSET_8X60",
	.id	= -1,
	.dev	= {
		.platform_data  = &htc_headset_8x60_data,
	},
};

/* HTC_HEADSET_MGR Driver */
static struct platform_device *headset_devices[] = {
	&htc_headset_pmic,
	&htc_headset_8x60,
	&htc_headset_gpio,
	/* Please put the headset detection driver on the last */
};

static struct headset_adc_config htc_headset_mgr_config[] = {
	{
		.type = HEADSET_MIC,
		.adc_max = 27404,
		.adc_min = 22294,
	},
	{
		.type = HEADSET_BEATS,
		.adc_max = 22293,
		.adc_min = 7976,
	},
	{
		.type = HEADSET_MIC,
		.adc_max = 7975,
		.adc_min = 666,
	},
	{
		.type = HEADSET_NO_MIC,
		.adc_max = 665,
		.adc_min = 0,
	},
};

static struct htc_headset_mgr_platform_data htc_headset_mgr_data = {
	.driver_flag		= 0,
	.headset_devices_num	= ARRAY_SIZE(headset_devices),
	.headset_devices	= headset_devices,
	.headset_config_num	= 0,
	.headset_config		= 0,
};

static struct platform_device htc_headset_mgr = {
	.name	= "HTC_HEADSET_MGR",
	.id	= -1,
	.dev	= {
		.platform_data  = &htc_headset_mgr_data,
	},
};

static void headset_device_register(void)
{
	pr_info("[HS_BOARD] (%s) Headset device register\n", __func__);
	platform_device_register(&htc_headset_mgr);
};

static uint32_t usb_ID_PIN_input_table[] = {
	GPIO_CFG(SHOOTER_GPIO_USB_ID, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
};

static uint32_t usb_ID_PIN_ouput_table[] = {
	GPIO_CFG(SHOOTER_GPIO_USB_ID, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
};

static uint32_t mhl_usb_switch_ouput_table[] = {
	GPIO_CFG(SHOOTER_GPIO_MHL_USB_SW, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_4MA),
};

static struct pm8xxx_mpp_init_info usb_mpp_init_configs[] = {
	PM8058_MPP_INIT(10, D_INPUT, PM8058_MPP_DIG_LEVEL_S3, DIN_TO_INT),
	PM8058_MPP_INIT(11, D_BI_DIR, PM8058_MPP_DIG_LEVEL_S3, BI_PULLUP_10KOHM),
};

static int mpp_init_setup(struct pm8xxx_mpp_init_info *configs, int length)
{
	int ret = 0, i;

	for (i = 0; i < length; i++) {
		ret = pm8xxx_mpp_config(configs[i].mpp,
				&configs[i].config);
		if (ret) {
			pr_err("%s: Config MPP %d of PM8058 failed\n",
					__func__, configs[i].mpp);
			return ret;
		}
	}
	return ret;
}

static void pm8058_usb_config(void)
{
	mpp_init_setup(usb_mpp_init_configs, ARRAY_SIZE(usb_mpp_init_configs));
}

void config_shooter_usb_id_gpios(bool output)
{
	if (output) {
		gpio_tlmm_config(usb_ID_PIN_ouput_table[0], 0);
		gpio_set_value(SHOOTER_GPIO_USB_ID, 1);
		printk(KERN_INFO "%s %d output high\n",  __func__, SHOOTER_GPIO_USB_ID);
	} else {
		gpio_tlmm_config(usb_ID_PIN_input_table[0], 0);
		printk(KERN_INFO "%s %d input none pull\n",  __func__, SHOOTER_GPIO_USB_ID);
	}
}

static void shooter_usb_dpdn_switch(int path)
{
	switch (path) {
	case PATH_USB:
	case PATH_MHL:
	{
		int polarity = 1; /* high = mhl */
		int mhl = (path == PATH_MHL);

		gpio_tlmm_config(mhl_usb_switch_ouput_table[0], 0);

		pr_info("[CABLE] %s: Set %s path\n", __func__, mhl ? "MHL" : "USB");
		gpio_set_value(SHOOTER_GPIO_MHL_USB_SW, (mhl ^ !polarity) ? 1 : 0);
		break;
	}
	}

#ifdef CONFIG_FB_MSM_HDMI_MHL
	sii9234_change_usb_owner((path == PATH_MHL)?1:0);
#endif
}

static struct cable_detect_platform_data cable_detect_pdata = {
	.vbus_mpp_gpio		= PM8058_MPP_PM_TO_SYS(10),
	.vbus_mpp_config	= pm8058_usb_config,
	.vbus_mpp_irq		= PM8058_IRQ_BASE + PM8058_CBLPWR_IRQ,
	.detect_type		= CABLE_TYPE_PMIC_ADC,
	.usb_id_pin_gpio	= SHOOTER_GPIO_USB_ID,
	.usb_dpdn_switch	= shooter_usb_dpdn_switch,
	.mhl_reset_gpio		= SHOOTER_GPIO_MHL_RESET,
	.mpp_data = {
		.usbid_mpp	= PM8058_MPP_PM_TO_SYS(XOADC_MPP_4),
		.usbid_amux	= PM_MPP_AIN_AMUX_CH5,
	},
	.config_usb_id_gpios	= config_shooter_usb_id_gpios,
#ifdef CONFIG_FB_MSM_HDMI_MHL
	.mhl_1v2_power		= mhl_sii9234_1v2_power,
#endif
};

static struct platform_device cable_detect_device = {
	.name   = "cable_detect",
	.id	= -1,
	.dev	= {
		.platform_data = &cable_detect_pdata,
	},
};

static void pmic8058_xoadc_mpp_config(void)
{
	/* Do not set mpp to amux in here.
	   Set the mpp to amux mapping when needed and then reset */
}

static struct regulator *vreg_ldo18_adc;

static int pmic8058_xoadc_vreg_config(int on)
{
	int rc;

	if (on) {
		rc = regulator_enable(vreg_ldo18_adc);
		if (rc)
			pr_err("%s: Enable of regulator ldo18_adc "
						"failed\n", __func__);
	} else {
		rc = regulator_disable(vreg_ldo18_adc);
		if (rc)
			pr_err("%s: Disable of regulator ldo18_adc "
						"failed\n", __func__);
	}

	return rc;
}

static int pmic8058_xoadc_vreg_setup(void)
{
	int rc;

	vreg_ldo18_adc = regulator_get(NULL, "8058_l18");
	if (IS_ERR(vreg_ldo18_adc)) {
		printk(KERN_ERR "%s: vreg get failed (%ld)\n",
			__func__, PTR_ERR(vreg_ldo18_adc));
		rc = PTR_ERR(vreg_ldo18_adc);
		goto fail;
	}

	rc = regulator_set_voltage(vreg_ldo18_adc, 2200000, 2200000);
	if (rc) {
		pr_err("%s: unable to set ldo18 voltage to 2.2V\n", __func__);
		goto fail;
	}

        rc = regulator_enable(vreg_ldo18_adc);
        if (rc) {
                pr_err("%s: Enable of regulator ldo18_adc "
                                        "failed\n", __func__);
                goto fail;
        }

	return rc;
fail:
	regulator_put(vreg_ldo18_adc);
	return rc;
}

static void pmic8058_xoadc_vreg_shutdown(void)
{
	regulator_put(vreg_ldo18_adc);
}

/* usec. For this ADC,
 * this time represents clk rate @ txco w/ 1024 decimation ratio.
 * Each channel has different configuration, thus at the time of starting
 * the conversion, xoadc will return actual conversion time
 */
static struct adc_properties pm8058_xoadc_data = {
	.adc_reference		= 2200, /* milli-voltage for this adc */
	.bitresolution		= 15,
	.bipolar		= 0,
	.conversiontime		= 54,
};

static struct xoadc_platform_data pm8058_xoadc_pdata = {
	.xoadc_prop = &pm8058_xoadc_data,
	.xoadc_mpp_config = pmic8058_xoadc_mpp_config,
	.xoadc_vreg_set = pmic8058_xoadc_vreg_config,
	.xoadc_num = XOADC_PMIC_0,
	.xoadc_vreg_setup = pmic8058_xoadc_vreg_setup,
	.xoadc_vreg_shutdown = pmic8058_xoadc_vreg_shutdown,
};
#endif

#ifdef CONFIG_PMIC8058
static int pm8058_pwm_config(struct pwm_device *pwm, int ch, int on)
{
	struct pm_gpio pwm_gpio_config = {
		.direction	= PM_GPIO_DIR_OUT,
		.output_buffer  = PM_GPIO_OUT_BUF_CMOS,
		.output_value   = 0,
		.pull		= PM_GPIO_PULL_NO,
		.vin_sel	= PM8058_GPIO_VIN_VPH,
		.out_strength   = PM_GPIO_STRENGTH_HIGH,
		.function	= PM_GPIO_FUNC_2,
	};

	int rc = -EINVAL;
	int id, mode, max_mA;

	id = mode = max_mA = 0;
	switch (ch) {
	case 0:
	case 1:
	case 2:
		if (on) {
			id = 24 + ch;
			rc = pm8xxx_gpio_config(PM8058_GPIO_PM_TO_SYS(id - 1),
							&pwm_gpio_config);
			if (rc)
				pr_err("%s: pm8xxx_gpio_config(%d): rc=%d\n",
					__func__, id, rc);
		}
		break;

	case 6:
		id = PM_PWM_LED_FLASH;
		mode = PM_PWM_CONF_PWM1;
		max_mA = 300;
		break;

	case 7:
		id = PM_PWM_LED_FLASH1;
		mode = PM_PWM_CONF_PWM1;
		max_mA = 300;
		break;

	default:
		break;
	}

	if (ch >= 6 && ch <= 7) {
		if (!on) {
			mode = PM_PWM_CONF_NONE;
			max_mA = 0;
		}
		rc = pm8058_pwm_config_led(pwm, id, mode, max_mA);
		if (rc)
			pr_err("%s: pm8058_pwm_config_led(ch=%d): rc=%d\n",
			       __func__, ch, rc);
	}
	return rc;

}

static struct pm8058_pwm_pdata pm8058_pwm_data = {
	.config		= pm8058_pwm_config,
};

#define PM8058_GPIO_INT           88
static struct pmic8058_led pmic8058_flash_leds[] = {
	[0] = {
		.name		= "camera:flash0",
		.max_brightness = 15,
		.id		= PMIC8058_ID_FLASH_LED_0,
	},
	[1] = {
		.name		= "camera:flash1",
		.max_brightness = 15,
		.id		= PMIC8058_ID_FLASH_LED_1,
	},
};

static struct pmic8058_leds_platform_data pm8058_flash_leds_data = {
	.num_leds = ARRAY_SIZE(pmic8058_flash_leds),
	.leds	= pmic8058_flash_leds,
};

static struct pm8058_led_config pm_led_config[] = {
	{
		.name = "green",
		.type = PM8058_LED_RGB,
		.bank = 0,
		.pwm_size = 9,
		.clk = PM_PWM_CLK_32KHZ,
		.pre_div = PM_PWM_PREDIVIDE_2,
		.pre_div_exp = 1,
		.pwm_value = 511,
	},
	{
		.name = "amber",
		.type = PM8058_LED_RGB,
		.bank = 1,
		.pwm_size = 9,
		.clk = PM_PWM_CLK_32KHZ,
		.pre_div = PM_PWM_PREDIVIDE_2,
		.pre_div_exp = 1,
		.pwm_value = 511,
	},
	{
		.name = "button-backlight",
		.type = PM8058_LED_DRVX,
		.bank = 6,
		.flags = PM8058_LED_LTU_EN,
		.period_us = USEC_PER_SEC / 1000,
		.start_index = 0,
		.duites_size = 8,
		.duty_time_ms = 32,
		.lut_flag = PM_PWM_LUT_RAMP_UP | PM_PWM_LUT_PAUSE_HI_EN,
		.out_current = 10,
	},
};

static struct pm8058_led_platform_data pm8058_leds_data = {
	.led_config = pm_led_config,
	.num_leds = ARRAY_SIZE(pm_led_config),
	.duties = {0, 15, 30, 45, 60, 75, 90, 100,
		   100, 90, 75, 60, 45, 30, 15, 0,
		   0, 0, 0, 0, 0, 0, 0, 0,
		   0, 0, 0, 0, 0, 0, 0, 0,
		   0, 0, 0, 0, 0, 0, 0, 0,
		   0, 0, 0, 0, 0, 0, 0, 0,
		   0, 0, 0, 0, 0, 0, 0, 0,
		   0, 0, 0, 0, 0, 0, 0, 0},
};

static struct platform_device pm8058_leds = {
	.name	= "leds-pm8058",
	.id	= -1,
	.dev	= {
		.platform_data  = &pm8058_leds_data,
	},
};

#define PMIC_GPIO_SDC3_DET 34
static int pm8058_gpios_init(void)
{
	int i;
	int rc;
	struct pm8058_gpio_cfg {
		int		gpio;
		struct pm_gpio	cfg;
	};

	struct pm8058_gpio_cfg gpio_cfgs[] = {
#ifdef CONFIG_MMC_MSM_CARD_HW_DETECTION
		{
			PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_SDC3_DET - 1),
			{
				.direction	= PM_GPIO_DIR_IN,
				.pull		= PM_GPIO_PULL_UP_30,
				.vin_sel	= 2,
				.function	= PM_GPIO_FUNC_NORMAL,
				.inv_int_pol	= 0,
			},
		},
#endif
		{ /* Audio Microphone Selector */
			PM8058_GPIO_PM_TO_SYS(SHOOTER_AUD_MIC_SEL),	/* 26 */
			{
				.direction	= PM_GPIO_DIR_OUT,
				.output_value	= 0,
				.output_buffer	= PM_GPIO_OUT_BUF_CMOS,
				.pull		= PM_GPIO_PULL_NO,
				.out_strength	= PM_GPIO_STRENGTH_HIGH,
				.function	= PM_GPIO_FUNC_NORMAL,
				.vin_sel	= 2,	/* LDO 1.8 V */
				.inv_int_pol	= 0,
			}
		},
		{ /* Audio Receiver Amplifier */
			PM8058_GPIO_PM_TO_SYS(SHOOTER_AUD_HP_EN),
			{
				.direction	= PM_GPIO_DIR_OUT,
				.output_value	= 0,
				.output_buffer	= PM_GPIO_OUT_BUF_CMOS,
				.pull		= PM_GPIO_PULL_NO,
				.out_strength	= PM_GPIO_STRENGTH_HIGH,
				.function	= PM_GPIO_FUNC_NORMAL,
				.vin_sel	= 6,	/* LDO5 2.85 V */
				.inv_int_pol	= 0,
			}
		},
		{ /* Timpani Reset */
			PM8058_GPIO_PM_TO_SYS(20),
			{
				.direction	= PM_GPIO_DIR_OUT,
				.output_value	= 0,
				.output_buffer	= PM_GPIO_OUT_BUF_CMOS,
				.pull		= PM_GPIO_PULL_NO,
				.out_strength	= PM_GPIO_STRENGTH_HIGH,
				.function	= PM_GPIO_FUNC_NORMAL,
				.vin_sel	= 2,
				.inv_int_pol	= 0,
			}
		},
		{
			PM8058_GPIO_PM_TO_SYS(SHOOTER_PS_VOUT),
			{
				.direction	= PM_GPIO_DIR_IN,
				.pull		= PM_GPIO_PULL_UP_1P5,
				.vin_sel	= PM8058_GPIO_VIN_L5,
				.function	= PM_GPIO_FUNC_NORMAL,
				.inv_int_pol	= 0,
			},
		},
		{ /* Green LED */
			PM8058_GPIO_PM_TO_SYS(SHOOTER_GREEN_LED),
			{
				.direction	= PM_GPIO_DIR_OUT,
				.output_value	= 1,
				.output_buffer	= PM_GPIO_OUT_BUF_CMOS,
				.pull		= PM_GPIO_PULL_NO,
				.out_strength	= PM_GPIO_STRENGTH_HIGH,
				.function	= PM_GPIO_FUNC_2,
				.vin_sel	= PM8058_GPIO_VIN_L5,
				.inv_int_pol	= 0,
			}
		},
		{ /* AMBER */
			PM8058_GPIO_PM_TO_SYS(SHOOTER_AMBER_LED),
			{
				.direction	= PM_GPIO_DIR_OUT,
				.output_value	= 1,
				.output_buffer	= PM_GPIO_OUT_BUF_CMOS,
				.pull		= PM_GPIO_PULL_NO,
				.out_strength	= PM_GPIO_STRENGTH_HIGH,
				.function	= PM_GPIO_FUNC_2,
				.vin_sel	= PM8058_GPIO_VIN_L5,
				.inv_int_pol	= 0,
			}
		},
#ifdef CONFIG_MACH_SHOOTER
		{ /* WIMAX HOST WAKEUP */
			PM8058_GPIO_PM_TO_SYS(SHOOTER_WIMAX_HOST_WAKEUP),
			{
				.direction	= PM_GPIO_DIR_IN,
				.output_value	= 0,
				.pull		= PM_GPIO_PULL_DN,
				.function	= PM_GPIO_FUNC_NORMAL,
				.vin_sel	= PM8058_GPIO_VIN_S3,
				.inv_int_pol	= 0,
			}
		},
#endif
		{ /* 3D CLK */
			PM8058_GPIO_PM_TO_SYS(SHOOTER_3DCLK),
			{
				.direction	= PM_GPIO_DIR_OUT,
				.output_value	= 0,
				.output_buffer	= PM_GPIO_OUT_BUF_CMOS,
				.pull		= PM_GPIO_PULL_NO,
				.out_strength	= PM_GPIO_STRENGTH_HIGH,
				.function	= PM_GPIO_FUNC_NORMAL,
				.vin_sel	= PM8058_GPIO_VIN_L5,
				.inv_int_pol	= 0,
			}
		},
		{ /* 3DLCM_PD */
			PM8058_GPIO_PM_TO_SYS(SHOOTER_3DLCM_PD),
			{
				.direction	= PM_GPIO_DIR_OUT,
				.output_value	= 0,
				.output_buffer	= PM_GPIO_OUT_BUF_CMOS,
				.pull		= PM_GPIO_PULL_NO,
				.out_strength	= PM_GPIO_STRENGTH_HIGH,
				.function	= PM_GPIO_FUNC_NORMAL,
				.vin_sel	= PM8058_GPIO_VIN_L5,
				.inv_int_pol	= 0,
			}
		},
		{ /* TORCH_SET1 for Flashlight */
			PM8058_GPIO_PM_TO_SYS(SHOOTER_TORCH_SET1),
			{
				.direction	= PM_GPIO_DIR_OUT,
				.output_value	= 0,
				.output_buffer	= PM_GPIO_OUT_BUF_CMOS,
				.pull		= PM_GPIO_PULL_NO,
				.out_strength	= PM_GPIO_STRENGTH_HIGH,
				.function	= PM_GPIO_FUNC_NORMAL,
				.vin_sel	= PM8058_GPIO_VIN_L5,
				.inv_int_pol	= 0,
			}
		},
		{ /* TORCH_SET2 for Flashlight */
			PM8058_GPIO_PM_TO_SYS(SHOOTER_TORCH_SET2),
			{
				.direction	= PM_GPIO_DIR_OUT,
				.output_value	= 0,
				.output_buffer	= PM_GPIO_OUT_BUF_CMOS,
				.pull		= PM_GPIO_PULL_NO,
				.out_strength	= PM_GPIO_STRENGTH_HIGH,
				.function	= PM_GPIO_FUNC_NORMAL,
				.vin_sel	= PM8058_GPIO_VIN_L5,
				.inv_int_pol	= 0,
			}
		},
		{
			PM8058_GPIO_PM_TO_SYS(SHOOTER_AUD_REMO_EN),
			{
				.direction	= PM_GPIO_DIR_OUT,
				.output_value	= 0,
				.output_buffer	= PM_GPIO_OUT_BUF_CMOS,
				.pull		= PM_GPIO_PULL_NO,
				.out_strength	= PM_GPIO_STRENGTH_HIGH,
				.function	= PM_GPIO_FUNC_NORMAL,
				.vin_sel	= PM8058_GPIO_VIN_S3, /* 1.8 V */
				.inv_int_pol	= 0,
			}
		},
		{
			PM8058_GPIO_PM_TO_SYS(SHOOTER_AUD_REMO_PRES),
			{
				.direction	= PM_GPIO_DIR_IN,
				.pull		= PM_GPIO_PULL_UP_1P5,
				.vin_sel	= PM8058_GPIO_VIN_L5, /* 2.85 V */
				.function	= PM_GPIO_FUNC_NORMAL,
				.inv_int_pol	= 0,
			},
		},
	};

	for (i = 0; i < ARRAY_SIZE(gpio_cfgs); ++i) {
		rc = pm8xxx_gpio_config(gpio_cfgs[i].gpio,
				&gpio_cfgs[i].cfg);
		if (rc < 0) {
			pr_err("%s pmic gpio config failed on element %d\n",
				__func__, i);
			return rc;
		}
	}

	return 0;
}

static struct pm8xxx_vibrator_platform_data pm8058_vib_pdata = {
	.initial_vibrate_ms = 0,
	.level_mV = 3000,
	.max_timeout_ms = 15000,
};

#if defined(CONFIG_PMIC8058_OTHC) || defined(CONFIG_PMIC8058_OTHC_MODULE)
#define PM8058_LINE_IN_DET_GPIO PM8058_GPIO_PM_TO_SYS(18)
static struct othc_accessory_info othc_accessories[]  = {
	{
		.accessory = OTHC_SVIDEO_OUT,
		.detect_flags = OTHC_MICBIAS_DETECT | OTHC_SWITCH_DETECT |
							OTHC_ADC_DETECT,
		.key_code = SW_VIDEOOUT_INSERT,
		.enabled = false,
		.adc_thres = {
				.min_threshold = 20,
				.max_threshold = 40,
		},
	},
	{
		.accessory = OTHC_ANC_HEADPHONE,
		.detect_flags = OTHC_MICBIAS_DETECT | OTHC_GPIO_DETECT |
							OTHC_SWITCH_DETECT,
		.gpio = PM8058_LINE_IN_DET_GPIO,
		.active_low = 1,
		.key_code = SW_HEADPHONE_INSERT,
		.enabled = true,
	},
	{
		.accessory = OTHC_ANC_HEADSET,
		.detect_flags = OTHC_MICBIAS_DETECT | OTHC_GPIO_DETECT,
		.gpio = PM8058_LINE_IN_DET_GPIO,
		.active_low = 1,
		.key_code = SW_HEADPHONE_INSERT,
		.enabled = true,
	},
	{
		.accessory = OTHC_HEADPHONE,
		.detect_flags = OTHC_MICBIAS_DETECT | OTHC_SWITCH_DETECT,
		.key_code = SW_HEADPHONE_INSERT,
		.enabled = true,
	},
	{
		.accessory = OTHC_MICROPHONE,
		.detect_flags = OTHC_GPIO_DETECT,
		.gpio = PM8058_LINE_IN_DET_GPIO,
		.active_low = 1,
		.key_code = SW_MICROPHONE_INSERT,
		.enabled = true,
	},
	{
		.accessory = OTHC_HEADSET,
		.detect_flags = OTHC_MICBIAS_DETECT,
		.key_code = SW_HEADPHONE_INSERT,
		.enabled = true,
	},
};

static struct othc_switch_info switch_info[] = {
	{
		.min_adc_threshold = 0,
		.max_adc_threshold = 100,
		.key_code = KEY_PLAYPAUSE,
	},
	{
		.min_adc_threshold = 100,
		.max_adc_threshold = 200,
		.key_code = KEY_REWIND,
	},
	{
		.min_adc_threshold = 200,
		.max_adc_threshold = 500,
		.key_code = KEY_FASTFORWARD,
	},
};

static struct othc_n_switch_config switch_config = {
	.voltage_settling_time_ms = 0,
	.num_adc_samples = 3,
	.adc_channel = CHANNEL_ADC_HDSET,
	.switch_info = switch_info,
	.num_keys = ARRAY_SIZE(switch_info),
	.default_sw_en = true,
	.default_sw_idx = 0,
};

static struct hsed_bias_config hsed_bias_config = {
	/* HSED mic bias config info */
	.othc_headset = OTHC_HEADSET_NO,
	.othc_lowcurr_thresh_uA = 100,
	.othc_highcurr_thresh_uA = 600,
	.othc_hyst_prediv_us = 7800,
	.othc_period_clkdiv_us = 62500,
	.othc_hyst_clk_us = 121000,
	.othc_period_clk_us = 312500,
	.othc_wakeup = 1,
};

static struct othc_hsed_config hsed_config_1 = {
	.hsed_bias_config = &hsed_bias_config,
	/*
	 * The detection delay and switch reporting delay are
	 * required to encounter a hardware bug (spurious switch
	 * interrupts on slow insertion/removal of the headset).
	 * This will introduce a delay in reporting the accessory
	 * insertion and removal to the userspace.
	 */
	.detection_delay_ms = 1500,
	/* Switch info */
	.switch_debounce_ms = 1500,
	.othc_support_n_switch = false,
	.switch_config = &switch_config,
	.ir_gpio = -1,
	/* Accessory info */
	.accessories_support = true,
	.accessories = othc_accessories,
	.othc_num_accessories = ARRAY_SIZE(othc_accessories),
};

static struct othc_regulator_config othc_reg = {
	.regulator	= "8058_l5",
	.max_uV		= 2850000,
	.min_uV		= 2850000,
};

/* MIC_BIAS0 is configured as normal MIC BIAS */
static struct pmic8058_othc_config_pdata othc_config_pdata_0 = {
	.micbias_select = OTHC_MICBIAS_0,
	.micbias_capability = OTHC_MICBIAS,
	.micbias_enable = OTHC_SIGNAL_OFF,
	.micbias_regulator = &othc_reg,
};

/* MIC_BIAS1 is configured as normal for OTHC */
static struct pmic8058_othc_config_pdata othc_config_pdata_1 = {
	.micbias_select = OTHC_MICBIAS_1,
	.micbias_capability = OTHC_MICBIAS,
	.micbias_enable = OTHC_SIGNAL_OFF,
	.micbias_regulator = &othc_reg,
};

static void __init msm8x60_init_pm8058_othc(void)
{
	if (SOCINFO_VERSION_MAJOR(socinfo_get_version()) == 2) {
		/* 3-switch headset supported only by V2 FFA and FLUID */
		hsed_config_1.accessories_adc_support = true,
		/* ADC based accessory detection works only on V2 and FLUID */
		hsed_config_1.accessories_adc_channel = CHANNEL_ADC_HDSET,
		hsed_config_1.othc_support_n_switch = true;
	}
}
#endif

static struct pm8xxx_rtc_platform_data pm8058_rtc_pdata = {
	.rtc_write_enable       = true,
	.rtc_alarm_powerup	= false,
};

static struct pm8xxx_pwrkey_platform_data pm8058_pwrkey_pdata = {
	.pull_up		= 1,
	.kpd_trigger_delay_us   = 15625,
	.wakeup			= 1,
};

static struct pm8xxx_misc_platform_data pm8058_misc_pdata = {
	.priority		= 0,
};

static struct pm8xxx_irq_platform_data pm8058_irq_pdata = {
	.irq_base		= PM8058_IRQ_BASE,
	.devirq			= MSM_GPIO_TO_INT(PM8058_GPIO_INT),
	.irq_trigger_flag	= IRQF_TRIGGER_LOW,
};

static struct pm8xxx_gpio_platform_data pm8058_gpio_pdata = {
	.gpio_base	= PM8058_GPIO_PM_TO_SYS(0),
};

static struct pm8xxx_mpp_platform_data pm8058_mpp_pdata = {
	.mpp_base	= PM8058_MPP_PM_TO_SYS(0),
};

static struct pm8058_platform_data pm8058_platform_data = {
	.irq_pdata		= &pm8058_irq_pdata,
	.gpio_pdata		= &pm8058_gpio_pdata,
	.mpp_pdata		= &pm8058_mpp_pdata,
	.rtc_pdata		= &pm8058_rtc_pdata,
	.pwrkey_pdata		= &pm8058_pwrkey_pdata,
#if defined(CONFIG_PMIC8058_OTHC) || defined(CONFIG_PMIC8058_OTHC_MODULE)
	.othc0_pdata		= &othc_config_pdata_0,
	.othc1_pdata		= &othc_config_pdata_1,
	.othc2_pdata		= NULL, /* No othc2 pdata */
#endif
	.pwm_pdata		= &pm8058_pwm_data,
	.misc_pdata		= &pm8058_misc_pdata,
#ifdef CONFIG_SENSORS_MSM_ADC
	.xoadc_pdata		= &pm8058_xoadc_pdata,
#endif
	.keypad_pdata		= NULL, /* No keypad pdata */
	.charger_pdata		= NULL, /* No charger pdata */
//	.leds_pdata		= &pm8058_flash_leds_data,
//	.vibrator_pdata		= &pm8058_vib_pdata,

	.regulator_pdatas	= NULL,
	.num_regulators		= 0,

	.xo_buffer_pdata	= NULL,
	.num_xo_buffers		= 0,
};

#ifdef CONFIG_MSM_SSBI
static struct msm_ssbi_platform_data msm8x60_ssbi_pm8058_pdata __devinitdata = {
	.controller_type = MSM_SBI_CTRL_PMIC_ARBITER,
	.slave	= {
		.name			= "pm8058-core",
		.platform_data		= &pm8058_platform_data,
	},
};
#endif
#endif  /* CONFIG_PMIC8058 */

static int shooter_ts_atmel_power(int on)
{
	pr_info("%s: power %d\n", __func__, on);

	gpio_set_value(SHOOTER_TP_RST, 0);
	msleep(5);
	gpio_set_value(SHOOTER_TP_RST, 1);
	msleep(40);

	return 0;
}

struct atmel_i2c_platform_data shooter_ts_atmel_data[] = {
	{
		.version = 0x020,
		.source = 1, /* ALPS, Nissha */
		.abs_x_min = 5,
		.abs_x_max = 1018,
		.abs_y_min = 7,
		.abs_y_max = 905,
		.abs_pressure_min = 0,
		.abs_pressure_max = 255,
		.abs_width_min = 0,
		.abs_width_max = 20,
		.gpio_irq = SHOOTER_TP_ATT_N,
		.power = shooter_ts_atmel_power,
		.config_T6 = {0, 0, 0, 0, 0, 0},
		.config_T7 = {16, 8, 50},
		.config_T8 = {9, 0, 5, 2, 0, 0, 5, 15, 4, 170},
		.config_T9 = {139, 0, 0, 20, 10, 0, 16, 30, 2, 1, 0, 2, 2, 0, 4, 14, 10, 10, 0, 0, 0, 0, 248, 228, 5, 5, 145, 50, 139, 80, 15, 10},
		.config_T15 = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		.config_T18 = {0, 0},
		.config_T19 = {0, 0, 0, 60, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		.config_T20 = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		.config_T22 = {15, 0, 0, 0, 0, 0, 0, 0, 25, 0, 0, 0, 7, 18, 255, 255, 0},
		.config_T23 = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		.config_T24 = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		.config_T25 = {3, 0, 16, 39, 124, 21, 0, 0, 0, 0, 0, 0, 0, 0},
		.config_T28 = {0, 0, 4, 4, 8, 60},
		.object_crc = {0xC9, 0x30, 0x64},
		.cable_config = {35, 25, 8, 16},
		.noise_config = {45, 2, 35},
		.GCAF_level = {20, 24, 28, 40, 63},
	},
	{
		.version = 0x020,
		.source = 0, /* TPK */
		.abs_x_min = 5,
		.abs_x_max = 1018,
		.abs_y_min = 7,
		.abs_y_max = 905,
		.abs_pressure_min = 0,
		.abs_pressure_max = 255,
		.abs_width_min = 0,
		.abs_width_max = 20,
		.gpio_irq = SHOOTER_TP_ATT_N,
		.power = shooter_ts_atmel_power,
		.config_T6 = {0, 0, 0, 0, 0, 0},
		.config_T7 = {16, 8, 50},
		.config_T8 = {8, 0, 5, 2, 0, 0, 5, 15, 4, 170},
		.config_T9 = {139, 0, 0, 20, 10, 0, 16, 30, 2, 1, 0, 2, 2, 0, 4, 14, 10, 10, 0, 0, 0, 0, 6, 0, 15, 14, 140, 43, 147, 77, 15, 10},
		.config_T15 = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		.config_T18 = {0, 0},
		.config_T19 = {0, 0, 0, 60, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		.config_T20 = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		.config_T22 = {15, 0, 0, 0, 0, 0, 0, 0, 25, 0, 0, 0, 7, 18, 255, 255, 0},
		.config_T23 = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		.config_T24 = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		.config_T25 = {3, 0, 16, 39, 124, 21, 0, 0, 0, 0, 0, 0, 0, 0},
		.config_T28 = {0, 0, 4, 4, 8, 60},
		.object_crc = {0x4F, 0xEE, 0xF3},
		.cable_config = {35, 25, 8, 16},
		.noise_config = {45, 2, 35},
		.GCAF_level = {20, 24, 28, 40, 63},
	},
	{
		.version = 0x016,
		.source = 1, /* ALPS, Nissha */
		.abs_x_min = 5,
		.abs_x_max = 1018,
		.abs_y_min = 7,
		.abs_y_max = 905,
		.abs_pressure_min = 0,
		.abs_pressure_max = 255,
		.abs_width_min = 0,
		.abs_width_max = 20,
		.gpio_irq = SHOOTER_TP_ATT_N,
		.power = shooter_ts_atmel_power,
		.config_T6 = {0, 0, 0, 0, 0, 0},
		.config_T7 = {16, 8, 50},
		.config_T8 = {9, 0, 5, 2, 0, 0, 5, 15},
		.config_T9 = {139, 0, 0, 20, 10, 0, 16, 30, 2, 1, 0, 2, 2, 0, 4, 14, 10, 10, 0, 0, 0, 0, 248, 228, 5, 5, 145, 50, 139, 80, 15},
		.config_T15 = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		.config_T18 = {0, 0},
		.config_T19 = {0, 0, 0, 60, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		.config_T20 = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		.config_T22 = {15, 0, 0, 0, 0, 0, 0, 0, 25, 0, 0, 0, 7, 18, 255, 255, 0},
		.config_T23 = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		.config_T24 = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		.config_T25 = {3, 0, 16, 39, 124, 21, 0, 0, 0, 0, 0, 0, 0, 0},
		.config_T27 = {0, 0, 0, 0, 0, 0, 0},
		.config_T28 = {0, 0, 4, 4, 8, 60},
		.cable_config = {35, 25, 8, 16},
		.GCAF_level = {20, 24, 28, 40, 63},
	},
	{
		.version = 0x016,
		.source = 0, /* TPK */
		.abs_x_min = 5,
		.abs_x_max = 1018,
		.abs_y_min = 7,
		.abs_y_max = 905,
		.abs_pressure_min = 0,
		.abs_pressure_max = 255,
		.abs_width_min = 0,
		.abs_width_max = 20,
		.gpio_irq = SHOOTER_TP_ATT_N,
		.power = shooter_ts_atmel_power,
		.config_T6 = {0, 0, 0, 0, 0, 0},
		.config_T7 = {16, 8, 50},
		.config_T8 = {8, 0, 5, 2, 0, 0, 5, 15},
		.config_T9 = {139, 0, 0, 20, 10, 0, 16, 30, 2, 1, 0, 2, 2, 0, 4, 14, 10, 10, 0, 0, 0, 0, 6, 0, 15, 14, 140, 43, 147, 77, 15},
		.config_T15 = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		.config_T18 = {0, 0},
		.config_T19 = {0, 0, 0, 60, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		.config_T20 = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		.config_T22 = {15, 0, 0, 0, 0, 0, 0, 0, 25, 0, 0, 0, 7, 18, 255, 255, 0},
		.config_T23 = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		.config_T24 = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		.config_T25 = {3, 0, 16, 39, 124, 21, 0, 0, 0, 0, 0, 0, 0, 0},
		.config_T27 = {0, 0, 0, 0, 0, 0, 0},
		.config_T28 = {0, 0, 4, 4, 8, 60},
		.cable_config = {35, 25, 8, 16},
		.GCAF_level = {20, 24, 28, 40, 63},
	},
};

static struct i2c_board_info msm_i2c_gsbi5_info[] = {
	{
		I2C_BOARD_INFO(ATMEL_QT602240_NAME, 0x94 >> 1),
		.platform_data = &shooter_ts_atmel_data,
		.irq = MSM_GPIO_TO_INT(SHOOTER_TP_ATT_N),
	},
};

static struct mpu3050_platform_data mpu3050_data = {
	.int_config = 0x10,
	.orientation = { -1, 0, 0, 0, 1, 0, 0, 0, -1 },
	.level_shifter = 0,
	.accel = {
		.get_slave_descr = get_accel_slave_descr,
		.adapt_num = 5, /* The i2c bus to which the mpu device is connected */
		.bus = EXT_SLAVE_BUS_SECONDARY,
		.address = 0x30 >> 1,
		.orientation = { -1, 0, 0, 0, 1, 0, 0, 0, -1 },
	},

	.compass = {
		.get_slave_descr = get_compass_slave_descr,
		.adapt_num = 5, /* The i2c bus to which the mpu device is connected */
		.bus = EXT_SLAVE_BUS_PRIMARY,
		.address = 0x1A >> 1,
		.orientation = { -1, 0, 0, 0, 1, 0, 0, 0, -1 },
	},
};

static struct i2c_board_info __initdata mpu3050_GSBI10_boardinfo[] = {
	{
		I2C_BOARD_INFO("mpu3050", 0xD0 >> 1),
		.irq = MSM_GPIO_TO_INT(SHOOTER_GYRO_INT),
		.platform_data = &mpu3050_data,
	},
};

static int isl29028_power(int pwr_device, uint8_t enable)
{
	return 0;
};

static struct isl29028_platform_data isl29028_pdata = {
	.intr = PM8058_GPIO_PM_TO_SYS(SHOOTER_PS_VOUT),
	.levels = {17, 79, 258, 588, 918, 1250, 1962, 2673, 3384, 4095},
	.golden_adc = 0x4E2,
	.power = isl29028_power,
	.lt = 0x15,
	.ht = 0x16,
};

static struct i2c_board_info i2c_isl29028_devices[] = {
	{
		I2C_BOARD_INFO(ISL29028_I2C_NAME, 0x8A >> 1),
		.platform_data = &isl29028_pdata,
		.irq = PM8058_GPIO_IRQ(PM8058_IRQ_BASE, SHOOTER_PS_VOUT),
	},
};

static int isl29029_power(int pwr_device, uint8_t enable)
{
	return 0;
};

static struct isl29029_platform_data isl29029_pdata = {
	.intr = PM8058_GPIO_PM_TO_SYS(SHOOTER_PS_VOUT),
	.levels = {17, 79, 258, 588, 918, 1250, 1962, 2673, 3384, 4095},
	.golden_adc = 0x4E2,
	.power = isl29029_power,
	.lt = 0x15,
	.ht = 0x16,
};

static struct i2c_board_info i2c_isl29029_devices[] = {
	{
		I2C_BOARD_INFO(ISL29029_I2C_NAME, 0x8A >> 1),
		.platform_data = &isl29029_pdata,
		.irq = PM8058_GPIO_IRQ(PM8058_IRQ_BASE, SHOOTER_PS_VOUT),
	},
};

#ifdef CONFIG_MSM8X60_AUDIO
static uint32_t msm_spi_gpio[] = {
	GPIO_CFG(SHOOTER_SPI_DO,  0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	GPIO_CFG(SHOOTER_SPI_DI,  0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
	GPIO_CFG(SHOOTER_SPI_CS,  0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	GPIO_CFG(SHOOTER_SPI_CLK, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
};

static uint32_t auxpcm_gpio_table[] = {
	GPIO_CFG(111, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	GPIO_CFG(112, 0, GPIO_CFG_INPUT,  GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
	GPIO_CFG(113, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	GPIO_CFG(114, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
};

static void msm_auxpcm_init(void)
{
	gpio_tlmm_config(auxpcm_gpio_table[0], GPIO_CFG_DISABLE);
	gpio_tlmm_config(auxpcm_gpio_table[1], GPIO_CFG_DISABLE);
	gpio_tlmm_config(auxpcm_gpio_table[2], GPIO_CFG_DISABLE);
	gpio_tlmm_config(auxpcm_gpio_table[3], GPIO_CFG_DISABLE);
}

static struct tpa2051d3_platform_data tpa2051d3_pdata = {
	.gpio_tpa2051_spk_en = SHOOTER_AUD_SPK_ENO,
	.spkr_cmd = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
#ifdef CONFIG_MACH_SHOOTER
	.hsed_cmd = {0x00, 0x0C, 0x25, 0x57, 0x6D, 0x4D, 0x0D},
#else
	.hsed_cmd = {0x00, 0x0C, 0x25, 0x57, 0xCD, 0x4D, 0x0D},
#endif
	.rece_cmd = {0x00, 0x02, 0x25, 0x57, 0x0D, 0x4D, 0x0D},
};

#define TPA2051D3_I2C_SLAVE_ADDR	(0xE0 >> 1)

static struct i2c_board_info msm_i2c_gsbi7_tpa2051d3_info[] = {
	{
                I2C_BOARD_INFO(TPA2051D3_I2C_NAME, TPA2051D3_I2C_SLAVE_ADDR),
                .platform_data = &tpa2051d3_pdata,
	},
};

void msm_snddev_voltage_on(void)
{
}

void __init shooter_audio_init(void);

void msm_snddev_voltage_off(void)
{
}

static struct spi_board_info msm_spi_board_info[] __initdata = {
	{
		.modalias	= "spi_aic3254",
		.mode		= SPI_MODE_1,
		.bus_num	= 0,
		.chip_select	= 0,
		.max_speed_hz	= 10800000,
	}
};
#endif /* CONFIG_MSM8X60_AUDIO */

#ifdef CONFIG_I2C
struct i2c_registry {
	int                    bus;
	struct i2c_board_info *info;
	int                    len;
};

static struct tps65200_platform_data tps65200_data = {
	.gpio_chg_stat = PM8058_GPIO_IRQ(PM8058_IRQ_BASE, SHOOTER_CHG_STAT),
	.gpio_chg_int  = MSM_GPIO_TO_INT(SHOOTER_GPIO_CHG_INT),
};

#ifdef CONFIG_SUPPORT_DQ_BATTERY
static int __init check_dq_setup(char *str)
{
	if (!strcmp(str, "PASS"))
		tps65200_data.dq_result = 1;
	else
		tps65200_data.dq_result = 0;

	return 1;
}
__setup("androidboot.dq=", check_dq_setup);
#endif

static struct i2c_board_info msm_tps_65200_boardinfo[] __initdata = {
	{
		I2C_BOARD_INFO("tps65200", 0xD4 >> 1),
		.platform_data = &tps65200_data,
	},
};

static struct i2c_registry msm8x60_i2c_devices[] __initdata = {
	{
		MSM_GSBI5_QUP_I2C_BUS_ID,
		msm_i2c_gsbi5_info,
		ARRAY_SIZE(msm_i2c_gsbi5_info),
	},
	{
		MSM_GSBI7_QUP_I2C_BUS_ID,
		msm_tps_65200_boardinfo,
		ARRAY_SIZE(msm_tps_65200_boardinfo),
	},
#ifdef CONFIG_MSM8X60_AUDIO
	{
		MSM_GSBI7_QUP_I2C_BUS_ID,
		msm_i2c_gsbi7_tpa2051d3_info,
		ARRAY_SIZE(msm_i2c_gsbi7_tpa2051d3_info),
	},
#endif
};
#endif /* CONFIG_I2C */

static void register_i2c_devices(void)
{
#ifdef CONFIG_I2C
	int i;

	/* Run the array and install devices as appropriate */
	for (i = 0; i < ARRAY_SIZE(msm8x60_i2c_devices); ++i) {
		i2c_register_board_info(msm8x60_i2c_devices[i].bus,
					msm8x60_i2c_devices[i].info,
					msm8x60_i2c_devices[i].len);
	}

	i2c_register_board_info(MSM_GSBI10_QUP_I2C_BUS_ID,
		mpu3050_GSBI10_boardinfo, ARRAY_SIZE(mpu3050_GSBI10_boardinfo));

	if (ps_type == 1) {
		i2c_register_board_info(MSM_GSBI10_QUP_I2C_BUS_ID,
			i2c_isl29028_devices,
			ARRAY_SIZE(i2c_isl29028_devices));
	} else if (ps_type == 2) {
		i2c_register_board_info(MSM_GSBI10_QUP_I2C_BUS_ID,
			i2c_isl29029_devices,
			ARRAY_SIZE(i2c_isl29029_devices));
	} else
		printk(KERN_DEBUG "No Intersil chips\n");
#endif
}

static struct platform_device *asoc_devices[] __initdata = {
	&asoc_msm_pcm,
	&asoc_msm_dai0,
	&asoc_msm_dai1,
};

static struct platform_device *devices[] __initdata = {
	&ram_console_device,
	&msm_device_smd,
	&msm_device_uart_dm12,
	&msm_pil_q6v3,
	&msm_pil_modem,
	&msm_pil_tzapps,
#ifdef CONFIG_I2C_QUP
	&msm_gsbi4_qup_i2c_device,
	&msm_gsbi5_qup_i2c_device,
	&msm_gsbi7_qup_i2c_device,
	&msm_gsbi10_qup_i2c_device,
#endif
#ifdef CONFIG_BT
	&shooter_rfkill,
#endif
#ifdef CONFIG_SERIAL_MSM_HS
	&msm_device_uart_dm1,
#endif
#if defined(CONFIG_SPI_QUP) || defined(CONFIG_SPI_QUP_MODULE)
	&msm_gsbi1_qup_spi_device,
#ifdef CONFIG_MACH_SHOOTER
	&msm_gsbi2_qup_spi_device,
#else
	&msm_gsbi3_qup_spi_device,
#endif
#endif
#ifdef CONFIG_MSM_SSBI
	&msm_device_ssbi_pmic1,
	&msm_device_ssbi_pmic2,
#endif
#ifdef CONFIG_I2C_SSBI
	&msm_device_ssbi3,
#endif
#ifdef CONFIG_USB_EHCI_HCD
	&msm_device_otg,
#endif
#ifdef CONFIG_USB_GADGET
	&msm_device_gadget_peripheral,
#endif
#ifdef CONFIG_USB_G_ANDROID
	&android_usb_device,
#endif
#ifdef CONFIG_ANDROID_PMEM
#ifndef CONFIG_MSM_MULTIMEDIA_USE_ION
	&android_pmem_device,
	&android_pmem_adsp_device,
	&android_pmem_smipool_device,
#endif
	&android_pmem_audio_device,
#endif
#ifdef CONFIG_MSM_ROTATOR
	&msm_rotator_device,
#endif
	&shooter_3Dpanel_device,
	&msm_kgsl_3d0,
	&msm_kgsl_2d0,
	&msm_kgsl_2d1,
#ifdef CONFIG_MSM_GEMINI
	&msm_gemini_device,
#endif
#ifdef CONFIG_MSM_VPE
	&msm_vpe_device,
#endif
#if defined(CONFIG_MSM_RPM_LOG) || defined(CONFIG_MSM_RPM_LOG_MODULE)
	&msm_rpm_log_device,
#endif
#if defined(CONFIG_MSM_RPM_STATS_LOG)
	&msm_rpm_stat_device,
#endif
	&msm_device_vidc,
#ifdef CONFIG_SENSORS_MSM_ADC
	&msm_adc_device,
#endif
#ifdef CONFIG_HTC_BATT8x60
	&htc_battery_pdev,
#endif
	&pm8058_leds,
	&rpm_regulator_device,
#ifdef CONFIG_HW_RANDOM_MSM
	&msm_device_rng,
#endif
	&msm_tsens_device,
	&msm_rpm_device,
	&cable_detect_device,
#ifdef CONFIG_ION_MSM
	&ion_dev,
#endif
	&msm8660_device_watchdog,
};

#ifdef CONFIG_ION_MSM
#ifdef CONFIG_MSM_MULTIMEDIA_USE_ION
static struct ion_cp_heap_pdata cp_mm_ion_pdata = {
	.permission_type = IPT_TYPE_MM_CARVEOUT,
	.align = PAGE_SIZE,
	.request_region = request_smi_region,
	.release_region = release_smi_region,
	.setup_region = setup_smi_region,
};

static struct ion_cp_heap_pdata cp_mfc_ion_pdata = {
	.permission_type = IPT_TYPE_MFC_SHAREDMEM,
	.align = PAGE_SIZE,
	.request_region = request_smi_region,
	.release_region = release_smi_region,
	.setup_region = setup_smi_region,
};

static struct ion_cp_heap_pdata cp_wb_ion_pdata = {
	.permission_type = IPT_TYPE_MDP_WRITEBACK,
	.align = PAGE_SIZE,
};

static struct ion_co_heap_pdata fw_co_ion_pdata = {
	.adjacent_mem_id = ION_CP_MM_HEAP_ID,
	.align = SZ_128K,
};

static struct ion_co_heap_pdata co_ion_pdata = {
	.adjacent_mem_id = INVALID_HEAP_ID,
	.align = PAGE_SIZE,
};
#endif

/**
 * These heaps are listed in the order they will be allocated. Due to
 * video hardware restrictions and content protection the FW heap has to
 * be allocated adjacent (below) the MM heap and the MFC heap has to be
 * allocated after the MM heap to ensure MFC heap is not more than 256MB
 * away from the base address of the FW heap.
 * However, the order of FW heap and MM heap doesn't matter since these
 * two heaps are taken care of by separate code to ensure they are adjacent
 * to each other.
 * Don't swap the order unless you know what you are doing!
 */
static struct ion_platform_data ion_pdata = {
	.nr = MSM_ION_HEAP_NUM,
	.heaps = {
		{
			.id	= ION_SYSTEM_HEAP_ID,
			.type	= ION_HEAP_TYPE_SYSTEM,
			.name	= ION_VMALLOC_HEAP_NAME,
		},
#ifdef CONFIG_MSM_MULTIMEDIA_USE_ION
		{
			.id	= ION_CP_MM_HEAP_ID,
			.type	= ION_HEAP_TYPE_CP,
			.name	= ION_MM_HEAP_NAME,
			.size	= MSM_ION_MM_SIZE,
			.memory_type = ION_SMI_TYPE,
			.extra_data = (void *) &cp_mm_ion_pdata,
		},
		{
			.id	= ION_MM_FIRMWARE_HEAP_ID,
			.type	= ION_HEAP_TYPE_CARVEOUT,
			.name	= ION_MM_FIRMWARE_HEAP_NAME,
			.size	= MSM_ION_MM_FW_SIZE,
			.memory_type = ION_SMI_TYPE,
			.extra_data = (void *) &fw_co_ion_pdata,
		},
		{
			.id	= ION_CP_MFC_HEAP_ID,
			.type	= ION_HEAP_TYPE_CP,
			.name	= ION_MFC_HEAP_NAME,
			.size	= MSM_ION_MFC_SIZE,
			.memory_type = ION_SMI_TYPE,
			.extra_data = (void *) &cp_mfc_ion_pdata,
		},
		{
			.id	= ION_SF_HEAP_ID,
			.type	= ION_HEAP_TYPE_CARVEOUT,
			.name	= ION_SF_HEAP_NAME,
			.size	= MSM_ION_SF_SIZE,
			.memory_type = ION_EBI_TYPE,
			.extra_data = (void *)&co_ion_pdata,
		},
		{
			.id	= ION_CAMERA_HEAP_ID,
			.type	= ION_HEAP_TYPE_CARVEOUT,
			.name	= ION_CAMERA_HEAP_NAME,
			.size	= MSM_ION_CAMERA_SIZE,
			.memory_type = ION_EBI_TYPE,
			.extra_data = &co_ion_pdata,
		},
		{
			.id	= ION_CP_WB_HEAP_ID,
			.type	= ION_HEAP_TYPE_CP,
			.name	= ION_WB_HEAP_NAME,
			.size	= MSM_ION_WB_SIZE,
			.memory_type = ION_EBI_TYPE,
			.extra_data = (void *) &cp_wb_ion_pdata,
		},
		{
			.id	= ION_QSECOM_HEAP_ID,
			.type	= ION_HEAP_TYPE_CARVEOUT,
			.name	= ION_QSECOM_HEAP_NAME,
			.size	= MSM_ION_QSECOM_SIZE,
			.memory_type = ION_EBI_TYPE,
			.extra_data = (void *) &co_ion_pdata,
		},
		{
			.id	= ION_AUDIO_HEAP_ID,
			.id	= ION_AUDIO_HEAP_ID,
			.type	= ION_HEAP_TYPE_CARVEOUT,
			.name	= ION_AUDIO_HEAP_NAME,
			.size	= MSM_ION_AUDIO_SIZE,
			.memory_type = ION_EBI_TYPE,
			.extra_data = (void *)&co_ion_pdata,
		},
#endif
	}
};

static struct platform_device ion_dev = {
	.name = "ion-msm",
	.id = 1,
	.dev = { .platform_data = &ion_pdata },
};
#endif


static struct memtype_reserve msm8x60_reserve_table[] __initdata = {
	/* Kernel SMI memory pool for video core, used for firmware */
	/* and encoder, decoder scratch buffers */
	/* Kernel SMI memory pool should always precede the user space */
	/* SMI memory pool, as the video core will use offset address */
	/* from the Firmware base */
	[MEMTYPE_SMI_KERNEL] = {
		.start	=	KERNEL_SMI_BASE,
		.limit	=	KERNEL_SMI_SIZE,
		.size	=	KERNEL_SMI_SIZE,
		.flags	=	MEMTYPE_FLAGS_FIXED,
	},
	/* User space SMI memory pool for video core */
	/* used for encoder, decoder input & output buffers  */
	[MEMTYPE_SMI] = {
		.start	=	USER_SMI_BASE,
		.limit	=	USER_SMI_SIZE,
		.flags	=	MEMTYPE_FLAGS_FIXED,
	},
	[MEMTYPE_EBI0] = {
		.flags	=	MEMTYPE_FLAGS_1M_ALIGN,
	},
	[MEMTYPE_EBI1] = {
		.flags	=	MEMTYPE_FLAGS_1M_ALIGN,
	},
};

static void reserve_ion_memory(void)
{
#if defined(CONFIG_ION_MSM) && defined(CONFIG_MSM_MULTIMEDIA_USE_ION)
	msm8x60_reserve_table[MEMTYPE_EBI1].size += MSM_ION_SF_SIZE;
	msm8x60_reserve_table[MEMTYPE_SMI].size += MSM_ION_MM_FW_SIZE;
	msm8x60_reserve_table[MEMTYPE_SMI].size += MSM_ION_MM_SIZE;
	msm8x60_reserve_table[MEMTYPE_SMI].size += MSM_ION_MFC_SIZE;
	msm8x60_reserve_table[MEMTYPE_EBI1].size += MSM_ION_CAMERA_SIZE;
	msm8x60_reserve_table[MEMTYPE_EBI1].size += MSM_ION_WB_SIZE;
	msm8x60_reserve_table[MEMTYPE_EBI1].size += MSM_ION_AUDIO_SIZE;
#endif
}

static void __init size_pmem_devices(void)
{
#ifdef CONFIG_ANDROID_PMEM
#ifndef CONFIG_MSM_MULTIMEDIA_USE_ION
	android_pmem_adsp_pdata.size = pmem_adsp_size;
	android_pmem_smipool_pdata.size = MSM_PMEM_SMIPOOL_SIZE;
	android_pmem_pdata.size = pmem_sf_size;
#endif
	android_pmem_audio_pdata.size = MSM_PMEM_AUDIO_SIZE;
#endif
}

static void __init reserve_memory_for(struct android_pmem_platform_data *p)
{
	msm8x60_reserve_table[p->memory_type].size += p->size;
}

static void __init reserve_pmem_memory(void)
{
#ifdef CONFIG_ANDROID_PMEM
#ifndef CONFIG_MSM_MULTIMEDIA_USE_ION
	reserve_memory_for(&android_pmem_adsp_pdata);
	reserve_memory_for(&android_pmem_smipool_pdata);
	reserve_memory_for(&android_pmem_pdata);
#endif
	reserve_memory_for(&android_pmem_audio_pdata);
	msm8x60_reserve_table[MEMTYPE_EBI1].size += pmem_kernel_ebi1_size;
#endif
}

static void __init reserve_mdp_memory(void)
{
	msm8x60_mdp_writeback(msm8x60_reserve_table);
}

static void __init msm8x60_calculate_reserve_sizes(void)
{
	size_pmem_devices();
	reserve_pmem_memory();
	reserve_ion_memory();
	reserve_mdp_memory();
}

static int msm8x60_paddr_to_memtype(unsigned int paddr)
{
	if (paddr >= 0x40000000 && paddr < 0x60000000)
		return MEMTYPE_EBI1;
	if (paddr >= 0x38000000 && paddr < 0x40000000)
		return MEMTYPE_SMI;
	return MEMTYPE_NONE;
}

static struct reserve_info msm8x60_reserve_info __initdata = {
	.memtype_reserve_table = msm8x60_reserve_table,
	.calculate_reserve_sizes = msm8x60_calculate_reserve_sizes,
	.paddr_to_memtype = msm8x60_paddr_to_memtype,
};

static void __init msm8x60_reserve(void)
{
	reserve_info = &msm8x60_reserve_info;
	msm_reserve();
}

#ifdef CONFIG_I2C_QUP
static uint32_t gsbi4_gpio_table[] = {
	GPIO_CFG(SHOOTER_CAM_I2C_SDA, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
	GPIO_CFG(SHOOTER_CAM_I2C_SCL, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
};

static uint32_t gsbi5_gpio_table[] = {
	GPIO_CFG(SHOOTER_TP_I2C_SDA, 1, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
	GPIO_CFG(SHOOTER_TP_I2C_SCL, 1, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
};

static uint32_t gsbi7_gpio_table[] = {
	GPIO_CFG(SHOOTER_GENERAL_I2C_SDA, 1, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
	GPIO_CFG(SHOOTER_GENERAL_I2C_SCL, 1, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
};

static uint32_t gsbi10_gpio_table[] = {
	GPIO_CFG(SHOOTER_SENSOR_I2C_SDA, 1, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIOMUX_DRV_8MA),
	GPIO_CFG(SHOOTER_SENSOR_I2C_SCL, 1, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIOMUX_DRV_8MA),
};


static void gsbi_qup_i2c_gpio_config(int adap_id, int config_type)
{
	printk(KERN_INFO "%s(): adap_id = %d, config_type = %d \n", __func__,adap_id,config_type);

	if ((adap_id == MSM_GSBI4_QUP_I2C_BUS_ID) && (config_type == 1)) {
		gpio_tlmm_config(gsbi4_gpio_table[0], GPIO_CFG_ENABLE);
		gpio_tlmm_config(gsbi4_gpio_table[1], GPIO_CFG_ENABLE);
	}

	if ((adap_id == MSM_GSBI4_QUP_I2C_BUS_ID) && (config_type == 0)) {
		gpio_tlmm_config(gsbi4_gpio_table[0], GPIO_CFG_DISABLE);
		gpio_tlmm_config(gsbi4_gpio_table[1], GPIO_CFG_DISABLE);
	}

	if ((adap_id == MSM_GSBI5_QUP_I2C_BUS_ID) && (config_type == 1)) {
		gpio_tlmm_config(gsbi5_gpio_table[0], GPIO_CFG_ENABLE);
		gpio_tlmm_config(gsbi5_gpio_table[1], GPIO_CFG_ENABLE);
	}

	if ((adap_id == MSM_GSBI5_QUP_I2C_BUS_ID) && (config_type == 0)) {
		gpio_tlmm_config(gsbi5_gpio_table[0], GPIO_CFG_DISABLE);
		gpio_tlmm_config(gsbi5_gpio_table[1], GPIO_CFG_DISABLE);
	}

	if ((adap_id == MSM_GSBI7_QUP_I2C_BUS_ID) && (config_type == 1)) {
		gpio_tlmm_config(gsbi7_gpio_table[0], GPIO_CFG_ENABLE);
		gpio_tlmm_config(gsbi7_gpio_table[1], GPIO_CFG_ENABLE);
	}

	if ((adap_id == MSM_GSBI7_QUP_I2C_BUS_ID) && (config_type == 0)) {
		gpio_tlmm_config(gsbi7_gpio_table[0], GPIO_CFG_DISABLE);
		gpio_tlmm_config(gsbi7_gpio_table[1], GPIO_CFG_DISABLE);
	}

	if ((adap_id == MSM_GSBI10_QUP_I2C_BUS_ID) && (config_type == 1)) {
		gpio_tlmm_config(gsbi10_gpio_table[0], GPIO_CFG_ENABLE);
		gpio_tlmm_config(gsbi10_gpio_table[1], GPIO_CFG_ENABLE);
	}

	if ((adap_id == MSM_GSBI10_QUP_I2C_BUS_ID) && (config_type == 0)) {
		gpio_tlmm_config(gsbi10_gpio_table[0], GPIO_CFG_DISABLE);
		gpio_tlmm_config(gsbi10_gpio_table[1], GPIO_CFG_DISABLE);
	}

}

static struct msm_i2c_platform_data msm_gsbi4_qup_i2c_pdata = {
	.clk_freq = 384000,
	.src_clk_rate = 24000000,
	.msm_i2c_config_gpio = gsbi_qup_i2c_gpio_config,
};

static struct msm_i2c_platform_data msm_gsbi5_qup_i2c_pdata = {
	.clk_freq = 100000,
	.src_clk_rate = 24000000,
	.msm_i2c_config_gpio = gsbi_qup_i2c_gpio_config,
};

static struct msm_i2c_platform_data msm_gsbi7_qup_i2c_pdata = {
	.clk_freq = 100000,
	.src_clk_rate = 24000000,
	.msm_i2c_config_gpio = gsbi_qup_i2c_gpio_config,
};

static struct msm_i2c_platform_data msm_gsbi10_qup_i2c_pdata = {
	.clk_freq = 384000,
	.src_clk_rate = 24000000,
	.msm_i2c_config_gpio = gsbi_qup_i2c_gpio_config,
};
#endif

#if defined(CONFIG_SPI_QUP) || defined(CONFIG_SPI_QUP_MODULE)
static struct msm_spi_platform_data msm_gsbi1_qup_spi_pdata = {
	.max_clock_speed = 10800000,
};

#ifdef CONFIG_MACH_SHOOTER
static struct msm_spi_platform_data msm_gsbi2_qup_spi_pdata = {
#else
static struct msm_spi_platform_data msm_gsbi3_qup_spi_pdata = {
#endif
	.max_clock_speed = 15060000,
};
#endif

#ifdef CONFIG_I2C_SSBI
/* CODEC/TSSC SSBI */
static struct msm_i2c_ssbi_platform_data msm_ssbi3_pdata = {
	.controller_type = MSM_SBI_CTRL_SSBI,
};
#endif

static void __init msm8x60_map_io(void)
{
	msm_shared_ram_phys = MSM_SHARED_RAM_PHYS;
	msm_map_msm8x60_io();

	if (socinfo_init() < 0)
		printk(KERN_ERR "%s: socinfo_init() failed!",   __func__);
}

#define GPIO_SDC_WP (GPIO_EXPANDER_GPIO_BASE + (16 * 1) + 6)
#if (defined(CONFIG_MMC_MSM_SDC1_SUPPORT)\
	|| defined(CONFIG_MMC_MSM_SDC2_SUPPORT)\
	|| defined(CONFIG_MMC_MSM_SDC3_SUPPORT)\
	|| defined(CONFIG_MMC_MSM_SDC4_SUPPORT)\
	|| defined(CONFIG_MMC_MSM_SDC5_SUPPORT))

/* 8x60 has 5 SDCC controllers */
#define MAX_SDCC_CONTROLLER	5

struct msm_sdcc_gpio {
	/* maximum 10 GPIOs per SDCC controller */
	s16 no;
	/* name of this GPIO */
	const char *name;
	bool always_on;
	bool is_enabled;
};

#ifdef CONFIG_MMC_MSM_SDC1_SUPPORT
static struct msm_sdcc_gpio sdc1_gpio_cfg[] = {
	{159, "sdc1_dat_0"},
	{160, "sdc1_dat_1"},
	{161, "sdc1_dat_2"},
	{162, "sdc1_dat_3"},
#ifdef CONFIG_MMC_MSM_SDC1_8_BIT_SUPPORT
	{163, "sdc1_dat_4"},
	{164, "sdc1_dat_5"},
	{165, "sdc1_dat_6"},
	{166, "sdc1_dat_7"},
#endif
	{167, "sdc1_clk"},
	{168, "sdc1_cmd"}
};

#ifdef CONFIG_HTC_MMC
static uint32_t sdc1_on_gpio_table[] = {
	GPIO_CFG(159, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_10MA), /* DAT0 */
	GPIO_CFG(160, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_10MA), /* DAT1 */
	GPIO_CFG(161, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_10MA), /* DAT2 */
	GPIO_CFG(162, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_10MA), /* DAT3 */
#ifdef CONFIG_MMC_MSM_SDC1_8_BIT_SUPPORT
	GPIO_CFG(163, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_10MA), /* DAT4 */
	GPIO_CFG(164, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_10MA), /* DAT5 */
	GPIO_CFG(165, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_10MA), /* DAT6 */
	GPIO_CFG(166, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_10MA), /* DAT7 */
#endif
	GPIO_CFG(167, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_14MA), /* CLK */
	GPIO_CFG(168, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_10MA), /* CMD */
};

static void config_gpio_table(uint32_t *table, int len)
{
	int n, rc;
	for (n = 0; n < len; n++) {
		rc = gpio_tlmm_config(table[n], GPIO_CFG_ENABLE);
		if (rc) {
			pr_err("[MMC] %s: gpio_tlmm_config(%#x)=%d\n",
				__func__, table[n], rc);
			break;
		}
	}
}
#endif
#endif

#ifdef CONFIG_MMC_MSM_SDC2_SUPPORT
static struct msm_sdcc_gpio sdc2_gpio_cfg[] = {
	{143, "sdc2_dat_0"},
	{144, "sdc2_dat_1", 1},
	{145, "sdc2_dat_2"},
	{146, "sdc2_dat_3"},
#ifdef CONFIG_MMC_MSM_SDC2_8_BIT_SUPPORT
	{147, "sdc2_dat_4"},
	{148, "sdc2_dat_5"},
	{149, "sdc2_dat_6"},
	{150, "sdc2_dat_7"},
#endif
	{151, "sdc2_cmd"},
	{152, "sdc2_clk", 1}
};
#endif

struct msm_sdcc_pad_pull_cfg {
	enum msm_tlmm_pull_tgt pull;
	u32 pull_val;
};

struct msm_sdcc_pad_drv_cfg {
	enum msm_tlmm_hdrive_tgt drv;
	u32 drv_val;
};

#ifdef CONFIG_MMC_MSM_SDC3_SUPPORT
static struct msm_sdcc_pad_drv_cfg sdc3_pad_on_drv_cfg[] = {
	{TLMM_HDRV_SDC3_CLK, GPIO_CFG_8MA},
	{TLMM_HDRV_SDC3_CMD, GPIO_CFG_8MA},
	{TLMM_HDRV_SDC3_DATA, GPIO_CFG_8MA}
};

static struct msm_sdcc_pad_pull_cfg sdc3_pad_on_pull_cfg[] = {
	{TLMM_PULL_SDC3_CMD, GPIO_CFG_PULL_UP},
	{TLMM_PULL_SDC3_DATA, GPIO_CFG_PULL_UP}
};

static struct msm_sdcc_pad_drv_cfg sdc3_pad_off_drv_cfg[] = {
	{TLMM_HDRV_SDC3_CLK, GPIO_CFG_2MA},
	{TLMM_HDRV_SDC3_CMD, GPIO_CFG_2MA},
	{TLMM_HDRV_SDC3_DATA, GPIO_CFG_2MA}
};

static struct msm_sdcc_pad_pull_cfg sdc3_pad_off_pull_cfg[] = {
	{TLMM_PULL_SDC3_CMD, GPIO_CFG_PULL_DOWN},
	{TLMM_PULL_SDC3_DATA, GPIO_CFG_PULL_DOWN}
};
#endif

#ifdef CONFIG_MMC_MSM_SDC4_SUPPORT
static struct msm_sdcc_pad_drv_cfg sdc4_pad_on_drv_cfg[] = {
	{TLMM_HDRV_SDC4_CLK, GPIO_CFG_8MA},
	{TLMM_HDRV_SDC4_CMD, GPIO_CFG_8MA},
	{TLMM_HDRV_SDC4_DATA, GPIO_CFG_8MA}
};

static struct msm_sdcc_pad_pull_cfg sdc4_pad_on_pull_cfg[] = {
	{TLMM_PULL_SDC4_CMD, GPIO_CFG_PULL_UP},
	{TLMM_PULL_SDC4_DATA, GPIO_CFG_PULL_UP}
};

static struct msm_sdcc_pad_drv_cfg sdc4_pad_off_drv_cfg[] = {
	{TLMM_HDRV_SDC4_CLK, GPIO_CFG_2MA},
	{TLMM_HDRV_SDC4_CMD, GPIO_CFG_2MA},
	{TLMM_HDRV_SDC4_DATA, GPIO_CFG_2MA}
};

static struct msm_sdcc_pad_pull_cfg sdc4_pad_off_pull_cfg[] = {
	{TLMM_PULL_SDC4_CMD, GPIO_CFG_PULL_DOWN},
	{TLMM_PULL_SDC4_DATA, GPIO_CFG_PULL_DOWN}
};
#endif

struct msm_sdcc_pin_cfg {
	/*
	 * = 1 if controller pins are using gpios
	 * = 0 if controller has dedicated MSM pins
	 */
	u8 is_gpio;
	u8 cfg_sts;
	u8 gpio_data_size;
	struct msm_sdcc_gpio *gpio_data;
	struct msm_sdcc_pad_drv_cfg *pad_drv_on_data;
	struct msm_sdcc_pad_drv_cfg *pad_drv_off_data;
	struct msm_sdcc_pad_pull_cfg *pad_pull_on_data;
	struct msm_sdcc_pad_pull_cfg *pad_pull_off_data;
	u8 pad_drv_data_size;
	u8 pad_pull_data_size;
	u8 sdio_lpm_gpio_cfg;
};

static struct msm_sdcc_pin_cfg sdcc_pin_cfg_data[MAX_SDCC_CONTROLLER] = {
#ifdef CONFIG_MMC_MSM_SDC1_SUPPORT
	[0] = {
		.is_gpio = 1,
		.gpio_data_size = ARRAY_SIZE(sdc1_gpio_cfg),
		.gpio_data = sdc1_gpio_cfg
	},
#endif
#ifdef CONFIG_MMC_MSM_SDC2_SUPPORT
	[1] = {
		.is_gpio = 1,
		.gpio_data_size = ARRAY_SIZE(sdc2_gpio_cfg),
		.gpio_data = sdc2_gpio_cfg
	},
#endif
#ifdef CONFIG_MMC_MSM_SDC3_SUPPORT
	[2] = {
		.is_gpio = 0,
		.pad_drv_on_data = sdc3_pad_on_drv_cfg,
		.pad_drv_off_data = sdc3_pad_off_drv_cfg,
		.pad_pull_on_data = sdc3_pad_on_pull_cfg,
		.pad_pull_off_data = sdc3_pad_off_pull_cfg,
		.pad_drv_data_size = ARRAY_SIZE(sdc3_pad_on_drv_cfg),
		.pad_pull_data_size = ARRAY_SIZE(sdc3_pad_on_pull_cfg)
	},
#endif
#ifdef CONFIG_MMC_MSM_SDC4_SUPPORT
	[3] = {
		.is_gpio = 0,
		.pad_drv_on_data = sdc4_pad_on_drv_cfg,
		.pad_drv_off_data = sdc4_pad_off_drv_cfg,
		.pad_pull_on_data = sdc4_pad_on_pull_cfg,
		.pad_pull_off_data = sdc4_pad_off_pull_cfg,
		.pad_drv_data_size = ARRAY_SIZE(sdc4_pad_on_drv_cfg),
		.pad_pull_data_size = ARRAY_SIZE(sdc4_pad_on_pull_cfg)
	},
#endif
#ifdef CONFIG_MMC_MSM_SDC5_SUPPORT
	[4] = {
		.is_gpio = 1,
		.gpio_data_size = ARRAY_SIZE(sdc5_gpio_cfg),
		.gpio_data = sdc5_gpio_cfg
	}
#endif
};

static int msm_sdcc_setup_gpio(int dev_id, unsigned int enable)
{
	int rc = 0;
	struct msm_sdcc_pin_cfg *curr;
	int n;

	curr = &sdcc_pin_cfg_data[dev_id - 1];
	if (!curr->gpio_data)
		goto out;

	for (n = 0; n < curr->gpio_data_size; n++) {
		if (enable) {

			if (curr->gpio_data[n].always_on &&
				curr->gpio_data[n].is_enabled)
				continue;
			pr_debug("%s: enable: %s\n", __func__,
					curr->gpio_data[n].name);
			rc = gpio_request(curr->gpio_data[n].no,
				curr->gpio_data[n].name);
			if (rc) {
				pr_err("%s: gpio_request(%d, %s)"
					"failed", __func__,
					curr->gpio_data[n].no,
					curr->gpio_data[n].name);
				goto free_gpios;
			}
			/* set direction as output for all GPIOs */
			rc = gpio_direction_output(
				curr->gpio_data[n].no, 1);
			if (rc) {
				pr_err("%s: gpio_direction_output"
					"(%d, 1) failed\n", __func__,
					curr->gpio_data[n].no);
				goto free_gpios;
			}
			curr->gpio_data[n].is_enabled = 1;
		} else {
			/*
			 * now free this GPIO which will put GPIO
			 * in low power mode and will also put GPIO
			 * in input mode
			 */
			if (curr->gpio_data[n].always_on)
				continue;
			pr_debug("%s: disable: %s\n", __func__,
					curr->gpio_data[n].name);
			gpio_free(curr->gpio_data[n].no);
			curr->gpio_data[n].is_enabled = 0;
		}
	}
	curr->cfg_sts = enable;
	goto out;

free_gpios:
	for (; n >= 0; n--)
		gpio_free(curr->gpio_data[n].no);
out:
	return rc;
}

static int msm_sdcc_setup_pad(int dev_id, unsigned int enable)
{
	int rc = 0;
	struct msm_sdcc_pin_cfg *curr;
	int n;

	curr = &sdcc_pin_cfg_data[dev_id - 1];
	if (!curr->pad_drv_on_data || !curr->pad_pull_on_data)
		goto out;

	if (enable) {
		/*
		 * set up the normal driver strength and
		 * pull config for pads
		 */
		for (n = 0; n < curr->pad_drv_data_size; n++) {
			if (curr->sdio_lpm_gpio_cfg) {
				if (curr->pad_drv_on_data[n].drv ==
						TLMM_HDRV_SDC4_DATA)
					continue;
			}
			msm_tlmm_set_hdrive(curr->pad_drv_on_data[n].drv,
				curr->pad_drv_on_data[n].drv_val);
		}
		for (n = 0; n < curr->pad_pull_data_size; n++) {
			if (curr->sdio_lpm_gpio_cfg) {
				if (curr->pad_pull_on_data[n].pull ==
						TLMM_PULL_SDC4_DATA)
					continue;
			}
			msm_tlmm_set_pull(curr->pad_pull_on_data[n].pull,
				curr->pad_pull_on_data[n].pull_val);
		}
	} else {
		/* set the low power config for pads */
		for (n = 0; n < curr->pad_drv_data_size; n++) {
			if (curr->sdio_lpm_gpio_cfg) {
				if (curr->pad_drv_off_data[n].drv ==
						TLMM_HDRV_SDC4_DATA)
					continue;
			}
			msm_tlmm_set_hdrive(
				curr->pad_drv_off_data[n].drv,
				curr->pad_drv_off_data[n].drv_val);
		}
		for (n = 0; n < curr->pad_pull_data_size; n++) {
			if (curr->sdio_lpm_gpio_cfg) {
				if (curr->pad_pull_off_data[n].pull ==
						TLMM_PULL_SDC4_DATA)
					continue;
			}
			msm_tlmm_set_pull(
				curr->pad_pull_off_data[n].pull,
				curr->pad_pull_off_data[n].pull_val);
		}
	}
	curr->cfg_sts = enable;
out:
	return rc;
}

struct sdcc_reg {
	/* VDD/VCC/VCCQ regulator name on PMIC8058/PMIC8089*/
	const char *reg_name;
	/*
	 * is set voltage supported for this regulator?
	 * 0 = not supported, 1 = supported
	 */
	unsigned char set_voltage_sup;
	/* voltage level to be set */
	unsigned int level;
	/* VDD/VCC/VCCQ voltage regulator handle */
	struct regulator *reg;
	/* is this regulator enabled? */
	bool enabled;
	/* is this regulator needs to be always on? */
	bool always_on;
	/* is operating power mode setting required for this regulator? */
	bool op_pwr_mode_sup;
	/* Load values for low power and high power mode */
	unsigned int lpm_uA;
	unsigned int hpm_uA;
};
/* all SDCC controllers require VDD/VCC voltage */
static struct sdcc_reg sdcc_vdd_reg_data[MAX_SDCC_CONTROLLER];
/* only SDCC1 requires VCCQ voltage */
static struct sdcc_reg sdcc_vccq_reg_data[1];
#ifdef CONFIG_MMC_MSM_SDC3_SUPPORT
/* all SDCC controllers may require voting for VDD PAD voltage */
static struct sdcc_reg sdcc_vddp_reg_data[MAX_SDCC_CONTROLLER];
#endif

struct sdcc_reg_data {
	struct sdcc_reg *vdd_data; /* keeps VDD/VCC regulator info */
	struct sdcc_reg *vccq_data; /* keeps VCCQ regulator info */
	struct sdcc_reg *vddp_data; /* keeps VDD Pad regulator info */
	unsigned char sts; /* regulator enable/disable status */
};
/* msm8x60 has 5 SDCC controllers */
static struct sdcc_reg_data sdcc_vreg_data[MAX_SDCC_CONTROLLER];

static int msm_sdcc_vreg_init_reg(struct sdcc_reg *vreg)
{
	int rc = 0;

	/* Get the regulator handle */
	vreg->reg = regulator_get(NULL, vreg->reg_name);
	if (IS_ERR(vreg->reg)) {
		rc = PTR_ERR(vreg->reg);
		pr_err("%s: regulator_get(%s) failed. rc=%d\n",
			__func__, vreg->reg_name, rc);
		goto out;
	}

	/* Set the voltage level if required */
	if (vreg->set_voltage_sup) {
		rc = regulator_set_voltage(vreg->reg, vreg->level,
					vreg->level);
		if (rc) {
			pr_err("%s: regulator_set_voltage(%s) failed rc=%d\n",
				__func__, vreg->reg_name, rc);
			goto vreg_put;
		}
	}
	goto out;

vreg_put:
	regulator_put(vreg->reg);
out:
	return rc;
}

static inline void msm_sdcc_vreg_deinit_reg(struct sdcc_reg *vreg)
{
	regulator_put(vreg->reg);
}

/* this init function should be called only once for each SDCC */
static int msm_sdcc_vreg_init(int dev_id, unsigned char init)
{
	int rc = 0;
	struct sdcc_reg *curr_vdd_reg, *curr_vccq_reg, *curr_vddp_reg;
	struct sdcc_reg_data *curr;

	curr = &sdcc_vreg_data[dev_id - 1];
	curr_vdd_reg = curr->vdd_data;
	curr_vccq_reg = curr->vccq_data;
	curr_vddp_reg = curr->vddp_data;

	if (init) {
		/*
		 * get the regulator handle from voltage regulator framework
		 * and then try to set the voltage level for the regulator
		 */
		if (curr_vdd_reg) {
			rc = msm_sdcc_vreg_init_reg(curr_vdd_reg);
			if (rc)
				goto out;
		}
		if (curr_vccq_reg) {
			rc = msm_sdcc_vreg_init_reg(curr_vccq_reg);
			if (rc)
				goto vdd_reg_deinit;
		}
		if (curr_vddp_reg) {
			rc = msm_sdcc_vreg_init_reg(curr_vddp_reg);
			if (rc)
				goto vccq_reg_deinit;
		}
		goto out;
	} else
		/* deregister with all regulators from regulator framework */
		goto vddp_reg_deinit;

vddp_reg_deinit:
	if (curr_vddp_reg)
		msm_sdcc_vreg_deinit_reg(curr_vddp_reg);
vccq_reg_deinit:
	if (curr_vccq_reg)
		msm_sdcc_vreg_deinit_reg(curr_vccq_reg);
vdd_reg_deinit:
	if (curr_vdd_reg)
		msm_sdcc_vreg_deinit_reg(curr_vdd_reg);
out:
	return rc;
}

static int msm_sdcc_vreg_enable(struct sdcc_reg *vreg)
{
	int rc;

	if (!vreg->enabled) {
		rc = regulator_enable(vreg->reg);
		if (rc) {
			pr_err("%s: regulator_enable(%s) failed. rc=%d\n",
				__func__, vreg->reg_name, rc);
			goto out;
		}
		vreg->enabled = 1;
	}

	/* Put always_on regulator in HPM (high power mode) */
	if (vreg->always_on && vreg->op_pwr_mode_sup) {
		rc = regulator_set_optimum_mode(vreg->reg, vreg->hpm_uA);
		if (rc < 0) {
			pr_err("%s: reg=%s: HPM setting failed"
				" hpm_uA=%d, rc=%d\n",
				__func__, vreg->reg_name,
				vreg->hpm_uA, rc);
			goto vreg_disable;
		}
		rc = 0;
	}
	goto out;

vreg_disable:
	regulator_disable(vreg->reg);
	vreg->enabled = 0;
out:
	return rc;
}

static int msm_sdcc_vreg_disable(struct sdcc_reg *vreg)
{
	int rc;

	/* Never disable always_on regulator */
	if (!vreg->always_on) {
		rc = regulator_disable(vreg->reg);
		if (rc) {
			pr_err("%s: regulator_disable(%s) failed. rc=%d\n",
				__func__, vreg->reg_name, rc);
			goto out;
		}
		vreg->enabled = 0;
	}

	/* Put always_on regulator in LPM (low power mode) */
	if (vreg->always_on && vreg->op_pwr_mode_sup) {
		rc = regulator_set_optimum_mode(vreg->reg, vreg->lpm_uA);
		if (rc < 0) {
			pr_err("%s: reg=%s: LPM setting failed"
				" lpm_uA=%d, rc=%d\n",
				__func__,
				vreg->reg_name,
				vreg->lpm_uA, rc);
			goto out;
		}
		rc = 0;
	}

out:
	return rc;
}

static int msm_sdcc_setup_vreg(int dev_id, unsigned char enable)
{
	int rc = 0;
	struct sdcc_reg *curr_vdd_reg, *curr_vccq_reg, *curr_vddp_reg;
	struct sdcc_reg_data *curr;

	curr = &sdcc_vreg_data[dev_id - 1];
	curr_vdd_reg = curr->vdd_data;
#ifndef CONFIG_HTC_MMC
	curr_vccq_reg = curr->vccq_data;
#endif
	curr_vddp_reg = curr->vddp_data;

	/* check if regulators are initialized or not? */
	if ((curr_vdd_reg && !curr_vdd_reg->reg) ||
		(curr_vccq_reg && !curr_vccq_reg->reg) ||
		(curr_vddp_reg && !curr_vddp_reg->reg)) {
		/* initialize voltage regulators required for this SDCC */
		rc = msm_sdcc_vreg_init(dev_id, 1);
		if (rc) {
			pr_err("%s: regulator init failed = %d\n",
				__func__, rc);
			goto out;
		}
	}

	if (curr->sts == enable)
		goto out;

	mdelay(5);
	if (curr_vdd_reg) {
		if (enable)
			rc = msm_sdcc_vreg_enable(curr_vdd_reg);
		else
			rc = msm_sdcc_vreg_disable(curr_vdd_reg);
		if (rc)
			goto out;
	}

	if (curr_vccq_reg) {
		if (enable)
			rc = msm_sdcc_vreg_enable(curr_vccq_reg);
		else
			rc = msm_sdcc_vreg_disable(curr_vccq_reg);
		if (rc)
			goto out;
	}

	if (curr_vddp_reg) {
		if (enable)
			rc = msm_sdcc_vreg_enable(curr_vddp_reg);
		else
			rc = msm_sdcc_vreg_disable(curr_vddp_reg);
		if (rc)
			goto out;
	}
	curr->sts = enable;

out:
	return rc;
}

static u32 msm_sdcc_setup_power(struct device *dv, unsigned int vdd)
{
	u32 rc_pin_cfg = 0;
	u32 rc_vreg_cfg = 0;
	u32 rc = 0;
	struct platform_device *pdev;
	struct msm_sdcc_pin_cfg *curr_pin_cfg;

	pdev = container_of(dv, struct platform_device, dev);

	/* setup gpio/pad */
	curr_pin_cfg = &sdcc_pin_cfg_data[pdev->id - 1];
	if (curr_pin_cfg->cfg_sts == !!vdd)
		goto setup_vreg;

	if (curr_pin_cfg->is_gpio)
		rc_pin_cfg = msm_sdcc_setup_gpio(pdev->id, !!vdd);
	else
		rc_pin_cfg = msm_sdcc_setup_pad(pdev->id, !!vdd);

setup_vreg:
	/* setup voltage regulators */
	rc_vreg_cfg = msm_sdcc_setup_vreg(pdev->id, !!vdd);

	if (rc_pin_cfg || rc_vreg_cfg)
		rc = rc_pin_cfg ? rc_pin_cfg : rc_vreg_cfg;

	return rc;
}

#ifdef CONFIG_MMC_MSM_SDC3_SUPPORT
#ifdef CONFIG_MMC_MSM_SDC3_WP_SUPPORT
static int msm_sdc3_get_wpswitch(struct device *dev)
{
	struct platform_device *pdev;
	int status;
	pdev = container_of(dev, struct platform_device, dev);

	status = gpio_request(GPIO_SDC_WP, "SD_WP_Switch");
	if (status) {
		pr_err("%s:Failed to request GPIO %d\n",
					__func__, GPIO_SDC_WP);
	} else {
		status = gpio_direction_input(GPIO_SDC_WP);
		if (!status) {
			status = gpio_get_value_cansleep(GPIO_SDC_WP);
			pr_info("%s: WP Status for Slot %d = %d\n",
				 __func__, pdev->id, status);
		}
		gpio_free(GPIO_SDC_WP);
	}
	return status;
}
#endif
#endif

#ifdef CONFIG_MMC_MSM_SDC3_SUPPORT
#ifdef CONFIG_MMC_MSM_CARD_HW_DETECTION
static unsigned int msm8x60_sdcc_slot_status(struct device *dev)
{
	int status;

	status = gpio_request(PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_SDC3_DET - 1)
				, "SD_HW_Detect");
	if (status) {
		pr_err("%s:Failed to request GPIO %d\n", __func__,
				PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_SDC3_DET - 1));
	} else {
		status = gpio_direction_input(
				PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_SDC3_DET - 1));
		if (!status)
			status = !(gpio_get_value_cansleep(
				PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_SDC3_DET - 1)));
		gpio_free(PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_SDC3_DET - 1));
	}
	return (unsigned int) status;
}
#endif
#endif
#endif

#ifdef CONFIG_MMC_MSM_SDC1_SUPPORT
static unsigned int shooter_emmcslot_type = MMC_TYPE_MMC;
static struct mmc_platform_data msm8x60_sdc1_data = {
	.ocr_mask       = MMC_VDD_27_28 | MMC_VDD_28_29,
	.translate_vdd  = msm_sdcc_setup_power,
#ifdef CONFIG_MMC_MSM_SDC1_8_BIT_SUPPORT
	.mmc_bus_width  = MMC_CAP_8_BIT_DATA,
#else
	.mmc_bus_width  = MMC_CAP_4_BIT_DATA,
#endif
	.msmsdcc_fmin	= 400000,
	.msmsdcc_fmid	= 24000000,
	.msmsdcc_fmax	= 48000000,
	.nonremovable	= 1,
	.pclk_src_dfab	= 1,
	.disable_runtime_pm = 1,
	.slot_type	= &shooter_emmcslot_type,
};
#endif

#ifdef CONFIG_MMC_MSM_SDC3_SUPPORT
static unsigned int shooter_sdslot_type = MMC_TYPE_SD;
static struct mmc_platform_data msm8x60_sdc3_data = {
	.ocr_mask       = MMC_VDD_27_28 | MMC_VDD_28_29,
	.translate_vdd  = msm_sdcc_setup_power,
	.mmc_bus_width  = MMC_CAP_4_BIT_DATA,
#ifdef CONFIG_MMC_MSM_SDC3_WP_SUPPORT
	.wpswitch  	= msm_sdc3_get_wpswitch,
#endif
#ifdef CONFIG_MMC_MSM_CARD_HW_DETECTION
	.status      = msm8x60_sdcc_slot_status,
	.status_irq  = PM8058_GPIO_IRQ(PM8058_IRQ_BASE,
				       PMIC_GPIO_SDC3_DET - 1),
	.irq_flags   = IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
#endif
	.msmsdcc_fmin	= 400000,
	.msmsdcc_fmid	= 24000000,
	.msmsdcc_fmax	= 48000000,
	.nonremovable	= 0,
	.pclk_src_dfab  = 1,
	.disable_runtime_pm = 1,
	.slot_type	= &shooter_sdslot_type,
};
#endif

static uint32_t msm_rpm_get_swfi_latency(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(msm_rpmrs_levels); i++) {
		if (msm_rpmrs_levels[i].sleep_mode ==
			MSM_PM_SLEEP_MODE_WAIT_FOR_INTERRUPT)
			return msm_rpmrs_levels[i].latency_us;
	}

	return 0;
}

static void __init msm8x60_init_mmc(void)
{
#ifdef CONFIG_MMC_MSM_SDC1_SUPPORT
#ifdef CONFIG_HTC_MMC
	config_gpio_table(sdc1_on_gpio_table, ARRAY_SIZE(sdc1_on_gpio_table));
#endif

	/* SDCC1 : eMMC card connected */
	sdcc_vreg_data[0].vdd_data = &sdcc_vdd_reg_data[0];
	sdcc_vreg_data[0].vdd_data->reg_name = "8901_l5";
	sdcc_vreg_data[0].vdd_data->set_voltage_sup = 1;
	sdcc_vreg_data[0].vdd_data->level = 2850000;
	sdcc_vreg_data[0].vdd_data->always_on = 1;
	sdcc_vreg_data[0].vdd_data->op_pwr_mode_sup = 1;
	sdcc_vreg_data[0].vdd_data->lpm_uA = 9000;
	sdcc_vreg_data[0].vdd_data->hpm_uA = 200000;

	sdcc_vreg_data[0].vccq_data = &sdcc_vccq_reg_data[0];
	sdcc_vreg_data[0].vccq_data->reg_name = "8901_lvs0";
	sdcc_vreg_data[0].vccq_data->set_voltage_sup = 0;
	sdcc_vreg_data[0].vccq_data->always_on = 1;

	msm8x60_sdc1_data.swfi_latency = msm_rpm_get_swfi_latency();
	msm_add_sdcc(1, &msm8x60_sdc1_data);
#endif
#ifdef CONFIG_MMC_MSM_SDC3_SUPPORT
	/* SDCC3 : External card slot connected */
	sdcc_vreg_data[2].vdd_data = &sdcc_vdd_reg_data[2];
	sdcc_vreg_data[2].vdd_data->reg_name = "8058_l14";
	sdcc_vreg_data[2].vdd_data->set_voltage_sup = 1;
	sdcc_vreg_data[2].vdd_data->level = 2850000;
	sdcc_vreg_data[2].vdd_data->always_on = 0;
	sdcc_vreg_data[2].vdd_data->op_pwr_mode_sup = 0;
	sdcc_vreg_data[2].vdd_data->lpm_uA = 9000;
	sdcc_vreg_data[2].vdd_data->hpm_uA = 200000;

	sdcc_vreg_data[2].vccq_data = NULL;

	sdcc_vreg_data[2].vddp_data = &sdcc_vddp_reg_data[2];
	sdcc_vreg_data[2].vddp_data->reg_name = "8058_l5";
	sdcc_vreg_data[2].vddp_data->set_voltage_sup = 1;
	sdcc_vreg_data[2].vddp_data->level = 2850000;
	sdcc_vreg_data[2].vddp_data->always_on = 1;
	sdcc_vreg_data[2].vddp_data->op_pwr_mode_sup = 1;
	/* Sleep current required is ~300 uA. But min. RPM
	 * vote can be in terms of mA (min. 1 mA).
	 * So let's vote for 2 mA during sleep.
	 */
	sdcc_vreg_data[2].vddp_data->lpm_uA = 10000;
	/* Max. Active current required is 16 mA */
	sdcc_vreg_data[2].vddp_data->hpm_uA = 16000;

	msm_add_sdcc(3, &msm8x60_sdc3_data);
#endif
#ifdef CONFIG_MMC_MSM_SDC4_SUPPORT
	shooter_init_mmc();
#endif
}

#ifdef CONFIG_MSM_DSPS
#define DSPS_PIL_GENERIC_NAME		"dsps"

static struct dsps_gpio_info dsps_gpios[] = {};

static void __init msm8x60_init_dsps(void)
{
	struct msm_dsps_platform_data *pdata =
		msm_dsps_device.dev.platform_data;

	pdata->pil_name = DSPS_PIL_GENERIC_NAME;
	pdata->gpios = dsps_gpios;
	pdata->gpios_num = ARRAY_SIZE(dsps_gpios);

	platform_device_register(&msm_dsps_device);
}
#endif /* CONFIG_MSM_DSPS */

#ifdef CONFIG_MSM_RPM
static struct msm_rpm_platform_data msm_rpm_data = {
	.reg_base_addrs = {
		[MSM_RPM_PAGE_STATUS] = MSM_RPM_BASE,
		[MSM_RPM_PAGE_CTRL] = MSM_RPM_BASE + 0x400,
		[MSM_RPM_PAGE_REQ] = MSM_RPM_BASE + 0x600,
		[MSM_RPM_PAGE_ACK] = MSM_RPM_BASE + 0xa00,
	},

	.irq_ack = RPM_SCSS_CPU0_GP_HIGH_IRQ,
	.irq_err = RPM_SCSS_CPU0_GP_LOW_IRQ,
	.irq_vmpm = RPM_SCSS_CPU0_GP_MEDIUM_IRQ,
	.msm_apps_ipc_rpm_reg = MSM_GCC_BASE + 0x008,
	.msm_apps_ipc_rpm_val = 4,
};
#endif

static ssize_t shooter_virtual_keys_show(struct kobject *kobj,
			struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf,
		__stringify(EV_KEY) ":" __stringify(KEY_HOME)   ":70:1020:90:90"
		":" __stringify(EV_KEY) ":" __stringify(KEY_MENU)   ":190:1020:100:90"
		":" __stringify(EV_KEY) ":" __stringify(KEY_BACK)   ":345:1020:100:90"
		":" __stringify(EV_KEY) ":" __stringify(KEY_SEARCH) ":468:1020:90:90"
		"\n");
}

static struct kobj_attribute shooter_virtual_keys_attr = {
	.attr = {
		.name = "virtualkeys.atmel-touchscreen",
		.mode = S_IRUGO,
	},
	.show = &shooter_virtual_keys_show,
};

static struct attribute *shooter_properties_attrs[] = {
	&shooter_virtual_keys_attr.attr,
	NULL
};

static struct attribute_group shooter_properties_attr_group = {
	.attrs = shooter_properties_attrs,
};

static void __init msm8x60_init_buses(void)
{
#ifdef CONFIG_I2C_QUP
	void *gsbi_mem = ioremap_nocache(0x19C00000, 4);
	if (!gsbi_mem){
		pr_err("%s(): Error getting memory\n", __func__);
		BUG_ON(1);
	}
	/* Setting protocol code to 0x60 for dual UART/I2C in GSBI12 */
	writel_relaxed(0x6 << 4, gsbi_mem);
	/* Ensure protocol code is written before proceeding further */
	mb();
	iounmap(gsbi_mem);

	msm_gsbi4_qup_i2c_device.dev.platform_data = &msm_gsbi4_qup_i2c_pdata;
	msm_gsbi5_qup_i2c_device.dev.platform_data = &msm_gsbi5_qup_i2c_pdata;
	msm_gsbi7_qup_i2c_device.dev.platform_data = &msm_gsbi7_qup_i2c_pdata;
	msm_gsbi10_qup_i2c_device.dev.platform_data = &msm_gsbi10_qup_i2c_pdata;
#endif
#if defined(CONFIG_SPI_QUP) || defined(CONFIG_SPI_QUP_MODULE)
	msm_gsbi1_qup_spi_device.dev.platform_data = &msm_gsbi1_qup_spi_pdata;
#ifdef CONFIG_MACH_SHOOTER
	msm_gsbi2_qup_spi_device.dev.platform_data = &msm_gsbi2_qup_spi_pdata;
#else
	msm_gsbi3_qup_spi_device.dev.platform_data = &msm_gsbi3_qup_spi_pdata;
#endif
#endif
#ifdef CONFIG_MSM_SSBI
	msm_device_ssbi_pmic1.dev.platform_data =
				&msm8x60_ssbi_pm8058_pdata;
	msm_device_ssbi_pmic2.dev.platform_data =
				&msm8x60_ssbi_pm8901_pdata;
#endif
#ifdef CONFIG_I2C_SSBI
	msm_device_ssbi3.dev.platform_data = &msm_ssbi3_pdata;
#endif
#if defined(CONFIG_USB_EHCI_HCD)
	msm_device_otg.dev.platform_data = &msm_otg_pdata;
#endif
#ifdef CONFIG_BT
	bt_export_bd_address();
#endif
#ifdef CONFIG_SERIAL_MSM_HS
	msm_uart_dm1_pdata.wakeup_irq = gpio_to_irq(SHOOTER_GPIO_BT_HOST_WAKE);
	msm_device_uart_dm1.dev.platform_data = &msm_uart_dm1_pdata;
#endif
#ifdef CONFIG_MSM_BUS_SCALING
	/* RPM calls are only enabled on V2 */
	if (SOCINFO_VERSION_MAJOR(socinfo_get_version()) == 2) {
		msm_bus_apps_fabric_pdata.rpm_enabled = 1;
		msm_bus_sys_fabric_pdata.rpm_enabled = 1;
		msm_bus_mm_fabric_pdata.rpm_enabled = 1;
		msm_bus_sys_fpb_pdata.rpm_enabled = 1;
		msm_bus_cpss_fpb_pdata.rpm_enabled = 1;
	}

	msm_bus_apps_fabric.dev.platform_data = &msm_bus_apps_fabric_pdata;
	msm_bus_sys_fabric.dev.platform_data = &msm_bus_sys_fabric_pdata;
	msm_bus_mm_fabric.dev.platform_data = &msm_bus_mm_fabric_pdata;
	msm_bus_sys_fpb.dev.platform_data = &msm_bus_sys_fpb_pdata;
	msm_bus_cpss_fpb.dev.platform_data = &msm_bus_cpss_fpb_pdata;
#endif
}

static void __init msm8x60_init(void)
{
	int rc = 0;
	struct kobject *properties_kobj;

	pmic_reset_irq = PM8058_IRQ_BASE + PM8058_RESOUT_IRQ;

	/*
	 * Initialize RPM first as other drivers and devices may need
	 * it for their initialization.
	 */
	BUG_ON(msm_rpm_init(&msm_rpm_data));
	BUG_ON(msm_rpmrs_levels_init(msm_rpmrs_levels,
				ARRAY_SIZE(msm_rpmrs_levels)));
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

	/* Buses need to be initialized before early-device registration
	 * to get the platform data for fabrics.
	 */
	msm8x60_init_buses();
	platform_add_devices(early_devices, ARRAY_SIZE(early_devices));

	acpuclk_init(&acpuclk_8x60_soc_data);
	msm8x60_init_gpiomux(msm8x60_htc_gpiomux_cfgs);
	msm8x60_init_mmc();

#if defined(CONFIG_PMIC8058_OTHC) || defined(CONFIG_PMIC8058_OTHC_MODULE)
	msm8x60_init_pm8058_othc();
#endif

	printk(KERN_INFO "[HS_BOARD] (%s) system_rev = %d, engineerid = %d\n",
	       __func__, system_rev, engineerid);
	if ((system_rev == 2 && engineerid >= 1) || system_rev > 2) {
		htc_headset_pmic_data.key_gpio =
			PM8058_GPIO_PM_TO_SYS(SHOOTER_AUD_REMO_PRES);
		htc_headset_pmic_data.key_enable_gpio =
			PM8058_GPIO_PM_TO_SYS(SHOOTER_AUD_REMO_EN);
		htc_headset_8x60.dev.platform_data =
			&htc_headset_8x60_data_xb;
		htc_headset_mgr_data.headset_config_num =
			ARRAY_SIZE(htc_headset_mgr_config);
		htc_headset_mgr_data.headset_config = htc_headset_mgr_config;
		printk(KERN_INFO "[HS_BOARD] (%s) Set MEMS config\n", __func__);
	}

#ifdef CONFIG_MSM_DSPS
	msm8x60_init_dsps();
#endif

	pm8058_platform_data.leds_pdata = &pm8058_flash_leds_data;
	pm8058_platform_data.vibrator_pdata = &pm8058_vib_pdata;

	platform_add_devices(msm_footswitch_devices,
					     msm_num_footswitch_devices);

	platform_add_devices(devices, ARRAY_SIZE(devices));

	platform_add_devices(asoc_devices, ARRAY_SIZE(asoc_devices));

	register_i2c_devices();
	shooter_init_panel();
	msm_pm_set_platform_data(msm_pm_data, ARRAY_SIZE(msm_pm_data));
	msm_pm_set_rpm_wakeup_irq(RPM_SCSS_CPU0_WAKE_UP_IRQ);
	msm_cpuidle_set_states(msm_cstates, ARRAY_SIZE(msm_cstates),
				msm_pm_data);
	BUG_ON(msm_pm_boot_init(&msm_pm_boot_pdata));

	pm8058_gpios_init();

#ifdef CONFIG_SENSORS_MSM_ADC
	msm_adc_pdata.target_hw = MSM_8x60;
#endif

	shooter_init_keypad();
	shooter_wifi_init();
	headset_device_register();

	properties_kobj = kobject_create_and_add("board_properties", NULL);
	if (properties_kobj)
		rc = sysfs_create_group(properties_kobj,
                                &shooter_properties_attr_group);
	if (!properties_kobj || rc)
		pr_err("failed to create board_properties\n");

#ifdef CONFIG_MSM8X60_AUDIO
        spi_register_board_info(msm_spi_board_info, ARRAY_SIZE(msm_spi_board_info));
        gpio_tlmm_config(msm_spi_gpio[0], GPIO_CFG_DISABLE);
        gpio_tlmm_config(msm_spi_gpio[1], GPIO_CFG_DISABLE);
        gpio_tlmm_config(msm_spi_gpio[2], GPIO_CFG_DISABLE);
        gpio_tlmm_config(msm_spi_gpio[3], GPIO_CFG_DISABLE);
        msm_auxpcm_init();
        msm_snddev_init();
        shooter_audio_init();
#endif
}

static void __init msm8x60_init_early(void)
{
	msm8x60_allocate_fb_region();
}

static void __init shooter_fixup(struct machine_desc *desc, struct tag *tags,
				char **cmdline, struct meminfo *mi)
{
	mem_size_mb = parse_tag_memsize((const struct tag *)tags);
	printk(KERN_DEBUG "%s: mem_size_mb=%u\n", __func__, mem_size_mb);
	engineerid = parse_tag_engineerid(tags);
	mi->nr_banks = 1;
	mi->bank[0].start = CONFIG_PHYS_OFFSET;
	mi->bank[0].size = 0x23800000;
	if (mem_size_mb == 1024)
		mi->bank[0].size += 0x10000000;
}

MACHINE_START(SHOOTER, "HTC Evo 3D CDMA")
	.fixup = shooter_fixup,
	.map_io = msm8x60_map_io,
	.reserve = msm8x60_reserve,
	.init_irq = msm8x60_init_irq,
	.handle_irq = gic_handle_irq,
	.init_machine = msm8x60_init,
	.timer = &msm_timer,
	.init_early = msm8x60_init_early,
MACHINE_END

MACHINE_START(SHOOTER_U, "HTC Evo 3D GSM")
	.fixup = shooter_fixup,
	.map_io = msm8x60_map_io,
	.reserve = msm8x60_reserve,
	.init_irq = msm8x60_init_irq,
	.handle_irq = gic_handle_irq,
	.init_machine = msm8x60_init,
	.timer = &msm_timer,
	.init_early = msm8x60_init_early,
MACHINE_END
