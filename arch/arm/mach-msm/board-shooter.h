/* linux/arch/arm/mach-msm/board-shooter.h
 *
 * Copyright (C) 2010-2011 HTC Corporation.
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

#ifndef __ARCH_ARM_MACH_MSM_BOARD_SHOOTER_H
#define __ARCH_ARM_MACH_MSM_BOARD_SHOOTER_H

#include <mach/board.h>
#include <mach/msm_memtypes.h>

#ifdef CONFIG_MACH_SHOOTER
#define SHOOTER_PROJECT_NAME	"shooter"
#else
#define SHOOTER_PROJECT_NAME	"shooter_u"
#endif


/* deal with memory allocation */

#define MSM_SHARED_RAM_PHYS		0x40000000
#define MSM_RAM_CONSOLE_BASE		0x40300000
#define MSM_RAM_CONSOLE_SIZE		SZ_1M

#ifdef CONFIG_FB_MSM_LCDC_DSUB
/* VGA = 1440 x 900 x 4(bpp) x 2(pages)
   prim = 1024 x 600 x 4(bpp) x 2(pages)
   This is the difference. */
#define MSM_FB_DSUB_PMEM_ADDER (0xA32000-0x4B0000)
#else
#define MSM_FB_DSUB_PMEM_ADDER (0)
#endif

#ifdef CONFIG_FB_MSM_TRIPLE_BUFFER
#define MSM_FB_PRIM_BUF_SIZE (roundup((960 * 540 * 4), 4096) * 3) /* 4 bpp x 3 pages */
#else
#define MSM_FB_PRIM_BUF_SIZE (roundup((960 * 540 * 4), 4096) * 2) /* 4 bpp x 2 pages */
#endif

#ifdef CONFIG_FB_MSM_HDMI_MSM_PANEL
#define MSM_FB_EXT_BUF_SIZE (roundup((1920 * 1080 * 2), 4096) * 1) /* 2 bpp x 1 page */
#elif defined(CONFIG_FB_MSM_TVOUT)
#define MSM_FB_EXT_BUF_SIZE (roundup((720 * 576 * 2), 4096) * 2) /* 2 bpp x 2 pages */
#else
#define MSM_FB_EXT_BUF_SIZE 0
#endif

#ifdef CONFIG_FB_MSM_HDMI_AS_PRIMARY
/* 4 bpp x 2 page HDMI case */
#define MSM_FB_SIZE roundup((1920 * 1088 * 4 * 2), 4096)
#else
/* Note: must be multiple of 4096 */
#define MSM_FB_SIZE roundup(MSM_FB_PRIM_BUF_SIZE + MSM_FB_EXT_BUF_SIZE + \
				MSM_FB_DSUB_PMEM_ADDER, 4096)
#endif

#ifdef CONFIG_FB_MSM_HDMI_AS_PRIMARY
#define MSM_PMEM_SF_SIZE 0x8000000 /* 128 Mbytes */
#else
#define MSM_PMEM_SF_SIZE 0x4000000 /* 64 Mbytes */
#endif

#ifdef CONFIG_FB_MSM_OVERLAY0_WRITEBACK
#define MSM_FB_OVERLAY0_WRITEBACK_SIZE roundup((1376 * 768 * 3 * 2), 4096)
#else
#define MSM_FB_OVERLAY0_WRITEBACK_SIZE (0)
#endif  /* CONFIG_FB_MSM_OVERLAY0_WRITEBACK */

#ifdef CONFIG_FB_MSM_OVERLAY1_WRITEBACK
#define MSM_FB_OVERLAY1_WRITEBACK_SIZE roundup((1920 * 1088 * 3 * 2), 4096)
#else
#define MSM_FB_OVERLAY1_WRITEBACK_SIZE (0)
#endif  /* CONFIG_FB_MSM_OVERLAY1_WRITEBACK */

#define MSM_PMEM_KERNEL_EBI1_SIZE  0x600000
#define MSM_PMEM_ADSP_SIZE         0x2000000
#define MSM_PMEM_AUDIO_SIZE        0x28B000

#define MSM_SMI_BASE          0x38000000
#define MSM_SMI_SIZE          0x4000000

#define KERNEL_SMI_BASE       (MSM_SMI_BASE)
#define KERNEL_SMI_SIZE       0x600000

#define USER_SMI_BASE         (KERNEL_SMI_BASE + KERNEL_SMI_SIZE)
#define USER_SMI_SIZE         (MSM_SMI_SIZE - KERNEL_SMI_SIZE)
#define MSM_PMEM_SMIPOOL_SIZE USER_SMI_SIZE

