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

#include "protect.h"
#include "storage.h"
#include "messages.h"
#include "usb.h"
#include "oled.h"
#include "buttons.h"
#include "pinmatrix.h"
#include "fsm.h"
#include "layout2.h"
#include "util.h"
#include "debug.h"
#include "gettext.h"

#define MAX_WRONG_PINS 4

bool protectAbortedByInitialize = false;
unsigned char protectbuttonflag=0;
extern unsigned char sendsuccessflag;
bool protectButton(ButtonRequestType type, bool confirm_only)
{
	ButtonRequest resp;
	bool result = false;
	bool acked = false;
#if DEBUG_LINK
	bool debug_decided = false;
#endif

	memset(&resp, 0, sizeof(ButtonRequest));
	resp.has_code = true;
	resp.code = type;
	usbTiny(1);
	buttonUpdate(); // Clear button state
	msg_write(MessageType_MessageType_ButtonRequest, &resp);
	loopuart=1;
	protectbuttonflag=1;
	for (;;) {
		usbPoll();

		// check for ButtonAck
		if (msg_tiny_id == MessageType_MessageType_ButtonAck) {
			msg_tiny_id = 0xFFFF;
			acked = true;
		}

		// button acked - check buttons
		if (acked) {
			usbSleep(5);
			buttonUpdate();
			if (button.YesUp) {
				result = true;
				while(sendsuccessflag!=0)
				{
					usbPoll();
				}
				break;
			}
			if (!confirm_only && button.NoUp) {
				result = false;
				while(sendsuccessflag!=0)
				{
				usbPoll();
				}
				break;
			}
		}

		// check for Cancel / Initialize
		if (msg_tiny_id == MessageType_MessageType_Cancel || msg_tiny_id == MessageType_MessageType_Initialize) {
			if (msg_tiny_id == MessageType_MessageType_Initialize) {
				protectAbortedByInitialize = true;
			}
			msg_tiny_id = 0xFFFF;
			result = false;
			break;
		}

#if DEBUG_LINK
		// check DebugLink
		if (msg_tiny_id == MessageType_MessageType_DebugLinkDecision) {
			msg_tiny_id = 0xFFFF;
			DebugLinkDecision *dld = (DebugLinkDecision *)msg_tiny;
			result = dld->yes_no;
			debug_decided = true;
		}

		if (acked && debug_decided) {
			break;
		}

		if (msg_tiny_id == MessageType_MessageType_DebugLinkGetState) {
			msg_tiny_id = 0xFFFF;
			fsm_msgDebugLinkGetState((DebugLinkGetState *)msg_tiny);
		}
#endif
	}

	usbTiny(0);
	loopuart=0;
	protectbuttonflag=0;
	return result;
}

const char *requestPin(PinMatrixRequestType type, const char *text)
{
	PinMatrixRequest resp;
	memset(&resp, 0, sizeof(PinMatrixRequest));
	resp.has_type = true;
	resp.type = type;
	usbTiny(1);
	msg_write(MessageType_MessageType_PinMatrixRequest, &resp);
	pinmatrix_start(text);
	loopuart=1;
	for (;;) {
		usbPoll();
		if (msg_tiny_id == MessageType_MessageType_PinMatrixAck) {
			msg_tiny_id = 0xFFFF;
			PinMatrixAck *pma = (PinMatrixAck *)msg_tiny;
			pinmatrix_done(pma->pin); // convert via pinmatrix
			usbTiny(0);
			return pma->pin;
		}
		if (msg_tiny_id == MessageType_MessageType_Cancel || msg_tiny_id == MessageType_MessageType_Initialize) {
			pinmatrix_done(0);
			if (msg_tiny_id == MessageType_MessageType_Initialize) {
				protectAbortedByInitialize = true;
			}
			msg_tiny_id = 0xFFFF;
			usbTiny(0);
			return 0;
		}
#if DEBUG_LINK
		if (msg_tiny_id == MessageType_MessageType_DebugLinkGetState) {
			msg_tiny_id = 0xFFFF;
			fsm_msgDebugLinkGetState((DebugLinkGetState *)msg_tiny);
		}
#endif
	}
		loopuart=0;
}

static void protectCheckMaxTry(uint32_t wait) {
	if (wait < (1 << MAX_WRONG_PINS))
		return;

	storage_wipe_pinerr();
	layoutDialog(&bmp_icon_error, NULL, NULL, NULL, _("Too many wrong PIN"), _("attempts. Storage has"), _("been wiped."), NULL, _("Please unplug"), _("the device."));
	// for (;;) {} // loop forever
}

