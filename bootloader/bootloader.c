/*
 * This file is part of the TREZOR project, https://trezor.io/
 *
 * Copyright (C) 2014 Pavol Rusnak <stick@satoshilabs.com>
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "bithd_device.h"

#include <string.h>

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/cm3/scb.h>
#include <libopencm3/stm32/flash.h>
#include "bootloader.h"
#include "buttons.h"
#include "setup.h"
#include "usb.h"
#include "oled.h"
#include "util.h"
#include "signatures.h"
#include "layout.h"
#include "serialno.h"
#include "rng.h"
#include "../timerbitpie.h"
#include "./uart.h"

#define PROTECTED 0

void layoutFirmwareHash(const uint8_t *hash)
{
	char str[4][17];
	for (int i = 0; i < 4; i++) {
		data2hex(hash + i * 8, 8, str[i]);
	}
	layoutDialog(&bmp_icon_question, "Abort", "Continue", "Compare fingerprints", str[0], str[1], str[2], str[3], NULL, NULL);
}

void show_halt(void)
{
	layoutDialog(&bmp_icon_error, NULL, NULL, NULL, "Unofficial firmware", "aborted.", NULL, "Close your BITHD", "contact our support.", NULL);
	system_halt();
}

void show_unofficial_warning(const uint8_t *hash)
{
	layoutDialog(&bmp_icon_warning, "Abort", "I'll take the risk", NULL, "WARNING!", NULL, "Unofficial firmware", "detected.", NULL, NULL);

	do {
		delay(100000);
		buttonUpdate();
	} while (!button.YesUp && !button.NoUp);

	if (button.NoUp) {
		show_halt(); // no button was pressed -> halt
	}

	layoutFirmwareHash(hash);

	do {
		delay(100000);
		buttonUpdate();
	} while (!button.YesUp && !button.NoUp);

	if (button.NoUp) {
		show_halt(); // no button was pressed -> halt
	}

	// everything is OK, user pressed 2x Continue -> continue program
}

void __attribute__((noreturn)) load_app(void)
{
	// zero out SRAM
	memset_reg(_ram_start, _ram_end, 0);

	load_vector_table((const vector_table_t *) FLASH_APP_START);
}

bool firmware_present(void)
{
#ifndef APPVER
	if (memcmp((const void *)FLASH_META_MAGIC, "TRZR", 4)) { // magic does not match
		return false;
	}
	if (*((const uint32_t *)FLASH_META_CODELEN) < 4096) { // firmware reports smaller size than 4kB
		return false;
	}
	if (*((const uint32_t *)FLASH_META_CODELEN) > FLASH_TOTAL_SIZE - (FLASH_APP_START - FLASH_ORIGIN)) { // firmware reports bigger size than flash size
		return false;
	}
#endif
	return true;
}

void bootloader_loop(void)
{
	oledClear();
	oledDrawBitmap(0, 8, &bmp_logo64);
	if (firmware_present()) {
		oledDrawString(52, 0, "BITHD");
		static char serial[25];
		fill_serialno_fixed(serial);
		oledDrawString(52, 20, "Serial No.");
		oledDrawString(52, 40, serial + 12); // second part of serial
		serial[12] = 0;
		oledDrawString(52, 30, serial);      // first part of serial
		oledDrawStringRight(OLED_WIDTH - 1, OLED_HEIGHT - 8, "Loader " VERSTR(VERSION_MAJOR) "." VERSTR(VERSION_MINOR) "." VERSTR(VERSION_PATCH));
	} else {
		oledDrawString(52, 10, "Welcome!");
		oledDrawString(52, 30, "Please visit");
		oledDrawString(52, 50, "Bithd.com/start");
	}
	oledRefresh();

	usbLoop(firmware_present());
}


/*********************
 * Function:clear Sram data,For fix  physical memory access issue
*****/
#define RamStartAdress 0x20000000
#define SizeOfRam          0x1ffff

int main(void)
{
	unsigned char bufacksigendsucess[]={0x90,0x00};
	unsigned char bufacksigenderro[]={0x67,0x00};

#ifndef APPVER
	setup();
#endif
	__stack_chk_guard = random32(); // this supports compiler provided unpredictable stack protection checks
#ifndef APPVER
#if PROTECTED
	memory_protect();
#endif
	oledInit();
#endif
///////////////////////////////////////////

IF_RAZOR(
	gpio_mode_setup(GPIOA, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, GPIO15);
)
IF_BITHD(
	gpio_mode_setup(GPIOA, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, BitBTN_PIN_YES);
	gpio_mode_setup(GPIOB, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, GPIO5);
)
    timer_init();


/////////////////////////////////////////////////
IF_RAZOR(
    uint16_t state;
)
IF_BITHD(
    uint16_t state= gpio_port_read(BitBTN_PORT);
)

    delay(100000);
IF_RAZOR(
    state = gpio_port_read(GPIOA);
 	if((state & GPIO15) == GPIO15)
)
IF_BITHD(
    state = gpio_port_read(GPIOB);
 	if ((state & GPIO5) == GPIO5)
)
    {
		usart_setup();    //uart init 115200
		if (!signatures_ok(NULL)) 
		{
		  CmdSendUart(5,bufacksigenderro,2);
		}
		else
		{
		  CmdSendUart(5,bufacksigendsucess,2);
		}	
		for (;;) 
		{
		UartDataSendrecive();
		}	
	}
	else
	{
IF_RAZOR(
		state = gpio_port_read(GPIOA);
)
IF_BITHD(
		state = gpio_port_read(BitBTN_PORT);
)
		oledClear();
		oledDrawBitmap(40, 8,&bmp_logo64);
		oledRefresh();
		
IF_RAZOR(
		if ((state & GPIO2) == GPIO2)
)
IF_BITHD(
		if ((state & BitBTN_PIN_YES) == BitBTN_PIN_YES)
)
		{
#if PROTECTED
			if (firmware_present()) 
			{
				if (!signatures_ok(NULL)) 
				{
					layoutDialog(&bmp_icon_warning, NULL, NULL, NULL, "Firmware installation", "aborted.", NULL, "You need to repeat", "the procedure with", "the correct firmware.");
					flash_unlock();
					for (int i = FLASH_CODE_SECTOR_FIRST; i <= FLASH_CODE_SECTOR_LAST; i++) {
						flash_erase_sector(i, FLASH_CR_PROGRAM_X32);
					}
					flash_lock();
				}
				else
				{
#endif
					load_app();
#if PROTECTED
				}	
			}
#endif
		}
		usart_setup();    //uart init 115200
		bootloader_loop();
	}

	return 0;
}
