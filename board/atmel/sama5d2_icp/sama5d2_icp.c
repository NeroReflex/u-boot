// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2018 Microchip Technology, Inc.
 *		      Eugen Hristev <eugen.hristev@microchip.com>
 */

#include <config.h>
#include <debug_uart.h>
#include <init.h>
#include <asm/global_data.h>
#include <asm/io.h>
#include <asm/arch/at91_common.h>
#include <asm/arch/atmel_pio4.h>
#include <asm/arch/atmel_mpddrc.h>
#include <asm/arch/atmel_sdhci.h>
#include <asm/arch/clk.h>
#include <asm/arch/gpio.h>
#include <asm/arch/sama5d2.h>

DECLARE_GLOBAL_DATA_PTR;

static void rgb_leds_init(void)
{
	atmel_pio4_set_pio_output(AT91_PIO_PORTB, 0, 0);	/* LED RED */
	atmel_pio4_set_pio_output(AT91_PIO_PORTB, 1, 0);	/* LED GREEN */
	atmel_pio4_set_pio_output(AT91_PIO_PORTA, 31, 1);	/* LED BLUE */
}

int board_late_init(void)
{
	return 0;
}

#ifdef CONFIG_DEBUG_UART_BOARD_INIT
static void board_uart0_hw_init(void)
{
	atmel_pio4_set_c_periph(AT91_PIO_PORTB, 26, ATMEL_PIO_PUEN_MASK); /* URXD0 */
	atmel_pio4_set_c_periph(AT91_PIO_PORTB, 27, 0);	/* UTXD0 */

	at91_periph_clk_enable(ATMEL_ID_UART0);
}

void board_debug_uart_init(void)
{
	board_uart0_hw_init();
}
#endif

int board_early_init_f(void)
{
	return 0;
}

int board_init(void)
{
	/* address of boot parameters */
	gd->bd->bi_boot_params = CFG_SYS_SDRAM_BASE + 0x100;

	rgb_leds_init();

	return 0;
}

int dram_init(void)
{
	gd->ram_size = get_ram_size((void *)CFG_SYS_SDRAM_BASE,
				    CFG_SYS_SDRAM_SIZE);
	return 0;
}

#define MAC24AA_MAC_OFFSET	0xfa

int misc_init_r(void)
{
#ifdef CONFIG_I2C_EEPROM
	at91_set_ethaddr(MAC24AA_MAC_OFFSET);
#endif
	return 0;
}

/* SPL */
#ifdef CONFIG_XPL_BUILD

/* must set PB25 low to enable the CAN transceivers */
static void board_can_stdby_dis(void)
{
	atmel_pio4_set_pio_output(AT91_PIO_PORTB, 25, 0);
}

static void board_leds_init(void)
{
	atmel_pio4_set_pio_output(AT91_PIO_PORTB, 0, 0); /* RED */
	atmel_pio4_set_pio_output(AT91_PIO_PORTB, 1, 1); /* GREEN */
	atmel_pio4_set_pio_output(AT91_PIO_PORTA, 31, 0); /* BLUE */
}

/* deassert reset lines for external periph in case of warm reboot */
static void board_reset_additional_periph(void)
{
	atmel_pio4_set_pio_output(AT91_PIO_PORTB, 16, 0); /* LAN9252_RST */
	atmel_pio4_set_pio_output(AT91_PIO_PORTC, 2, 0); /* HSIC_RST */
	atmel_pio4_set_pio_output(AT91_PIO_PORTC, 17, 0); /* USB2534_RST */
	atmel_pio4_set_pio_output(AT91_PIO_PORTD, 4, 0); /* KSZ8563_RST */
}

static void board_start_additional_periph(void)
{
	atmel_pio4_set_pio_output(AT91_PIO_PORTB, 16, 1); /* LAN9252_RST */
	atmel_pio4_set_pio_output(AT91_PIO_PORTC, 2, 1); /* HSIC_RST */
	atmel_pio4_set_pio_output(AT91_PIO_PORTC, 17, 1); /* USB2534_RST */
	atmel_pio4_set_pio_output(AT91_PIO_PORTD, 4, 1); /* KSZ8563_RST */
}

#ifdef CONFIG_SD_BOOT
void spl_mmc_init(void)
{
	atmel_pio4_set_a_periph(AT91_PIO_PORTA, 1, 0);	/* CMD */
	atmel_pio4_set_a_periph(AT91_PIO_PORTA, 2, 0);	/* DAT0 */
	atmel_pio4_set_a_periph(AT91_PIO_PORTA, 3, 0);	/* DAT1 */
	atmel_pio4_set_a_periph(AT91_PIO_PORTA, 4, 0);	/* DAT2 */
	atmel_pio4_set_a_periph(AT91_PIO_PORTA, 5, 0);	/* DAT3 */
	atmel_pio4_set_a_periph(AT91_PIO_PORTA, 0, 0);	/* CK */
	atmel_pio4_set_a_periph(AT91_PIO_PORTA, 13, 0);	/* CD */

	at91_periph_clk_enable(ATMEL_ID_SDMMC0);
}
#endif

void spl_board_init(void)
{
#ifdef CONFIG_SD_BOOT
	spl_mmc_init();
#endif
	board_reset_additional_periph();
	board_can_stdby_dis();
	board_leds_init();
}

