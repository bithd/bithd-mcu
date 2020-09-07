/*
 * This file is part of the TREZOR project, https://trezor.io/
 *
 * Copyright (C) 2016 Alex Beregszaszi <alex@rtfs.hu>
 * Copyright (C) 2016 Pavol Rusnak <stick@satoshilabs.com>
 * Copyright (C) 2016 Jochen Hoenicke <hoenicke@gmail.com>
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

#include <stdio.h>

#include "ethereum.h"
#include "fsm.h"
#include "layout2.h"
#include "messages.h"
#include "transaction.h"
#include "ecdsa.h"
#include "protect.h"
#include "crypto.h"
#include "secp256k1.h"
#include "sha3.h"
#include "address.h"
#include "util.h"
#include "gettext.h"
#include "ethereum_tokens.h"
#include "eth_multisig_wallet.h"
#include "storage.h"

/* maximum supported chain id.  v must fit in an uint32_t. */
#define MAX_CHAIN_ID 2147483630

static bool ethereum_signing = false;
static uint32_t data_total, data_left;
static EthereumTxRequest msg_tx_request;
static CONFIDENTIAL uint8_t privkey[32];
static uint32_t chain_id;
struct SHA3_CTX keccak_ctx;

#define DEBUG_HASH_DATA 0
#if DEBUG_HASH_DATA
static char hash_src[1024];
static uint32_t hash_pos;
#endif

static uint8_t multisig_threshold = 0;
static uint8_t multisig_owner_count = 0;
static inline void hash_data(const uint8_t *buf, size_t size)
{
	sha3_Update(&keccak_ctx, buf, size);

#if DEBUG_HASH_DATA
    data2hex(buf, size, hash_src + hash_pos);
    hash_pos += size * 2;
#endif
}

/*
 * Push an RLP encoded length to the hash buffer.
 */
static void hash_rlp_length(uint32_t length, uint8_t firstbyte)
{
	uint8_t buf[4];
	if (length == 1 && firstbyte <= 0x7f) {
		/* empty length header */
	} else if (length <= 55) {
		buf[0] = 0x80 + length;
		hash_data(buf, 1);
	} else if (length <= 0xff) {
		buf[0] = 0xb7 + 1;
		buf[1] = length;
		hash_data(buf, 2);
	} else if (length <= 0xffff) {
		buf[0] = 0xb7 + 2;
		buf[1] = length >> 8;
		buf[2] = length & 0xff;
		hash_data(buf, 3);
	} else {
		buf[0] = 0xb7 + 3;
		buf[1] = length >> 16;
		buf[2] = length >> 8;
		buf[3] = length & 0xff;
		hash_data(buf, 4);
	}
}

/*
 * Push an RLP encoded list length to the hash buffer.
 */
static void hash_rlp_list_length(uint32_t length)
{
	uint8_t buf[4];
	if (length <= 55) {
		buf[0] = 0xc0 + length;
		hash_data(buf, 1);
	} else if (length <= 0xff) {
		buf[0] = 0xf7 + 1;
		buf[1] = length;
		hash_data(buf, 2);
	} else if (length <= 0xffff) {
		buf[0] = 0xf7 + 2;
		buf[1] = length >> 8;
		buf[2] = length & 0xff;
		hash_data(buf, 3);
	} else {
		buf[0] = 0xf7 + 3;
		buf[1] = length >> 16;
		buf[2] = length >> 8;
		buf[3] = length & 0xff;
		hash_data(buf, 4);
	}
}

/*
 * Push an RLP encoded length field and data to the hash buffer.
 */
static void hash_rlp_field(const uint8_t *buf, size_t size)
{
	hash_rlp_length(size, buf[0]);
	hash_data(buf, size);
}

/*
 * Push an RLP encoded number to the hash buffer.
 * Ethereum yellow paper says to convert to big endian and strip leading zeros.
 */
static void hash_rlp_number(uint32_t number)
{
	if (!number) {
		return;
	}
	uint8_t data[4];
	data[0] = (number >> 24) & 0xff;
	data[1] = (number >> 16) & 0xff;
	data[2] = (number >> 8) & 0xff;
	data[3] = (number) & 0xff;
	int offset = 0;
	while (!data[offset]) {
		offset++;
	}
	hash_rlp_field(data + offset, 4 - offset);
}

/*
 * Calculate the number of bytes needed for an RLP length header.
 * NOTE: supports up to 16MB of data (how unlikely...)
 * FIXME: improve
 */
static int rlp_calculate_length(int length, uint8_t firstbyte)
{
	if (length == 1 && firstbyte <= 0x7f) {
		return 1;
	} else if (length <= 55) {
		return 1 + length;
	} else if (length <= 0xff) {
		return 2 + length;
	} else if (length <= 0xffff) {
		return 3 + length;
	} else {
		return 4 + length;
	}
}

static void send_request_chunk(void)
{
	int progress = 1000 - (data_total > 1000000
						   ? data_left / (data_total/800)
						   : data_left * 800 / data_total);
	switch (storage_getLang()) {
		case CHINESE :
			layoutProgress("签名中#.##.##.#", progress);
			break;
		default :
			layoutProgress(_("Signing"), progress);
			break;
	}
	msg_tx_request.has_data_length = true;
	msg_tx_request.data_length = data_left <= 1024 ? data_left : 1024;
	msg_write(MessageType_MessageType_EthereumTxRequest, &msg_tx_request);
}

static int ethereum_is_canonic(uint8_t v, uint8_t signature[64])
{
	(void) signature;
	return (v & 2) == 0;
}