#define MSM_ION_SF_SIZE		0x4000000 /* 64MB */
#define MSM_ION_CAMERA_SIZE     MSM_PMEM_ADSP_SIZE
#define MSM_ION_MM_FW_SIZE	0x200000 /* (2MB) */
#define MSM_ION_MM_SIZE		0x3600000 /* (54MB) */
#define MSM_ION_MFC_SIZE	SZ_8K
#ifdef CONFIG_FB_MSM_OVERLAY1_WRITEBACK
#define MSM_ION_WB_SIZE		0xC00000 /* 12MB */
#else
#define MSM_ION_WB_SIZE		0x600000 /* 6MB */
#endif
#define MSM_ION_QSECOM_SIZE	0x600000 /* (6MB) */
#define MSM_ION_AUDIO_SIZE	MSM_PMEM_AUDIO_SIZE

#ifdef CONFIG_MSM_MULTIMEDIA_USE_ION
#define MSM_ION_HEAP_NUM	9
#else
#define MSM_ION_HEAP_NUM	1
#endif
/* end deal with memory allocation */

/* GPIO definition */

/* Direct Keys */
#define SHOOTER_GPIO_SW_LCM_3D       (64)
#define SHOOTER_GPIO_SW_LCM_2D       (68)
#define SHOOTER_GPIO_KEY_VOL_DOWN    (103)
#define SHOOTER_GPIO_KEY_VOL_UP      (104)
#define SHOOTER_GPIO_KEY_CAM_STEP2   (115)
#define SHOOTER_GPIO_KEY_CAM_STEP1   (123)
#define SHOOTER_GPIO_KEY_POWER       (125)

/* Battery */
#define SHOOTER_GPIO_MBAT_IN		   (61)
#define SHOOTER_GPIO_CHG_INT		   (126)

/* Wifi */
#define SHOOTER_GPIO_WIFI_IRQ              (46)
#define SHOOTER_GPIO_WIFI_SHUTDOWN_N       (96)

/* WiMax */
#define SHOOTER_GPIO_WIMAX_UART_SIN        (41)
#define SHOOTER_GPIO_WIMAX_UART_SOUT       (42)
#define SHOOTER_GPIO_V_WIMAX_1V2_RF_EN     (43)
#define SHOOTER_GPIO_WIMAX_EXT_RST         (49)
#define SHOOTER_GPIO_V_WIMAX_DVDD_EN       (94)
#define SHOOTER_GPIO_V_WIMAX_PVDD_EN       (105)
#define SHOOTER_GPIO_WIMAX_SDIO_D0         (143)
#define SHOOTER_GPIO_WIMAX_SDIO_D1         (144)
#define SHOOTER_GPIO_WIMAX_SDIO_D2         (145)
#define SHOOTER_GPIO_WIMAX_SDIO_D3         (146)
#define SHOOTER_GPIO_WIMAX_SDIO_CMD        (151)
#define SHOOTER_GPIO_WIMAX_SDIO_CLK_CPU    (152)
#define SHOOTER_GPIO_CPU_WIMAX_SW          (156)
#define SHOOTER_GPIO_CPU_WIMAX_UART_EN     (157)

/* Sensors */
#define SHOOTER_SENSOR_I2C_SDA		(72)
#define SHOOTER_SENSOR_I2C_SCL		(73)
#define SHOOTER_GYRO_INT            (127)

/* General */
#define SHOOTER_GENERAL_I2C_SDA		(59)
#define SHOOTER_GENERAL_I2C_SCL		(60)

/* Microp */

/* TP */
#define SHOOTER_TP_I2C_SDA           (51)
#define SHOOTER_TP_I2C_SCL           (52)
#define SHOOTER_TP_ATT_N             (57)

/* LCD */
#define GPIO_LCM_ID	50
#define GPIO_LCM_RST_N	66

/* Audio */
#define SHOOTER_AUD_CODEC_RST        (67)
#define SHOOTER_AUD_CDC_LDO_SEL      (116)
/* BT */
#define SHOOTER_GPIO_BT_HOST_WAKE      (45)
#define SHOOTER_GPIO_BT_UART1_TX       (53)
#define SHOOTER_GPIO_BT_UART1_RX       (54)
#define SHOOTER_GPIO_BT_UART1_CTS      (55)
#define SHOOTER_GPIO_BT_UART1_RTS      (56)
#define SHOOTER_GPIO_BT_SHUTDOWN_N     (100)
#define SHOOTER_GPIO_BT_CHIP_WAKE      (130)
#define SHOOTER_GPIO_BT_RESET_N        (142)

/* USB */
#define SHOOTER_GPIO_USB_ID             (63)
#define SHOOTER_GPIO_MHL_RESET          (70)
#define SHOOTER_GPIO_MHL_INT            (71)
#define SHOOTER_GPIO_MHL_USB_EN        (139)
#define SHOOTER_GPIO_MHL_USB_SW         (99)