bool protectPin(bool use_cached)
{
	if (!storage.has_pin || storage.pin[0] == 0 || (use_cached && session_isPinCached())) {
		return true;
	}
	uint32_t *fails = storage_getPinFailsPtr();
	uint32_t wait = ~*fails;
	protectCheckMaxTry(wait);
	usbTiny(1);
	while (wait > 0) {
		// convert wait to secstr string
		char secstrbuf[20];
		strlcpy(secstrbuf, _("________0 seconds"), sizeof(secstrbuf));
		char *secstr = secstrbuf + 9;
		uint32_t secs = wait;
		while (secs > 0 && secstr >= secstrbuf) {
			secstr--;
			*secstr = (secs % 10) + '0';
			secs /= 10;
		}
		if (wait == 1) {
			secstrbuf[16] = 0;
		}
		switch (storage_getLang()) {
				case CHINESE :
					layoutZhDialogSwipe(&bmp_icon_question, NULL, NULL, NULL, "输入#P##I##N#码错误", NULL, "请稍等#.##.##.#", NULL);
					break;
				default	:
					layoutDialogSwipe(&bmp_icon_question, NULL, NULL, NULL, "Wrong PIN entered", NULL, "Please wait ...", NULL, NULL, NULL);
					break;
			}
		//layoutDialog(&bmp_icon_info, NULL, NULL, NULL, _("Wrong PIN entered"), NULL, _("Please wait"), secstr, _("to continue ..."), NULL);
		// wait one second
		usbSleep(1000);
		if (msg_tiny_id == MessageType_MessageType_Initialize) {
			protectAbortedByInitialize = true;
			msg_tiny_id = 0xFFFF;
			usbTiny(0);
			fsm_sendFailure(FailureType_Failure_PinCancelled, NULL);
			return false;
		}
		wait--;
	}
	usbTiny(0);
	const char *pin;
	switch (storage_getLang()) {
		case CHINESE :
			pin = requestPin(PinMatrixRequestType_PinMatrixRequestType_Current, "请输入当前#P##I##N#码#:#");
			break;
		default	:
			pin = requestPin(PinMatrixRequestType_PinMatrixRequestType_Current, "Please enter current PIN:");
			break;
	}
	if (!pin) {
		fsm_sendFailure(FailureType_Failure_PinCancelled, NULL);
		return false;
	}
	if (!storage_increasePinFails(fails)) {
		fsm_sendFailure(FailureType_Failure_PinInvalid, NULL);
		return false;
	}
	if (storage_containsPin(pin)) {
		session_cachePin();
		storage_resetPinFails(fails);
		return true;
	} else {
		protectCheckMaxTry(~*fails);
		fsm_sendFailure(FailureType_Failure_PinInvalid, NULL);
		return false;
	}
}

bool protectChangePin(void)
{
	const char *pin;
	char pin1[17], pin2[17];

	switch (storage_getLang()) {
		case CHINESE :
			pin = requestPin(PinMatrixRequestType_PinMatrixRequestType_NewFirst, "请输入新的#P##I##N#码#:#");
			break;
		default	:
			pin = requestPin(PinMatrixRequestType_PinMatrixRequestType_NewFirst, "Please enter new PIN:");
			break;
	}
	if (!pin) {
		return false;
	}
	strlcpy(pin1, pin, sizeof(pin1));
	switch (storage_getLang()) {
		case CHINESE :
			pin = requestPin(PinMatrixRequestType_PinMatrixRequestType_NewSecond, "请再输入#P##I##N#码#:#");
			break;
		default	:
			pin = requestPin(PinMatrixRequestType_PinMatrixRequestType_NewSecond, "Please re-enter new PIN:");
			break;
	}
	if (!pin) {
		return false;
	}
	strlcpy(pin2, pin, sizeof(pin2));
	if (strcmp(pin1, pin2) == 0) {
		storage_setPin(pin1);
		return true;
	} else {
		return false;
	}
}

bool protectPassphrase(void)
{
	if (!storage.has_passphrase_protection || !storage.passphrase_protection || session_isPassphraseCached()) {
		return true;
	}

	PassphraseRequest resp;
	memset(&resp, 0, sizeof(PassphraseRequest));
	usbTiny(1);
	msg_write(MessageType_MessageType_PassphraseRequest, &resp);

	switch (storage_getLang()) {
		case CHINESE :
			layoutZhDialogSwipe(&bmp_icon_info, NULL, NULL, NULL, "请在手机上输入", NULL, "您的密码#!#", NULL);
			break;
		default	:
			layoutDialogSwipe(&bmp_icon_info, NULL, NULL, NULL, "Please enter", NULL,"your passphrase",NULL, "on your mobile phone!", NULL);
			break;												
	}

	bool result;
	loopuart=1;
	for (;;) {
		usbPoll();
		if (msg_tiny_id == MessageType_MessageType_PassphraseAck) {
			msg_tiny_id = 0xFFFF;
			PassphraseAck *ppa = (PassphraseAck *)msg_tiny;
			session_cachePassphrase(ppa->passphrase);
			result = true;
			break;
		}
		if (msg_tiny_id == MessageType_MessageType_Cancel || msg_tiny_id == MessageType_MessageType_Initialize) {
			if (msg_tiny_id == MessageType_MessageType_Initialize) {
				protectAbortedByInitialize = true;
			}
			msg_tiny_id = 0xFFFF;
			result = false;
			break;
		}
	}
	loopuart=0;
	usbTiny(0);
	layoutHome();
	return result;
}