static void send_signature(void)
{
	uint8_t hash[32], sig[64];
	uint8_t v;
	switch (storage_getLang())
	{
		case CHINESE:
			layoutProgress("签名中#.##.##.#", 1000);		
			break;		
		default:
			layoutProgress(_("Signing"),1000);
			break;
	}
	/* eip-155 replay protection */
	if (chain_id != 0) {
		/* hash v=chain_id, r=0, s=0 */
		hash_rlp_number(chain_id);
		hash_rlp_length(0, 0);
		hash_rlp_length(0, 0);
	}

	keccak_Final(&keccak_ctx, hash);
	if (ecdsa_sign_digest(&secp256k1, privkey, hash, sig, &v, ethereum_is_canonic) != 0) {
		fsm_sendFailure(FailureType_Failure_ProcessError, _("Signing failed"));
		ethereum_signing_abort();
		return;
	}

	memset(privkey, 0, sizeof(privkey));

	/* Send back the result */
	msg_tx_request.has_data_length = false;

	msg_tx_request.has_signature_v = true;
	if (chain_id) {
		msg_tx_request.signature_v = v + 2 * chain_id + 35;
	} else {
		msg_tx_request.signature_v = v + 27;
	}

	msg_tx_request.has_signature_r = true;
	msg_tx_request.signature_r.size = 32;
	memcpy(msg_tx_request.signature_r.bytes, sig, 32);

	msg_tx_request.has_signature_s = true;
	msg_tx_request.signature_s.size = 32;
	memcpy(msg_tx_request.signature_s.bytes, sig + 32, 32);

	msg_write(MessageType_MessageType_EthereumTxRequest, &msg_tx_request);

	ethereum_signing_abort();
}
/* Format a 256 bit number (amount in wei) into a human readable format
 * using standard ethereum units.
 * The buffer must be at least 25 bytes.
 */
static void ethereumFormatAmount(const bignum256 *amnt,TokenType *token, char *buf, int buflen)
{
	bignum256 bn1e9;
	bn_read_uint32(1000000000, &bn1e9);
	char *suffix = NULL;
	int decimals = 18;
	if (token == UnknownToken) {
		strlcpy(buf, "Unknown token value", buflen);
		return;
	} else
	if (token != NULL) {
		suffix = token->ticker;
		decimals = token->decimals;
	} else
	if (bn_is_less(amnt, &bn1e9)) {
		suffix = " Wei";
		decimals = 0;
	} else {
		switch (chain_id) {
			case  1: suffix = " ETH";  break;  // Ethereum Mainnet
			case 61: suffix = " ETC";  break;  // Ethereum Classic Mainnet
			case 62: suffix = " tETC"; break;  // Ethereum Classic Testnet
			case 30: suffix = " RSK";  break;  // Rootstock Mainnet
			case 31: suffix = " tRSK"; break;  // Rootstock Testnet
			case  3: suffix = " tETH"; break;  // Ethereum Testnet: Ropsten
			case  4: suffix = " tETH"; break;  // Ethereum Testnet: Rinkeby
			case 42: suffix = " tETH"; break;  // Ethereum Testnet: Kovan
			case  2: suffix = " EXP";  break;  // Expanse
			case  8: suffix = " UBQ";  break;  // UBIQ
			default: suffix = " UNKN"; break;  // unknown chain
		}
	}
	bn_format(amnt, NULL, suffix, decimals, 0, false, buf, buflen);
}

// static void layoutEthereumConfirmSubmitMultisigTx()
// {
// 	int i = 0;
// 	return i;
// }

// static void layoutEthereumConfirmConfirmMultisigTx()
// {
// 	int i = 0;
// 	return i;
// }

