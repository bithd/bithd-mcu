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

#ifndef __BUTTONS_H__
#define __BUTTONS_H__

#include "bithd_device.h"

#include <libopencm3/stm32/gpio.h>
#include <stdbool.h>

struct buttonState {
	volatile bool YesUp;
	volatile int YesDown;
	volatile bool NoUp;
	volatile int NoDown;
};

extern struct buttonState button;

void buttonUpdate(void);

#ifndef BTN_PORT
#define BTN_PORT	GPIOC
#endif

#ifndef BTN_PIN_YES
#if defined(BITHD_RAZOR)
#define BTN_PIN_YES	GPIO0
#elif defined(BITHD_BITHD)
#define BTN_PIN_YES	GPIO10
#else
#error "No valid DEVICE_MODEL defined"
#endif
#endif

#ifndef BTN_PIN_NO
#if defined(BITHD_RAZOR)
#define BTN_PIN_NO	GPIO14
#elif defined(BITHD_BITHD)
#define BTN_PIN_NO	GPIO12
#else
#error "No valid DEVICE_MODEL defined"
#endif
#endif

#if defined(BITHD_RAZOR)
#define BitBTN_PORT GPIOC
#define BitBTN_PIN_YES GPIO0
#define BitBTN_PIN_NO	GPIO14

#define FUNC1_PORT GPIOC
#define FUNC2_PORT GPIOB
#define FUNC3_PORT GPIOA

#define FUNC1_PIN	GPIO9
#define FUNC2_PIN	GPIO15
#define FUNC3_PIN	GPIO15
#elif defined(BITHD_BITHD)
#define BitBTN_PORT GPIOA
#define BitBTN_PIN_YES GPIO2
#else
#error "No valid DEVICE_MODEL defined"
#endif

#endif
