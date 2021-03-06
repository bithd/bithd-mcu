/*
 * This file is part of the BitHD project, https://bithd.com/
 *
 * Copyright (C) 2020 Bitpie
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

#include "tron.h"
#include "fsm.h"
#include "layout2.h"
#include "messages.h"
#include "transaction.h"
#include "ecdsa.h"
#include "protect.h"
#include "crypto.h"
#include "secp256k1.h"
#include "sha2.h"
#include "address.h"
#include "util.h"
#include "gettext.h"
#include "storage.h"
#include "base58.h"

#include <pb_decode.h>
#include "tron.pb.h"
#include "tron_err.h"
#include "tron_ui.h"

extern int ethereum_is_canonic(uint8_t v, uint8_t signature[64]);
extern void ethereum_message_hash(const uint8_t *message, size_t message_len, uint8_t hash[32]);

void tron_message_sign(TronSignMessage *msg, const HDNode *node, TronMessageSignature *resp)
{
	uint8_t hash[32];

	if (!hdnode_get_ethereum_pubkeyhash(node, resp->address.bytes)) {
		return;
	}
	resp->has_address = true;
	resp->address.size = 20;
    if (msg->do_hash) {
	    ethereum_message_hash(msg->message.bytes, msg->message.size, hash);
    } else {
        memcpy(hash, msg->message.bytes, 32);
    }

	uint8_t v;
	if (ecdsa_sign_digest_DER(&secp256k1, node->private_key, hash, resp->signature.bytes, &v, ethereum_is_canonic) != 0) {
		fsm_sendFailure(FailureType_Failure_ProcessError, _("Signing failed"));
		return;
	}

	resp->has_signature = true;
	resp->signature.bytes[64] = 27 + v;
	resp->signature.size = 65;
	msg_write(MessageType_MessageType_TronMessageSignature, resp);
}

int tron_eth_2_trx_address(const uint8_t eth_address[20], char *str, int strsize) {
	uint8_t address_bytes[21];
	address_bytes[0] = 0x41;	// Tron address prefix
	memcpy(&address_bytes[1], eth_address, 20);

	int r = base58_encode_check(address_bytes, sizeof(address_bytes), HASHER_SHA2D, str, strsize);
	return r;
}

int decode_transfer_contract(const protocol_Any_value_t *any, char *to_str, unsigned to_str_len, uint64_t *amount, MSG msg) {
	protocol_TransferContract contract;
	pb_istream_t stream = pb_istream_from_buffer(any->bytes, any->size);
	if (!pb_decode(&stream, protocol_TransferContract_fields, &contract))
		return errmsg(msg, E_TRON_DecodeTransferContract, _("Failed to decode TransferContract pb data"));

	*amount = contract.amount;

	if (contract.to_address.size != 21 && contract.to_address.bytes[0] != 0x41)
		return errmsg(msg, E_TRON_DecodeTransferContract, _("Invalid to_address in TransferContract pb data"));

	if (tron_eth_2_trx_address(&contract.to_address.bytes[1], to_str, to_str_len) < 34)
		return errmsg(msg, E_TRON_EncodeTronAddress, _("Failed to encode to Tron address"));

	return 0;
}

int decode_trc20_contract(const protocol_Any_value_t *any, char *to_str, unsigned to_str_len, uint8_t *value_bytes, ConstTronTokenPtr *p_token, MSG msg) {
	protocol_TriggerSmartContract contract;
	pb_istream_t stream = pb_istream_from_buffer(any->bytes, any->size);
	if (!pb_decode(&stream, protocol_TriggerSmartContract_fields, &contract))
		return errmsg(msg, E_TRON_DecodeTriggerSmartContract, _("Failed to decode TriggerSmartContract pb data"));

	if (contract.contract_address.size != 21 && contract.contract_address.bytes[0] != 0x41)
		return errmsg(msg, E_TRON_InvalidAddress, _("Invalid Tron address"));

	if (contract.data.size < 4)
		return errmsg(msg, E_TRON_InvalidCallData, _("Invalid Tron contract call data"));

	*p_token = get_tron_token_by_address(&contract.contract_address.bytes[1]);
	if (!*p_token)
		return errmsg(msg, E_TRON_UnsupportedToken, _("Unsupported token"));

	// parse chunk data as ERC20 transfer
	static uint8_t TRANSFER_SIG[4] = {0xa9, 0x05, 0x9c, 0xbb};
	// check method sig
	if (memcmp(TRANSFER_SIG, contract.data.bytes, 4))
		return errmsg(msg, E_TRON_InvalidMethodSignature, _("Not TRC20 transfer method signature"));
	
	if (contract.data.size != 68)
		return errmsg(msg, E_TRON_InvalidCallData, _("Invalid TRC20 transfer method arguments data size"));

	if (tron_eth_2_trx_address(&contract.data.bytes[4 + 12], to_str, to_str_len) < 34)
		return errmsg(msg, E_TRON_EncodeTronAddress, _("Failed to encode to Tron address"));

	memcpy(value_bytes, &contract.data.bytes[4 + 32], 32);

	return 0;
}

bool tron_sign_raw_tx(const uint8_t *raw_data, int raw_data_size, const HDNode *node, TronSignature *resp)
{
	// decode raw_tx
	protocol_Transaction_raw tx;
	pb_istream_t stream = pb_istream_from_buffer(raw_data, raw_data_size);
	if (!pb_decode(&stream, protocol_Transaction_raw_fields, &tx)) {
		fsm_sendFailure(FailureType_Failure_DataError, "failed to decode tx data");
		return false;
	}

	char to_str[36];
	ConstTronTokenPtr token = NULL;
	uint64_t amount = 0;
	uint8_t value_bytes[32];

	if (tx.contract_count != 1) {
		fsm_sendFailure(FailureType_Failure_DataError, "contract array size is not 1");
		return false;
	}
	MSG msg;
	switch (tx.contract[0].type)
	{
	case protocol_Transaction_Contract_ContractType_TransferContract:
		if (strcmp(tx.contract[0].parameter.type_url, "type.googleapis.com/protocol.TransferContract"))
		{
			fsm_sendFailure(FailureType_Failure_DataError, "contract type_url mismatch");
			return false;
		}
		if (decode_transfer_contract(&tx.contract[0].parameter.value, to_str, sizeof(to_str), &amount, msg))
		{
			fsm_sendFailure(FailureType_Failure_DataError, msg);
			return false;
		}
		break;
	case protocol_Transaction_Contract_ContractType_TriggerSmartContract:
		if (strcmp(tx.contract[0].parameter.type_url, "type.googleapis.com/protocol.TriggerSmartContract"))
		{
			fsm_sendFailure(FailureType_Failure_DataError, "contract type_url mismatch");
			return false;
		}
		if (decode_trc20_contract(&tx.contract[0].parameter.value, to_str, sizeof(to_str), value_bytes, &token, msg))
		{
			fsm_sendFailure(FailureType_Failure_DataError, msg);
			return false;
		}
		break;
	case protocol_Transaction_Contract_ContractType_TransferAssetContract:
	default:
		fsm_sendFailure(FailureType_Failure_DataError, "unsupported contract type");
		return false;
	}

	// parse fee
	uint64_t fee = tx.fee_limit;

	// display tx info and ask user to confirm
	layoutTronConfirmTx(to_str, amount, value_bytes, token);
	if (!protectButton(ButtonRequestType_ButtonRequest_SignTx, false)) {
		fsm_sendFailure(FailureType_Failure_ActionCancelled, NULL);
		// TODO ethereum_signing_abort();
		return false;
	}

	layoutTronFee(amount, value_bytes, token, fee);
	if (!protectButton(ButtonRequestType_ButtonRequest_SignTx, false)) {
		fsm_sendFailure(FailureType_Failure_ActionCancelled, NULL);
		// TODO ethereum_signing_abort();
		return false;
	}

	// hash the tx
	uint8_t hash[32];
	SHA256_CTX ctx;
	sha256_Init(&ctx);
	sha256_Update(&ctx, raw_data, raw_data_size);
	sha256_Final(&ctx, hash);

	// sign tx hash
	uint8_t v;
	if (ecdsa_sign_digest_DER(&secp256k1, node->private_key, hash, resp->signature.bytes, &v, ethereum_is_canonic) != 0) {
		fsm_sendFailure(FailureType_Failure_ProcessError, _("Signing failed"));
		return false;
	}

	// fill response
	resp->signature.bytes[64] = 27 + v;
	resp->signature.size = 65;

	return true;
}

void tron_format_amount(const uint64_t amount, char *buf, int buflen) {
	uint64_t integer_part = amount / 1000000;
	unsigned fractional_part = amount % 1000000;
	snprintf(buf, buflen, "%llu.%.6u TRX", integer_part, fractional_part);
}

void tron_format_token_amount(const bignum256 *amnt, ConstTronTokenPtr token, char *buf, int buflen) {
	if (token == NULL) {
		strlcpy(buf, "Unknown token value", buflen);
		return;
	}
	bn_format(amnt, NULL, token->ticker, token->decimals, 0, false, buf, buflen);
}