static void layoutEthereumConfirmTx(const uint8_t *to, uint32_t to_len, const uint8_t *value, uint32_t value_len, TokenType *token)
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
			ethereumFormatAmount(&val, NULL, amount, sizeof(amount));
		}
	} else {
		ethereumFormatAmount(&val, token, amount, sizeof(amount));
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

static void layoutEthereumData(const uint8_t *data, uint32_t len, uint32_t total_len)
{
	char hexdata[3][17];
	char summary[20];
	uint32_t printed = 0;
	for (int i = 0; i < 3; i++) {
		uint32_t linelen = len - printed;
		if (linelen > 8) {
			linelen = 8;
		}
		data2hex(data, linelen, hexdata[i]);
		data += linelen;
		printed += linelen;
	}

	strcpy(summary, "...          bytes");
	char *p = summary + 11;
	uint32_t number = total_len;
	while (number > 0) {
		*p-- = '0' + number % 10;
		number = number / 10;
	}
	char *summarystart = summary;
	if (total_len == printed)
		summarystart = summary + 4;

	layoutDialogSwipe(&bmp_icon_question,
		_("Cancel"),
		_("Confirm"),
		NULL,
		_("Transaction data:"),
		hexdata[0],
		hexdata[1],
		hexdata[2],
		summarystart,
		NULL
	);
}

static void layoutEthereumFee(const uint8_t *value, uint32_t value_len,
							  const uint8_t *gas_price, uint32_t gas_price_len,
							  const uint8_t *gas_limit, uint32_t gas_limit_len,
							  TokenType *token)
{
	bignum256 val, gas;
	uint8_t pad_val[32];
	char tx_value[32];
	char gas_value[32];

	memset(pad_val, 0, sizeof(pad_val));
	memcpy(pad_val + (32 - gas_price_len), gas_price, gas_price_len);
	bn_read_be(pad_val, &val);

	memset(pad_val, 0, sizeof(pad_val));
	memcpy(pad_val + (32 - gas_limit_len), gas_limit, gas_limit_len);
	bn_read_be(pad_val, &gas);
	bn_multiply(&val, &gas, &secp256k1.prime);

	ethereumFormatAmount(&gas, NULL, gas_value, sizeof(gas_value));

	memset(pad_val, 0, sizeof(pad_val));
	memcpy(pad_val + (32 - value_len), value, value_len);
	bn_read_be(pad_val, &val);

	ethereumFormatAmount(&val, token, tx_value, sizeof(tx_value));

	layoutDialogSwipe(&bmp_icon_question,
		_("Cancel"),
		_("Confirm"),
		NULL,
		_("Really send"),
		tx_value,
		_("paying up to"),
		gas_value,
		_("for gas?"),
		NULL
	);
}

static void layoutEthereumConfirmMultiSig(
        const uint8_t *contract_address,
        uint32_t tx_id,
        const uint8_t *gas_price, uint32_t gas_price_len,
        const uint8_t *gas_limit, uint32_t gas_limit_len
) {
    bignum256 val, gas;
    uint8_t pad_val[32];
    char confirm[32];
    char gas_value[32];
    char hex1[22];
    char hex2[22];

    memset(pad_val, 0, sizeof(pad_val));
    memcpy(pad_val + (32 - gas_price_len), gas_price, gas_price_len);
    bn_read_be(pad_val, &val);

    memset(pad_val, 0, sizeof(pad_val));
    memcpy(pad_val + (32 - gas_limit_len), gas_limit, gas_limit_len);
    bn_read_be(pad_val, &gas);
    bn_multiply(&val, &gas, &secp256k1.prime);

    ethereumFormatAmount(&gas, NULL, gas_value, sizeof(gas_value));
    strcat(gas_value, " ?");

    data2hex(contract_address, 10, hex1);
    hex1[20] = 0;
    data2hex(contract_address + 10, 10, hex2);
    hex2[20] = 0;

    snprintf(confirm, sizeof(confirm), "for multisig tx %d", (unsigned)tx_id);

    layoutDialogSwipe(NULL,
                      _("Cancel"),
                      _("Confirm"),
                      NULL,
                      _("Exec confirm on contract"),
                      hex1,
                      hex2,
                      confirm,
                      _("paying gas up to"),
                      gas_value
    );
}

/*
 * RLP fields:
 * - nonce (0 .. 32)
 * - gas_price (0 .. 32)
 * - gas_limit (0 .. 32)
 * - to (0, 20)
 * - value (0 .. 32)
 * - data (0 ..)
 */

static bool ethereum_signing_check(EthereumSignTx *msg)
{
	if (!msg->has_gas_price || !msg->has_gas_limit) {
		return false;
	}
	
	if (msg->to.size != 20 && msg->to.size != 0) {
		/* Address has wrong length */
		return false;
	}

	// sending transaction to address 0 (contract creation) without a data field
	if (msg->to.size == 0 && (!msg->has_data_length || msg->data_length == 0)) {
		return false;
	}

	if (msg->gas_price.size + msg->gas_limit.size  > 30) {
		// sanity check that fee doesn't overflow
		return false;
	}

	return true;
}

static bool ethereum_generate_multisig_signing_check(EthereumSignGenerateMultisigContract *msg) 
{
	return msg->owners_count > 0;	
}

static bool ethereum_signing_multisig_check(EthereumSignSubmitMultisigTx *msg)
{
	if (!msg->has_multisig_address || msg->multisig_address.size != 20) {
		return false;
	}
	if (!msg->has_gas_price || !msg->has_gas_limit) {
		return false;
	}
	
	if (msg->to.size != 20 && msg->to.size != 0) {
		/* Address has wrong length */
		return false;
	}
	
	if (msg->gas_price.size + msg->gas_limit.size  > 30) {
		// sanity check that fee doesn't overflow
		return false;
	}

	return true;
}

static bool ethereum_signing_confirm_check(EthereumSignConfirmMultisigTx *msg) 
{
	if (!msg->has_multisig_address || msg->multisig_address.size != 20) {
		return false;
	}

	if (!msg->has_gas_price || !msg->has_gas_limit) {
		return false;
	}

	if (msg->to.size != 20 && msg->to.size != 0) {
		/* Address has wrong length */
		return false;
	}

	if (msg->gas_price.size + msg->gas_limit.size  > 30) {
		// sanity check that fee doesn't overflow
		return false;
	}

	return true;
}

/***********************************/
static const uint8_t *pubkeytoken[5] = {
(uint8_t *)"\x04\xCF\x97\xF4\x76\xD5\x84\xDD\x2C\x0F\x61\x32\x15\x99\xF1\x62\x0C\xF4\xF1\x1A\xF4\xCF\x6E\x0F\xBD\x17\x24\xA1\x60\x8C\x48\x99\xDA\x6A\xA7\xC4\x51\xC2\xF8\xAE\x3F\xEE\x92\x48\x89\xF2\x84\xAC\x48\x52\xEA\xAD\xC6\x44\xFA\x9B\x98\x8E\xD2\xD3\xD1\x31\x85\xD6\xF6",
	};
int signatures_ok_Alltoken(EthereumSignTx *msg)
{
	unsigned char bufunsigne[1+20+32+4];
	uint8_t hash[32];
	unsigned char i=0;

    bufunsigne[i]=msg->chain_id;
	i++;
	memcpy(&bufunsigne[i],msg->to.bytes,20);
	i=i+20;
	memcpy(&bufunsigne[i],msg->TOKENticker.bytes,msg->TOKENticker.size);
	i=i+msg->TOKENticker.size;
	bufunsigne[i]=(unsigned char)msg->TOKENdecimals;
	i++;
	sha256_Raw(bufunsigne,i, hash);

	if (ecdsa_verify_digest(&secp256k1, pubkeytoken[msg->TOKENpublickeynumber],msg->TOKENsignedfortickerhash.bytes, hash) != 0) { // failure
		return 0;
	}
	
	return 1;
}

int verify_token_sig(uint32_t msg_chain_id, uint8_t to[], uint8_t ticker_bytes[], uint32_t ticker_size
        , uint32_t TOKENdecimals, uint32_t TOKENpublickeynumber, uint8_t TOKENsignedfortickerhash[])
{
    unsigned char bufunsigne[1+20+32+4];
    uint8_t hash[32];
    unsigned char i=0;

    bufunsigne[i] = msg_chain_id;
    i++;
    memcpy(&bufunsigne[i], to, 20);
    i += 20;
    memcpy(&bufunsigne[i], ticker_bytes, ticker_size);
    i += ticker_size;
    bufunsigne[i] = (unsigned char)TOKENdecimals;
    i++;
    sha256_Raw(bufunsigne,i, hash);

    if (ecdsa_verify_digest(&secp256k1, pubkeytoken[TOKENpublickeynumber], TOKENsignedfortickerhash, hash) != 0) { // failure
        return 0;
    }

    return 1;
}

/**********************************/
static void layoutCreateMultisigWalletFee(const uint8_t *gas_price, uint32_t gas_price_len,
							  const uint8_t *gas_limit, uint32_t gas_limit_len)
{
	bignum256 val, gas;
	uint8_t pad_val[32];
	char tx_value[32];
	char gas_value[32];

	memset(pad_val, 0, sizeof(pad_val));
	memcpy(pad_val + (32 - gas_price_len), gas_price, gas_price_len);
	bn_read_be(pad_val, &val);

	memset(pad_val, 0, sizeof(pad_val));
	memcpy(pad_val + (32 - gas_limit_len), gas_limit, gas_limit_len);
	bn_read_be(pad_val, &gas);
	bn_multiply(&val, &gas, &secp256k1.prime);

	ethereumFormatAmount(&gas, NULL, gas_value, sizeof(gas_value));
	strcpy(tx_value,  _("Multisig Wallet"));
	layoutDialogSwipe(
		&bmp_icon_question,
		_("Cancel"),
		_("Confirm"),
		NULL,
		_("Really Create"),
		tx_value,
		_("paying up to"),
		gas_value,
		_("for gas?"),
		NULL
	);
}

static void layoutEthereumConfirmMultisigWalletType(const uint32_t owner_count, const uint32_t threshold)
{
	char type[] = "# of #";
	type[0] = (uint8_t) (threshold + 48);
	type[5] = (uint8_t) (owner_count + 48);
	layoutDialogSwipe(
		&bmp_icon_warning,
		_("Cancel"),
		_("Continue"),
		NULL,
		_("Multisig Wallet"),
		_("Type:"),
		NULL,
		_(type), 
		NULL,
		NULL
	);
}

static void layoutEthereumConfirmMultisigWalletOwner(const uint8_t *owner, uint8_t pos)
{
	char posstr[] = "### owner:";
	posstr[0] = pos + 49;
	if(pos == 0) {
		posstr[1] = 's';
		posstr[2] = 't';
	} else if(pos == 1) {
		posstr[1] = 'n';
		posstr[2] = 'd';
	} else if(pos == 2) {
		posstr[1] = 'e';
		posstr[2] = 'd';
	} else {
		posstr[1] = 't';
		posstr[2] = 'h';
	}
	char ownerstr1[18];
	char ownerstr1t[18];
	char ownerstr2[18];
	char ownerstr3[18];
	data2hex(owner, 8, ownerstr1t);
	memcpy(ownerstr1+2, ownerstr1t, 16);
	ownerstr1[0] = '0';
	ownerstr1[1] = 'x';
	data2hex(owner+8, 9, ownerstr2);
	data2hex(owner+17, 3, ownerstr3);
	layoutDialogSwipe(
		&bmp_icon_warning,
		_("Cancel"),
		_("Continue"),
		NULL,
		_("Multisig Wallet"),
		_(posstr),
		_(ownerstr1),
		_(ownerstr2),
		_(ownerstr3),
		NULL
	);
}

void ethereum_confirm_multisig_tx(EthereumSignConfirmMultisigTx *msg, const HDNode *node) 
{
	ethereum_signing = true;
	sha3_256_Init(&keccak_ctx);

	memset(&msg_tx_request, 0, sizeof(EthereumTxRequest));

	if (!msg->has_value)
		msg->value.size = 0;
	if (!msg->has_to)
		msg->to.size = 0;
	if (!msg->has_nonce)
		msg->nonce.size = 0;
	
	if (msg->has_chain_id) {
		if (msg->chain_id < 1 || msg->chain_id > MAX_CHAIN_ID) {
			fsm_sendFailure(FailureType_Failure_DataError, _("Chain Id out of bounds"));
			ethereum_signing_abort();
			return;
		}
		chain_id = msg->chain_id;
	} else {
		chain_id = 0;
	}

	data_total = 36;

	// safety checks
	if (!ethereum_signing_confirm_check(msg)) {
		fsm_sendFailure(FailureType_Failure_DataError, _("Safety check failed"));
		ethereum_signing_abort();
		return;
	}

	// TODO layout confirm tx information.

	layoutEthereumConfirmMultiSig(msg->multisig_address.bytes, msg->multisig_tx_id
	        , msg->gas_price.bytes, msg->gas_price.size, msg->gas_limit.bytes, msg->gas_limit.size);
	if (!protectButton(ButtonRequestType_ButtonRequest_SignTx, false)) {
		fsm_sendFailure(FailureType_Failure_ActionCancelled, NULL);
		ethereum_signing_abort();
		return;
	}

	uint8_t abi_tx_id[4];
	abi_tx_id[0] = (msg->multisig_tx_id >> 24) & 0xff;
	abi_tx_id[1] = (msg->multisig_tx_id >> 16) & 0xff;
	abi_tx_id[2] = (msg->multisig_tx_id >> 8) & 0xff;
	abi_tx_id[3] = msg->multisig_tx_id & 0xff; 

	// stage 1: calculate total rlp length
	uint32_t rlp_length = 0;
	
	switch (storage_getLang())
	{
		case CHINESE:
			layoutProgress("签名中#.##.##.#", 0);		
			break;		
		default:
			layoutProgress(_("Signing"),0);
			break;
	}
	rlp_length += rlp_calculate_length(msg->nonce.size, msg->nonce.bytes[0]);
	rlp_length += rlp_calculate_length(msg->gas_price.size, msg->gas_price.bytes[0]);
	rlp_length += rlp_calculate_length(msg->gas_limit.size, msg->gas_limit.bytes[0]);
	rlp_length += rlp_calculate_length(msg->multisig_address.size, msg->multisig_address.bytes[0]);
	rlp_length += rlp_calculate_length(0, 0);
	rlp_length += rlp_calculate_length(data_total, 0);
	if (chain_id) {
		rlp_length += rlp_calculate_length(1, chain_id);
		rlp_length += rlp_calculate_length(0, 0);
		rlp_length += rlp_calculate_length(0, 0);
	}

	// stage 2: story header fields:
	hash_rlp_list_length(rlp_length);

	switch (storage_getLang())
	{
		case CHINESE:
			layoutProgress("签名中#.##.##.#", 100);		
			break;		
		default:
			layoutProgress(_("Signing"),100);
			break;
	}

	hash_rlp_field(msg->nonce.bytes, msg->nonce.size);
	hash_rlp_field(msg->gas_price.bytes, msg->gas_price.size);
	hash_rlp_field(msg->gas_limit.bytes, msg->gas_limit.size);
	hash_rlp_field(msg->multisig_address.bytes, msg->multisig_address.size);
	hash_rlp_field(0, 0);
	hash_rlp_length(data_total, method_confirm[0]);
	hash_data(method_confirm, 4);
	hash_data(uint32_start, 28);
	hash_data(abi_tx_id, 4);

	memcpy(privkey, node->private_key, 32);
	send_signature();
}

#define MAX_MULTISIG_DATA_LEN 1024
void ethereum_submit_multisig_tx(EthereumSignSubmitMultisigTx *msg, const HDNode *node) 
{
	ethereum_signing = true;
	sha3_256_Init(&keccak_ctx);
#if DEBUG_HASH_DATA
    hash_pos = 0;
#endif

	memset(&msg_tx_request, 0, sizeof(EthereumTxRequest));

	if (!msg->has_value)
		msg->value.size = 0;
	if (!msg->has_to)
		msg->to.size = 0;
	if (!msg->has_nonce)
		msg->nonce.size = 0;

	if (msg->has_chain_id) {
		if (msg->chain_id < 1 || msg->chain_id > MAX_CHAIN_ID) {
			fsm_sendFailure(FailureType_Failure_DataError, _("Chain Id out of bounds"));
			ethereum_signing_abort();
			return;
		}
		chain_id = msg->chain_id;
	} else {
		chain_id = 0;
	}

	bool is_token = msg->has_data_initial_chunk;
	uint32_t chunk_size = is_token ? msg->data_initial_chunk.size : 0;
	if (chunk_size && chunk_size > MAX_MULTISIG_DATA_LEN) {
		fsm_sendFailure(FailureType_Failure_DataError, _("Exceeded max data length."));
		ethereum_signing_abort();
		return;
	}

//    debug_print("chunk_size", "%d", chunk_size);

    uint32_t chunk_packed_len =  chunk_size == 0 ? 32 : (chunk_size + 31) / 32 * 32;
	data_total = 68 + 64 + chunk_packed_len;

//    debug_print("chunk_packed_len", "%d", chunk_packed_len);
//    debug_print("data_total", "%d", data_total);

    uint8_t chunk_pack_zero[32];
    memset(chunk_pack_zero, 0, sizeof(chunk_pack_zero));

    uint8_t chunk_size_u256[32];
	memset(chunk_size_u256, 0, sizeof(chunk_size_u256));
	chunk_size_u256[28] = chunk_size >> 24;
    chunk_size_u256[29] = chunk_size >> 16;
    chunk_size_u256[30] = chunk_size >> 8;
    chunk_size_u256[31] = chunk_size;

    // detect token and parse transfer arguments
    uint8_t *to_bytes;
    uint32_t to_len;
    uint8_t *amount_bytes;
    uint32_t amount_len;
    TokenType *token = NULL;
    TokenType TOKEN;
    if (is_token) {
        token = tokenByChainAddress(chain_id, msg->to.bytes);
        if (token == UnknownToken) {
            // int verify_token_sig(uint32_t msg_chain_id, uint8_t to[], uint8_t ticker_bytes[], uint32_t ticker_size
            //        , uint32_t TOKENdecimals, uint32_t TOKENpublickeynumber, uint8_t TOKENsignedfortickerhash[])
            if (verify_token_sig(msg->chain_id, msg->to.bytes, msg->TOKENticker.bytes, msg->TOKENticker.size
                    , msg->TOKENdecimals, msg->TOKENpublickeynumber, msg->TOKENsignedfortickerhash.bytes)) {
                TOKEN.chain_id = chain_id;
                memcpy(TOKEN.address, msg->to.bytes, 20);
                memcpy(TOKEN.ticker, msg->TOKENticker.bytes, msg->TOKENticker.size);
                TOKEN.ticker[msg->TOKENticker.size] = 0;
                TOKEN.decimals = msg->TOKENdecimals;
                token = &TOKEN;
            } else {
                token = UnknownToken;
            }
        }
        // parse chunk data as ERC20 transfer
        static uint8_t TRANSFER_SIG[4] = {0xa9, 0x05, 0x9c, 0xbb};
        // check method sig
        if (memcmp(TRANSFER_SIG, msg->data_initial_chunk.bytes, 4)) {
            fsm_sendFailure(FailureType_Failure_DataError, _("Invalid method signature in data field"));
            ethereum_signing_abort();
            return;
        }
        if (msg->data_initial_chunk.size != 68) {
            fsm_sendFailure(FailureType_Failure_DataError, _("Invalid arguments data size"));
            ethereum_signing_abort();
            return;
        }
        to_bytes = &msg->data_initial_chunk.bytes[4 + 12];
        to_len = 20;
        amount_bytes = &msg->data_initial_chunk.bytes[4 + 32];
        amount_len = 32;
    } else {
        to_bytes = msg->to.bytes;
        to_len = msg->to.size;
        amount_bytes = msg->value.bytes;
        amount_len = msg->value.size;
    }

    // safety checks
	if (!ethereum_signing_multisig_check(msg)) {
		fsm_sendFailure(FailureType_Failure_DataError, _("Safety check failed"));
		ethereum_signing_abort();
		return;
	}

	// todo layout ethereum form
	layoutEthereumConfirmTx(to_bytes, to_len, amount_bytes, amount_len, token);
	if (!protectButton(ButtonRequestType_ButtonRequest_SignTx, false)) {
		fsm_sendFailure(FailureType_Failure_ActionCancelled, NULL);
		ethereum_signing_abort();
		return;
	}

	layoutEthereumFee(amount_bytes, amount_len,
					  msg->gas_price.bytes, msg->gas_price.size,
					  msg->gas_limit.bytes, msg->gas_limit.size, token);
	if (!protectButton(ButtonRequestType_ButtonRequest_SignTx, false)) {
		fsm_sendFailure(FailureType_Failure_ActionCancelled, NULL);
		ethereum_signing_abort();
		return;
	}

	// stage 1: calculate total rlp length
	uint32_t rlp_length = 0;
	
	switch (storage_getLang())
	{
		case CHINESE:
			layoutProgress("签名中#.##.##.#", 0);		
			break;		
		default:
			layoutProgress(_("Signing"),0);
			break;
	}
	rlp_length += rlp_calculate_length(msg->nonce.size, msg->nonce.bytes[0]);
	rlp_length += rlp_calculate_length(msg->gas_price.size, msg->gas_price.bytes[0]);
	rlp_length += rlp_calculate_length(msg->gas_limit.size, msg->gas_limit.bytes[0]);
	rlp_length += rlp_calculate_length(msg->multisig_address.size, msg->multisig_address.bytes[0]);
	rlp_length += rlp_calculate_length(0, 0);
	rlp_length += rlp_calculate_length(data_total, 0);
	if (chain_id) {
		rlp_length += rlp_calculate_length(1, chain_id);
		rlp_length += rlp_calculate_length(0, 0);
		rlp_length += rlp_calculate_length(0, 0);
	}

	// stage 2: story header fields:
	hash_rlp_list_length(rlp_length);

	uint8_t abi_value[32];
	memset(abi_value, 0, 32);
	memcpy(abi_value + 32 - (msg->value.size), msg->value.bytes, msg->value.size);

	switch (storage_getLang())
	{
		case CHINESE:
			layoutProgress("签名中#.##.##.#", 100);		
			break;		
		default:
			layoutProgress(_("Signing"),100);
			break;
	}
	hash_rlp_field(msg->nonce.bytes, msg->nonce.size);
	hash_rlp_field(msg->gas_price.bytes, msg->gas_price.size);
	hash_rlp_field(msg->gas_limit.bytes, msg->gas_limit.size);
	hash_rlp_field(msg->multisig_address.bytes, msg->multisig_address.size);
	hash_rlp_field(0, 0);
	hash_rlp_length(data_total, method_submit_tx[0]);
	hash_data(method_submit_tx, 4);
	hash_data(address_start, 12);
//    debug_print_binary("hashed address_start", address_start, 12);
	hash_data(msg->to.bytes, 20);
//    debug_print_binary("hashed to.bytes", msg->to.bytes, 20);

    hash_data(abi_value, 32);
//    debug_print_binary("hashed abi_value", abi_value, sizeof(abi_value));

    hash_data(submit_end, 32);
    hash_data(chunk_size_u256, 32);
//    debug_print_binary("hashed size", chunk_size_u256, sizeof(chunk_size_u256));
    if (chunk_size > 0) {
        hash_data(msg->data_initial_chunk.bytes, chunk_size);
//        debug_print("hashed chunk len", "%d", chunk_size);
//        debug_print_binary("hashed chunk", msg->data_initial_chunk.bytes, chunk_size);
    }
    if (chunk_size < chunk_packed_len) {
        hash_data(chunk_pack_zero, chunk_packed_len - chunk_size);
//        debug_print("hashed zero len", "%d", chunk_packed_len - chunk_size);
//        debug_print_binary("hashed cpz", chunk_pack_zero, chunk_packed_len - chunk_size);
    }

	memcpy(privkey, node->private_key, 32);

#if DEBUG_HASH_DATA
    hash_src[hash_pos] = 0;
    fsm_sendFailure(FailureType_Failure_DataError, hash_src);
#else
    send_signature();
#endif
}

void ethereum_generate_multisig_signing_init(EthereumSignGenerateMultisigContract *msg, const HDNode *node)
{
	if (msg->owners_count < 2 || msg->owners_count > 50) {
		fsm_sendFailure(FailureType_Failure_DataError, _("owners count error."));
		return;
	}
	for (size_t i = 0; i < msg->owners_count; i++) {
		if (msg->owners[i].size != 20) {
			fsm_sendFailure(FailureType_Failure_DataError, _("owner address error."));
			return;
		}
	}
	if (msg->threshold > msg->owners_count || msg->threshold < 1) {
		fsm_sendFailure(FailureType_Failure_DataError, _("threshold error."));
		return;
	}

	ethereum_signing = true;
	sha3_256_Init(&keccak_ctx);

	memset(&msg_tx_request, 0, sizeof(EthereumTxRequest));

	if (msg->has_chain_id) {
		if (msg->chain_id < 1 || msg->chain_id > MAX_CHAIN_ID) {
			fsm_sendFailure(FailureType_Failure_DataError, _("Chain Id out of bounds"));
			ethereum_signing_abort();
			return;
		}
		chain_id = msg->chain_id;
	} else {
		chain_id = 0;
	}

	// safety checks
	if (!ethereum_generate_multisig_signing_check(msg)) {
		fsm_sendFailure(FailureType_Failure_DataError, _("Safety check failed"));
		ethereum_signing_abort();
		return;
	}

	layoutEthereumConfirmMultisigWalletType(msg->owners_count, msg->threshold);
	if (!protectButton(ButtonRequestType_ButtonRequest_SignTx, false)) {
		fsm_sendFailure(FailureType_Failure_ActionCancelled, NULL);
		ethereum_signing_abort();
		return;
	}

	for (size_t i = 0; i < msg->owners_count; i++) {
		layoutEthereumConfirmMultisigWalletOwner(msg->owners[i].bytes, (uint8_t)i);
		if (!protectButton(ButtonRequestType_ButtonRequest_SignTx, false)) {
			fsm_sendFailure(FailureType_Failure_ActionCancelled, NULL);
			ethereum_signing_abort();
			return;
		}
	}

	layoutCreateMultisigWalletFee(
					  msg->gas_price.bytes, msg->gas_price.size,
					  msg->gas_limit.bytes, msg->gas_limit.size);

	if (!protectButton(ButtonRequestType_ButtonRequest_SignTx, false)) {
		fsm_sendFailure(FailureType_Failure_ActionCancelled, NULL);
		ethereum_signing_abort();
		return;
	}

	data_total = ETH_MULTISIG_CONTRACT_LENGTH + ((3 + msg->owners_count) * 32);

	/* Stage 1: Calculate total RLP length */
	// uint32_t rlp_length = 0;

	switch (storage_getLang())
	{
		case CHINESE:
			layoutProgress("签名中#.##.##.#", 0);		
			break;		
		default:
			layoutProgress(_("Signing"),0);
			break;
	}
	uint32_t rlp_length = 0;
	rlp_length += rlp_calculate_length(msg->nonce.size, msg->nonce.bytes[0]); // nonce
	rlp_length += rlp_calculate_length(msg->gas_price.size, msg->gas_price.bytes[0]); // price
	rlp_length += rlp_calculate_length(msg->gas_limit.size, msg->gas_limit.bytes[0]); // limit
	rlp_length += rlp_calculate_length(0, 0); // to
	rlp_length += rlp_calculate_length(0, 0); // value
	rlp_length += rlp_calculate_length(data_total, multisig_wallet_contract[0]);
	if (chain_id) {
		rlp_length += rlp_calculate_length(1, chain_id);
		rlp_length += rlp_calculate_length(0, 0);
		rlp_length += rlp_calculate_length(0, 0);
	}

	/* Stage 2: Store header fields */
	hash_rlp_list_length(rlp_length);
	switch (storage_getLang())
	{
		case CHINESE:
			layoutProgress("签名中#.##.##.#", 100);		
			break;		
		default:
			layoutProgress(_("Signing"),100);
			break;
	}

	hash_rlp_field(msg->nonce.bytes, msg->nonce.size);
	hash_rlp_field(msg->gas_price.bytes, msg->gas_price.size);
	hash_rlp_field(msg->gas_limit.bytes, msg->gas_limit.size);
	hash_rlp_field(0, 0);
	hash_rlp_field(0, 0);
	// multisig contract
	hash_rlp_length(data_total, multisig_wallet_contract[0]);
	for (int i=0; i< ETH_MULTISIG_CONTRACT_LENGTH; ){
		int next = i + 1024;
		if (next < ETH_MULTISIG_CONTRACT_LENGTH) {
			hash_data(multisig_wallet_contract + i, 1024);
		} else {
			hash_data(multisig_wallet_contract + i, ETH_MULTISIG_CONTRACT_LENGTH - i);
		}
		i = next;
	}
	// multisig contract params
	hash_data(params_start, 63);
	multisig_threshold = (uint8_t)msg->threshold;
	hash_data(&multisig_threshold, 1);
	hash_data(length_start, 31);
	multisig_owner_count = (uint8_t)msg->owners_count;
	hash_data(&multisig_owner_count, 1);
	for (size_t i = 0; i < msg->owners_count; i ++ ) {
		hash_data(address_start, 12);
		hash_data(msg->owners[i].bytes, 20);
	}

	memcpy(privkey, node->private_key, 32);

	send_signature();
}

void ethereum_signing_init(EthereumSignTx *msg, const HDNode *node)
{
	ethereum_signing = true;
	sha3_256_Init(&keccak_ctx);

	memset(&msg_tx_request, 0, sizeof(EthereumTxRequest));
	/* set fields to 0, to avoid conditions later */
	if (!msg->has_value)
		msg->value.size = 0;
	if (!msg->has_data_initial_chunk)
		msg->data_initial_chunk.size = 0;
	if (!msg->has_to)
		msg->to.size = 0;
	if (!msg->has_nonce)
		msg->nonce.size = 0;

	/* eip-155 chain id */
	if (msg->has_chain_id) {
		if (msg->chain_id < 1 || msg->chain_id > MAX_CHAIN_ID) {
			fsm_sendFailure(FailureType_Failure_DataError, _("Chain Id out of bounds"));
			ethereum_signing_abort();
			return;
		}
		chain_id = msg->chain_id;
	} else {
		chain_id = 0;
	}

	if (msg->has_data_length && msg->data_length > 0) {
		if (!msg->has_data_initial_chunk || msg->data_initial_chunk.size == 0) {
			fsm_sendFailure(FailureType_Failure_DataError, _("Data length provided, but no initial chunk"));
			ethereum_signing_abort();
			return;
		}
		/* Our encoding only supports transactions up to 2^24 bytes.  To
		 * prevent exceeding the limit we use a stricter limit on data length.
		 */
		if (msg->data_length > 16000000)  {
			fsm_sendFailure(FailureType_Failure_DataError, _("Data length exceeds limit"));
			ethereum_signing_abort();
			return;
		}
		data_total = msg->data_length;
	} else {
		data_total = 0;
	}
	if (msg->data_initial_chunk.size > data_total) {
		fsm_sendFailure(FailureType_Failure_DataError, _("Invalid size of initial chunk"));
		ethereum_signing_abort();
		return;
	}

	// safety checks
	if (!ethereum_signing_check(msg)) {
		fsm_sendFailure(FailureType_Failure_DataError, _("Safety check failed"));
		ethereum_signing_abort();
		return;
	}

	TokenType *token = NULL;
	TokenType TOKEN;

	// detect ERC-20 token
	if (msg->to.size == 20 && msg->value.size == 0 && data_total == 68 && msg->data_initial_chunk.size == 68
	    && memcmp(msg->data_initial_chunk.bytes, "\xa9\x05\x9c\xbb\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", 16) == 0) {
		
		token = tokenByChainAddress(chain_id, msg->to.bytes);
		if(token==UnknownToken)
		{
			if(signatures_ok_Alltoken(msg))
			{
				TOKEN.chain_id=chain_id;
				memcpy(TOKEN.address,msg->to.bytes,20);
				memcpy(TOKEN.ticker,msg->TOKENticker.bytes,msg->TOKENticker.size);
				TOKEN.ticker[msg->TOKENticker.size]=0;
				TOKEN.decimals=msg->TOKENdecimals;
				token=&TOKEN;
			}
			else
			{
				token=UnknownToken;
			}
		}
		///////////////////////////////////////////////////////
	}

	uint8_t *to_bytes;
	uint32_t to_len;
    uint8_t *amount_bytes;
    uint32_t amount_len;
    if (token) {
        to_bytes = msg->data_initial_chunk.bytes + 16;
        to_len = 20;
        amount_bytes = msg->data_initial_chunk.bytes + 36;
        amount_len = 32;
    } else {
        to_bytes = msg->to.bytes;
        to_len = msg->to.size;
        amount_bytes = msg->value.bytes;
        amount_len = msg->value.size;
    }

    layoutEthereumConfirmTx(to_bytes, to_len, amount_bytes, amount_len, token);

	if (!protectButton(ButtonRequestType_ButtonRequest_SignTx, false)) {
		fsm_sendFailure(FailureType_Failure_ActionCancelled, NULL);
		ethereum_signing_abort();
		return;
	}

	if (token == NULL && data_total > 0) {
		layoutEthereumData(msg->data_initial_chunk.bytes, msg->data_initial_chunk.size, data_total);
		if (!protectButton(ButtonRequestType_ButtonRequest_SignTx, false)) {
			fsm_sendFailure(FailureType_Failure_ActionCancelled, NULL);
			ethereum_signing_abort();
			return;
		}
	}

    layoutEthereumFee(amount_bytes, amount_len,
					  msg->gas_price.bytes, msg->gas_price.size,
					  msg->gas_limit.bytes, msg->gas_limit.size, token);
	if (!protectButton(ButtonRequestType_ButtonRequest_SignTx, false)) {
		fsm_sendFailure(FailureType_Failure_ActionCancelled, NULL);
		ethereum_signing_abort();
		return;
	}
	
	/* Stage 1: Calculate total RLP length */
	uint32_t rlp_length = 0;
	switch (storage_getLang())
	{
		case CHINESE:
			layoutProgress("签名中#.##.##.#", 0);		
			break;		
		default:
			layoutProgress(_("Signing"),0);
			break;
	}

	rlp_length += rlp_calculate_length(msg->nonce.size, msg->nonce.bytes[0]);
	rlp_length += rlp_calculate_length(msg->gas_price.size, msg->gas_price.bytes[0]);
	rlp_length += rlp_calculate_length(msg->gas_limit.size, msg->gas_limit.bytes[0]);
	rlp_length += rlp_calculate_length(msg->to.size, msg->to.bytes[0]);
	rlp_length += rlp_calculate_length(msg->value.size, msg->value.bytes[0]);
	rlp_length += rlp_calculate_length(data_total, msg->data_initial_chunk.bytes[0]);
	if (chain_id) {
		rlp_length += rlp_calculate_length(1, chain_id);
		rlp_length += rlp_calculate_length(0, 0);
		rlp_length += rlp_calculate_length(0, 0);
	}

	/* Stage 2: Store header fields */
	hash_rlp_list_length(rlp_length);

	switch (storage_getLang())
	{
		case CHINESE:
			layoutProgress("签名中#.##.##.#", 100);		
			break;		
		default:
			layoutProgress(_("Signing"),100);
			break;
	}

	hash_rlp_field(msg->nonce.bytes, msg->nonce.size);
	hash_rlp_field(msg->gas_price.bytes, msg->gas_price.size);
	hash_rlp_field(msg->gas_limit.bytes, msg->gas_limit.size);
	hash_rlp_field(msg->to.bytes, msg->to.size);
	hash_rlp_field(msg->value.bytes, msg->value.size);
	hash_rlp_length(data_total, msg->data_initial_chunk.bytes[0]);
	hash_data(msg->data_initial_chunk.bytes, msg->data_initial_chunk.size);
	data_left = data_total - msg->data_initial_chunk.size;

	memcpy(privkey, node->private_key, 32);

	if (data_left > 0) {
		send_request_chunk();
	} else {
		send_signature();
	}
}

void ethereum_signing_txack(EthereumTxAck *tx)
{
	if (!ethereum_signing) {
		fsm_sendFailure(FailureType_Failure_UnexpectedMessage, _("Not in Ethereum signing mode"));
		layoutHome();
		return;
	}

	if (tx->data_chunk.size > data_left) {
		fsm_sendFailure(FailureType_Failure_DataError, _("Too much data"));
		ethereum_signing_abort();
		return;
	}

	if (data_left > 0 && (!tx->has_data_chunk || tx->data_chunk.size == 0)) {
		fsm_sendFailure(FailureType_Failure_DataError, _("Empty data chunk received"));
		ethereum_signing_abort();
		return;
	}

	hash_data(tx->data_chunk.bytes, tx->data_chunk.size);

	data_left -= tx->data_chunk.size;

	if (data_left > 0) {
		send_request_chunk();
	} else {
		send_signature();
	}
}

void ethereum_signing_abort(void)
{
	if (ethereum_signing) {
		memset(privkey, 0, sizeof(privkey));
		layoutHome();
		ethereum_signing = false;
	}
}

static void ethereum_message_hash(const uint8_t *message, size_t message_len, uint8_t hash[32])
{
	struct SHA3_CTX ctx;
	sha3_256_Init(&ctx);
	sha3_Update(&ctx, (const uint8_t *)"\x19" "Ethereum Signed Message:\n", 26);
	uint8_t c;
	if (message_len > 1000000000) { c = '0' + message_len / 1000000000 % 10; sha3_Update(&ctx, &c, 1); }
	if (message_len > 100000000)  { c = '0' + message_len / 100000000  % 10; sha3_Update(&ctx, &c, 1); }
	if (message_len > 10000000)   { c = '0' + message_len / 10000000   % 10; sha3_Update(&ctx, &c, 1); }
	if (message_len > 1000000)    { c = '0' + message_len / 1000000    % 10; sha3_Update(&ctx, &c, 1); }
	if (message_len > 100000)     { c = '0' + message_len / 100000     % 10; sha3_Update(&ctx, &c, 1); }
	if (message_len > 10000)      { c = '0' + message_len / 10000      % 10; sha3_Update(&ctx, &c, 1); }
	if (message_len > 1000)       { c = '0' + message_len / 1000       % 10; sha3_Update(&ctx, &c, 1); }
	if (message_len > 100)        { c = '0' + message_len / 100        % 10; sha3_Update(&ctx, &c, 1); }
	if (message_len > 10)         { c = '0' + message_len / 10         % 10; sha3_Update(&ctx, &c, 1); }
	                                c = '0' + message_len              % 10; sha3_Update(&ctx, &c, 1);
	sha3_Update(&ctx, message, message_len);
	keccak_Final(&ctx, hash);
}

void ethereum_message_sign(EthereumSignMessage *msg, const HDNode *node, EthereumMessageSignature *resp)
{
	uint8_t hash[32];

	if (!hdnode_get_ethereum_pubkeyhash(node, resp->address.bytes)) {
		return;
	}
	resp->has_address = true;
	resp->address.size = 20;
	ethereum_message_hash(msg->message.bytes, msg->message.size, hash);

	uint8_t v;
	if (ecdsa_sign_digest(&secp256k1, node->private_key, hash, resp->signature.bytes, &v, ethereum_is_canonic) != 0) {
		fsm_sendFailure(FailureType_Failure_ProcessError, _("Signing failed"));
		return;
	}

	resp->has_signature = true;
	resp->signature.bytes[64] = 27 + v;
	resp->signature.size = 65;
	msg_write(MessageType_MessageType_EthereumMessageSignature, resp);
}

int ethereum_message_verify(EthereumVerifyMessage *msg)
{
	if (msg->signature.size != 65 || msg->address.size != 20) {
		fsm_sendFailure(FailureType_Failure_DataError, _("Malformed data"));
		return 1;
	}

	uint8_t pubkey[65];
	uint8_t hash[32];

	ethereum_message_hash(msg->message.bytes, msg->message.size, hash);

	/* v should be 27, 28 but some implementations use 0,1.  We are
	 * compatible with both.
	 */
	uint8_t v = msg->signature.bytes[64];
	if (v >= 27) {
		v -= 27;
	}
	// if (v >= 2 ||
	// 	ecdsa_verify_digest_recover(&secp256k1, pubkey, msg->signature.bytes, hash, v) != 0) {
	// 	return 2;
	// }

	struct SHA3_CTX ctx;
	sha3_256_Init(&ctx);
	sha3_Update(&ctx, pubkey + 1, 64);
	keccak_Final(&ctx, hash);

	/* result are the least significant 160 bits */
	if (memcmp(msg->address.bytes, hash + 12, 20) != 0) {
		return 2;
	}
	return 0;
}
