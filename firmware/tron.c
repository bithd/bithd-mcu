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
#include "sha3.h"
#include "address.h"
#include "util.h"
#include "gettext.h"
#include "storage.h"
#include "base58.h"

#include <pb_decode.h>
#include "tron.pb.h"

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

	int r = base58_encode_check(address_bytes, sizeof(address_bytes), HASHER_SHA2, str, strsize);
	return r;
}

extern void layoutTronConfirmTx(const uint8_t *to, uint32_t to_len, const uint8_t *value, uint32_t value_len, TronTokenType *token);

void tron_sign_raw_tx(const uint8_t *raw_tx, int raw_tx_size, const HDNode *node, TronSignature *resp) {
	// decode raw_tx
	protocol_Transaction tx;
	pb_istream_t stream = pb_istream_from_buffer(raw_tx, raw_tx_size);
	pb_decode(&stream, protocol_Transaction_fields, &tx);

	// display tx info

	// ask user to confirm

	// sign the tx
	uint8_t hash[32];
	struct SHA3_CTX ctx;
	sha3_256_Init(&ctx);
	sha3_Update(&ctx, raw_tx, raw_tx_size);
	keccak_Final(&ctx, hash);

	uint8_t v;
	if (ecdsa_sign_digest_DER(&secp256k1, node->private_key, hash, resp->signature.bytes, &v, ethereum_is_canonic) != 0) {
		fsm_sendFailure(FailureType_Failure_ProcessError, _("Signing failed"));
		return;
	}

	// fill response
	resp->signature.size = 65;
}
