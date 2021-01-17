#include <stdio.h>
#include "layout2.h"
#include "tron.h"

void layoutTronConfirmTx(const uint8_t *to, uint32_t to_len, const uint8_t *value, uint32_t value_len, TronTokenType *token)
{
	bignum256 val;
	uint8_t pad_val[32];
	memset(pad_val, 0, sizeof(pad_val));
	memcpy(pad_val + (32 - value_len), value, value_len);
	bn_read_be(pad_val, &val);

	char amount[32];
	if (token == NULL) {
		if (bn_is_zero(&val)) {
			strcpy(amount, _("message"));
		} else {
			tron_format_amount(&val, amount, sizeof(amount));
		}
	} else {
		tron_format_token_amount(&val, token, amount, sizeof(amount));
	}

	char _to1[] = "to 0x__________";
	char _to2[] = "_______________";
	char _to3[] = "_______________?";

	if (to_len) {
		char to_str[41];
		ethereum_address_checksum(to, to_str, false, 0);
		memcpy(_to1 + 5, to_str, 10);
		memcpy(_to2, to_str + 10, 15);
		memcpy(_to3, to_str + 25, 15);
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