void spl_display_print(void)
{
}

void spl_board_prepare_for_boot(void)
{
	board_start_additional_periph();
}

static void ddrc_conf(struct atmel_mpddrc_config *ddrc)
{
	ddrc->md = (ATMEL_MPDDRC_MD_DBW_32_BITS | ATMEL_MPDDRC_MD_DDR3_SDRAM);

	ddrc->cr = (ATMEL_MPDDRC_CR_NC_COL_10 |
		    ATMEL_MPDDRC_CR_NR_ROW_14 |
		    ATMEL_MPDDRC_CR_CAS_DDR_CAS5 |
		    ATMEL_MPDDRC_CR_DIC_DS |
		    ATMEL_MPDDRC_CR_NB_8BANKS |
		    ATMEL_MPDDRC_CR_DECOD_INTERLEAVED |
		    ATMEL_MPDDRC_CR_UNAL_SUPPORTED);

	ddrc->rtr = 0x298;

	ddrc->tpr0 = ((6 << ATMEL_MPDDRC_TPR0_TRAS_OFFSET) |
		      (3 << ATMEL_MPDDRC_TPR0_TRCD_OFFSET) |
		      (3 << ATMEL_MPDDRC_TPR0_TWR_OFFSET) |
		      (9 << ATMEL_MPDDRC_TPR0_TRC_OFFSET) |
		      (3 << ATMEL_MPDDRC_TPR0_TRP_OFFSET) |
		      (4 << ATMEL_MPDDRC_TPR0_TRRD_OFFSET) |
		      (4 << ATMEL_MPDDRC_TPR0_TWTR_OFFSET) |
		      (4 << ATMEL_MPDDRC_TPR0_TMRD_OFFSET));

	ddrc->tpr1 = ((27 << ATMEL_MPDDRC_TPR1_TRFC_OFFSET) |
		      (29 << ATMEL_MPDDRC_TPR1_TXSNR_OFFSET) |
		      (0 << ATMEL_MPDDRC_TPR1_TXSRD_OFFSET) |
		      (10 << ATMEL_MPDDRC_TPR1_TXP_OFFSET));

	ddrc->tpr2 = ((0 << ATMEL_MPDDRC_TPR2_TXARD_OFFSET) |
		      (0 << ATMEL_MPDDRC_TPR2_TXARDS_OFFSET) |
		      (0 << ATMEL_MPDDRC_TPR2_TRPA_OFFSET) |
		      (4 << ATMEL_MPDDRC_TPR2_TRTP_OFFSET) |
		      (7 << ATMEL_MPDDRC_TPR2_TFAW_OFFSET));
}

void at91_mem_init(void)
{
	struct at91_pmc *pmc = (struct at91_pmc *)ATMEL_BASE_PMC;
	struct atmel_mpddr *mpddrc = (struct atmel_mpddr *)ATMEL_BASE_MPDDRC;
	struct atmel_mpddrc_config ddrc_config;
	u32 reg;

	ddrc_conf(&ddrc_config);

	at91_periph_clk_enable(ATMEL_ID_MPDDRC);
	writel(AT91_PMC_DDR, &pmc->scer);

	reg = readl(&mpddrc->io_calibr);
	reg &= ~ATMEL_MPDDRC_IO_CALIBR_RDIV;
	reg |= ATMEL_MPDDRC_IO_CALIBR_DDR3_RZQ_55;
	reg &= ~ATMEL_MPDDRC_IO_CALIBR_TZQIO;
	reg |= ATMEL_MPDDRC_IO_CALIBR_TZQIO_(100);
	writel(reg, &mpddrc->io_calibr);

	writel(ATMEL_MPDDRC_RD_DATA_PATH_SHIFT_ONE_CYCLE,
	       &mpddrc->rd_data_path);

	ddr3_init(ATMEL_BASE_MPDDRC, ATMEL_BASE_DDRCS, &ddrc_config);

	writel(0x5355, &mpddrc->cal_mr4);
	writel(64, &mpddrc->tim_cal);
}

void at91_pmc_init(void)
{
	u32 tmp;

	/*
	 * while coming from the ROM code, we run on PLLA @ 492 MHz / 164 MHz
	 * so we need to slow down and configure MCKR accordingly.
	 * This is why we have a special flavor of the switching function.
	 */
	tmp = AT91_PMC_MCKR_PLLADIV_2 |
	      AT91_PMC_MCKR_MDIV_3 |
	      AT91_PMC_MCKR_CSS_MAIN;
	at91_mck_init_down(tmp);

	tmp = AT91_PMC_PLLAR_29 |
	      AT91_PMC_PLLXR_PLLCOUNT(0x3f) |
	      AT91_PMC_PLLXR_MUL(82) |
	      AT91_PMC_PLLXR_DIV(1);
	at91_plla_init(tmp);

	tmp = AT91_PMC_MCKR_H32MXDIV |
	      AT91_PMC_MCKR_PLLADIV_2 |
	      AT91_PMC_MCKR_MDIV_3 |
	      AT91_PMC_MCKR_CSS_PLLA;
	at91_mck_init(tmp);
}
#endif