/* Camera */

/* Flashlight */
#define SHOOTER_FLASH_EN             (29)
#define SHOOTER_TORCH_EN             (30)

/* Accessory */
#define SHOOTER_GPIO_AUD_HP_DET        (31)

/* SPI */
#define SHOOTER_SPI_DO                 (33)
#define SHOOTER_SPI_DI                 (34)
#define SHOOTER_SPI_CS                 (35)
#define SHOOTER_SPI_CLK                (36)

/* LCM */
#define SHOOTER_CTL_3D_1		(131)
#define SHOOTER_CTL_3D_2		(132)
#define SHOOTER_CTL_3D_3		(133)
#define SHOOTER_CTL_3D_4		(134)
#define SHOOTER_LCM_3D_PDz		(135)

/* CAMERA SPI */
#ifdef CONFIG_MACH_SHOOTER
#define SHOOTER_SP3D_SPI_DO                 (37)
#define SHOOTER_SP3D_SPI_DI                 (38)
#define SHOOTER_SP3D_SPI_CS                 (39)
#define SHOOTER_SP3D_SPI_CLK                (40)
#else
#define SHOOTER_SP3D_SPI_DO                 (41)
#define SHOOTER_SP3D_SPI_DI                 (42)
#define SHOOTER_SP3D_SPI_CS                 (43)
#define SHOOTER_SP3D_SPI_CLK                (44)
#endif

/* CAMERA GPIO */
#define SHOOTER_CAM_I2C_SDA           (47)
#define SHOOTER_CAM_I2C_SCL           (48)

#define SHOOTER_SP3D_GATE              (107)
#define SHOOTER_SP3D_CORE_GATE         (58)
#define SHOOTER_SP3D_SYS_RST           (102)
#define SHOOTER_SP3D_PDX               (137)

#define SHOOTER_S5K4E1_PD				(137)
#define SHOOTER_S5K4E1_INTB				(102)
#define SHOOTER_S5K4E1_VCM_PD			(58)

#define SHOOTER_SP3D_MCLK		(32)
#define SHOOTER_WEBCAM_STB		(140)
#define SHOOTER_WEBCAM_RST		(138)
#define SHOOTER_CAM_SEL			(141)


/* PMIC */

/* PMIC GPIO definition */
#define PMGPIO(x) (x-1)
#define SHOOTER_VOL_UP             (104)
#define SHOOTER_VOL_DN             (103)
#define SHOOTER_AUD_HP_EN          PMGPIO(18)
#define SHOOTER_AUD_SPK_ENO        PMGPIO(19)
#define SHOOTER_3DLCM_PD           PMGPIO(20)
#define SHOOTER_PS_VOUT            PMGPIO(22)
#define SHOOTER_GREEN_LED          PMGPIO(24)
#define SHOOTER_AMBER_LED          PMGPIO(25)
#define SHOOTER_3DCLK              PMGPIO(26)
#define SHOOTER_AUD_MIC_SEL        PMGPIO(14)
#define SHOOTER_WIFI_BT_SLEEP_CLK  PMGPIO(38)
#define SHOOTER_WIMAX_HOST_WAKEUP  PMGPIO(17)
#define SHOOTER_WIMAX_DEBUG12      PMGPIO(16)
#define SHOOTER_WIMAX_DEBUG14_XA   PMGPIO(28)
#define SHOOTER_WIMAX_DEBUG15_XA   PMGPIO(30)
#define SHOOTER_WIMAX_DEBUG14      PMGPIO(35)
#define SHOOTER_WIMAX_DEBUG15      PMGPIO(36)
#define SHOOTER_TP_RST             PMGPIO(23)
#ifdef CONFIG_MACH_SHOOTER
#define SHOOTER_TORCH_SET1         PMGPIO(32)
#else
#define SHOOTER_TORCH_SET1         PMGPIO(40)
#endif
#define SHOOTER_TORCH_SET2         PMGPIO(31)
#define SHOOTER_CHG_STAT	   PMGPIO(33)
#define SHOOTER_AUD_REMO_EN        PMGPIO(15)
#define SHOOTER_AUD_REMO_PRES      PMGPIO(37)

#define SW_CAM				0x0e

void __init shooter_init_mmc(void);
void __init shooter_audio_init(void);
void __init shooter_init_keypad(void);
int __init shooter_wifi_init(void);
void __init shooter_init_panel(void);
void msm8x60_allocate_fb_region(void);
void msm8x60_mdp_writeback(struct memtype_reserve *reserve_table);

#endif /* __ARCH_ARM_MACH_MSM_BOARD_SHOOTER_H */
