#include <linux/kernel.h>
#include <linux/platform_device.h>

#include <linux/bootmem.h>
#include <linux/proc_fs.h>

#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/hardware/gic.h>


#include "devices-msm8x60.h"

#include <mach/board.h>
#include <mach/board_htc.h>
#include <mach/msm_iomap.h>
#include <mach/socinfo.h>

#include "timer.h"
#include "board-shooter.h"

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

static struct platform_device *devices[] __initdata = {
	&ram_console_device,
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
	platform_add_devices(devices, ARRAY_SIZE(devices));
}

static void __init shooter_fixup(struct machine_desc *desc, struct tag *tags,
				char **cmdline, struct meminfo *mi)
{
	mi->nr_banks = 1;
	mi->bank[0].start = CONFIG_PHYS_OFFSET;
	mi->bank[0].size = 0x33800000;
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
