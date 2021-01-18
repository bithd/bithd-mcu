#include <stdio.h>
#include "gettext.h"
#include "layout2.h"
#include "tron_ui.h"
#include "tron.h"

void layoutTronConfirmTx(const char *to_str, const uint64_t value, const uint8_t *value_bytes, uint32_t value_len, ConstTronTokenPtr token) {
	bignum256 val;
	uint8_t pad_val[32];
	memset(pad_val, 0, sizeof(pad_val));
	memcpy(pad_val + (32 - value_len), value_bytes, value_len);
	bn_read_be(pad_val, &val);

	char amount[32];
	if (token == NULL) {
		if (value == 0) {
			strcpy(amount, _("message"));
		} else {
			tron_format_amount(value, amount, sizeof(amount));
		}
	} else {
		tron_format_token_amount(&val, token, amount, sizeof(amount));
	}

	// ex: TNUC9Qb1rRpS5CbWLmNMxXBjyFoydXjWFR
	char _to1[] = "to 0x________";
	char _to2[] = "_____________";
	char _to3[] = "_____________?";

	int to_len = strlen(to_str);
	if (to_len) {
		memcpy(_to1 + 5, to_str, 8);
		memcpy(_to2, to_str + 8, 13);
		memcpy(_to3, to_str + 21, 13);
	} else {
		strlcpy(_to1, _("to new contract?"), sizeof(_to1));
		strlcpy(_to2, "", sizeof(_to2));
		strlcpy(_to3, "", sizeof(_to3));
	}

	layoutDialogSwipe(&bmp_icon_question,
		_("Cancel"),
		_("Confirm"),
		NULL,
		_("Send"),
		amount,
		_to1,
		_to2,
		_to3,
		NULL
	);
}

void layoutTronFee(const uint64_t value, const uint8_t *value_bytes, uint32_t value_len, ConstTronTokenPtr token, const uint64_t fee) {
	bignum256 val;
	uint8_t pad_val[32];
	memset(pad_val, 0, sizeof(pad_val));
	memcpy(pad_val + (32 - value_len), value_bytes, value_len);
	bn_read_be(pad_val, &val);

	char gas_value[32];
	tron_format_amount(fee, gas_value, sizeof(gas_value));

	char tx_value[32];
	if (token == NULL) {
		if (value == 0) {
			strcpy(tx_value, _("message"));
		} else {
			tron_format_amount(value, tx_value, sizeof(tx_value));
		}
	} else {
		tron_format_token_amount(&val, token, tx_value, sizeof(tx_value));
	}

	layoutDialogSwipe(&bmp_icon_question,
		_("Cancel"),
		_("Confirm"),
		NULL,
		_("Really send"),
		tx_value,
		_("paying up to"),
		gas_value,
		_("for fee?"),
		NULL
	);
}
