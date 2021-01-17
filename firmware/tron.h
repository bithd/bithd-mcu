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

#ifndef __TRON_H__
#define __TRON_H__

#include <stdint.h>
#include <stdbool.h>
#include "bip32.h"
#include "messages.pb.h"

void tron_message_sign(TronSignMessage *msg, const HDNode *node, TronMessageSignature *resp);
int tron_eth_2_trx_address(const uint8_t *eth_address, char *str, int strsize);
void tron_sign_raw_tx(const uint8_t *raw_tx, int raw_tx_size, const HDNode *node, TronSignature *resp);

typedef struct {
    uint8_t address[20];    // according eth address
    char    ticker[10];     // token name
    int     decimals;       // token decimals
} TronTokenType;

void tron_format_amount(const uint64_t amount, char *buf, int buflen);
void tron_format_token_amount(const bignum256 *amnt, TronTokenType *token, char *buf, int buflen);

#endif  // __TRON_H__
