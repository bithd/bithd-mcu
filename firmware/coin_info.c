// This file is automatically generated by coin_info.py -- DO NOT EDIT!

#include "coins.h"

#include "curves.h"
#include "secp256k1.h"

const CoinInfo coins[COINS_COUNT] = {
{
	.coin_name = "Bitcoin",
	.coin_shortcut = " BTC",
	.maxfee_kb = 2000000,
	.signed_message_header = "\x18" "Bitcoin Signed Message:\n",
	.has_address_type = true,
	.has_address_type_p2sh = true,
	.has_segwit = true,
	.has_fork_id = false,
	.force_bip143 = false,
	.decred = false,
	.address_type = 0,
	.address_type_p2sh = 5,
	.xpub_magic = 0x0488b21e,
	.fork_id = 0,
	.bech32_prefix = "bc",
	.cashaddr_prefix = NULL,
	.coin_type = (0 | 0x80000000),
	.curve_name = SECP256K1_NAME,
	.curve = &secp256k1_info,
},
{
	.coin_name = "Testnet",
	.coin_shortcut = " TEST",
	.maxfee_kb = 10000000,
	.signed_message_header = "\x18" "Bitcoin Signed Message:\n",
	.has_address_type = true,
	.has_address_type_p2sh = true,
	.has_segwit = true,
	.has_fork_id = false,
	.force_bip143 = false,
	.decred = false,
	.address_type = 111,
	.address_type_p2sh = 196,
	.xpub_magic = 0x043587cf,
	.fork_id = 0,
	.bech32_prefix = "tb",
	.cashaddr_prefix = NULL,
	.coin_type = (1 | 0x80000000),
	.curve_name = SECP256K1_NAME,
	.curve = &secp256k1_info,
},
{
	.coin_name = "Bcash",
	.coin_shortcut = " BCH",
	.maxfee_kb = 500000,
	.signed_message_header = "\x18" "Bitcoin Signed Message:\n",
	.has_address_type = true,
	.has_address_type_p2sh = true,
	.has_segwit = false,
	.has_fork_id = true,
	.force_bip143 = true,
	.decred = false,
	.address_type = 0,
	.address_type_p2sh = 5,
	.xpub_magic = 0x0488b21e,
	.fork_id = 0,
	.bech32_prefix = NULL,
	.cashaddr_prefix = "bitcoincash",
	.coin_type = (145 | 0x80000000),
	.curve_name = SECP256K1_NAME,
	.curve = &secp256k1_info,
},
{
	.coin_name = "Bcash Testnet",
	.coin_shortcut = " TBCH",
	.maxfee_kb = 10000000,
	.signed_message_header = "\x18" "Bitcoin Signed Message:\n",
	.has_address_type = true,
	.has_address_type_p2sh = true,
	.has_segwit = false,
	.has_fork_id = true,
	.force_bip143 = true,
	.decred = false,
	.address_type = 111,
	.address_type_p2sh = 196,
	.xpub_magic = 0x043587cf,
	.fork_id = 0,
	.bech32_prefix = NULL,
	.cashaddr_prefix = "bchtest",
	.coin_type = (1 | 0x80000000),
	.curve_name = SECP256K1_NAME,
	.curve = &secp256k1_info,
},
{
	.coin_name = "BcashSv",
	.coin_shortcut = " BSV",
	.maxfee_kb = 500000,
	.signed_message_header = "\x18" "Bitcoin Signed Message:\n",
	.has_address_type = true,
	.has_address_type_p2sh = true,
	.has_segwit = false,
	.has_fork_id = true,
	.force_bip143 = true,
	.decred = false,
	.address_type = 0,
	.address_type_p2sh = 5,
	.xpub_magic = 0x0488b21e,
	.fork_id = 0,
	.bech32_prefix = NULL,
	.cashaddr_prefix = "bitcoincash",
	.coin_type = (236 | 0x80000000),
	.curve_name = SECP256K1_NAME,
	.curve = &secp256k1_info,

},
{
	.coin_name = "Bgold",
	.coin_shortcut = " BTG",
	.maxfee_kb = 500000,
	.signed_message_header = "\x1d" "Bitcoin Gold Signed Message:\n",
	.has_address_type = true,
	.has_address_type_p2sh = true,
	.has_segwit = true,
	.has_fork_id = true,
	.force_bip143 = true,
	.decred = false,
	.address_type = 38,
	.address_type_p2sh = 23,
	.xpub_magic = 0x0488b21e,
	.fork_id = 79,
	.bech32_prefix = "btg",
	.cashaddr_prefix = NULL,
	.coin_type = (156 | 0x80000000),
	.curve_name = SECP256K1_NAME,
	.curve = &secp256k1_info,
},
{
	.coin_name = "Bprivate",
	.coin_shortcut = " BTCP",
	.maxfee_kb = 1000000,
	.signed_message_header = "\x1f" "BitcoinPrivate Signed Message:\n",
	.has_address_type = true,
	.has_address_type_p2sh = true,
	.has_segwit = false,
	.has_fork_id = true,
	.force_bip143 = false,
	.decred = false,
	.address_type = 4901,
	.address_type_p2sh = 5039,
	.xpub_magic = 0x0488b21e,
	.fork_id = 42,
	.bech32_prefix = NULL,
	.cashaddr_prefix = NULL,
	.coin_type = (183 | 0x80000000),
	.curve_name = SECP256K1_NAME,
	.curve = &secp256k1_info,
},
{
	.coin_name = "Dash",
	.coin_shortcut = " DASH",
	.maxfee_kb = 100000,
	.signed_message_header = "\x19" "DarkCoin Signed Message:\n",
	.has_address_type = true,
	.has_address_type_p2sh = true,
	.has_segwit = false,
	.has_fork_id = false,
	.force_bip143 = false,
	.decred = false,
	.address_type = 76,
	.address_type_p2sh = 16,
	.xpub_magic = 0x02fe52cc,
	.fork_id = 0,
	.bech32_prefix = NULL,
	.cashaddr_prefix = NULL,
	.coin_type = (5 | 0x80000000),
	.curve_name = SECP256K1_NAME,
	.curve = &secp256k1_info,
},
{
	.coin_name = "Dash Testnet",
	.coin_shortcut = " tDASH",
	.maxfee_kb = 100000,
	.signed_message_header = "\x19" "DarkCoin Signed Message:\n",
	.has_address_type = true,
	.has_address_type_p2sh = true,
	.has_segwit = false,
	.has_fork_id = false,
	.force_bip143 = false,
	.decred = false,
	.address_type = 140,
	.address_type_p2sh = 19,
	.xpub_magic = 0x043587cf,
	.fork_id = 0,
	.bech32_prefix = NULL,
	.cashaddr_prefix = NULL,
	.coin_type = (1 | 0x80000000),
	.curve_name = SECP256K1_NAME,
	.curve = &secp256k1_info,
},
{
	.coin_name = "Decred",
	.coin_shortcut = " DCR",
	.maxfee_kb = 1000000,
	.signed_message_header = "\x17" "Decred Signed Message:\n",
	.has_address_type = true,
	.has_address_type_p2sh = true,
	.has_segwit = false,
	.has_fork_id = false,
	.force_bip143 = false,
	.decred = true,
	.address_type = 1855,
	.address_type_p2sh = 1818,
	.xpub_magic = 0x02fda926,
	.fork_id = 0,
	.bech32_prefix = NULL,
	.cashaddr_prefix = NULL,
	.coin_type = (42 | 0x80000000),
	.curve_name = SECP256K1_DECRED_NAME,
	.curve = &secp256k1_decred_info,
},
{
	.coin_name = "Decred Testnet",
	.coin_shortcut = " TDCR",
	.maxfee_kb = 10000000,
	.signed_message_header = "\x17" "Decred Signed Message:\n",
	.has_address_type = true,
	.has_address_type_p2sh = true,
	.has_segwit = false,
	.has_fork_id = false,
	.force_bip143 = false,
	.decred = true,
	.address_type = 3873,
	.address_type_p2sh = 3836,
	.xpub_magic = 0x043587d1,
	.fork_id = 0,
	.bech32_prefix = NULL,
	.cashaddr_prefix = NULL,
	.coin_type = (1 | 0x80000000),
	.curve_name = SECP256K1_DECRED_NAME,
	.curve = &secp256k1_decred_info,
},
{
	.coin_name = "DigiByte",
	.coin_shortcut = " DGB",
	.maxfee_kb = 500000,
	.signed_message_header = "\x19" "DigiByte Signed Message:\n",
	.has_address_type = true,
	.has_address_type_p2sh = true,
	.has_segwit = true,
	.has_fork_id = false,
	.force_bip143 = false,
	.decred = false,
	.address_type = 30,
	.address_type_p2sh = 63,
	.xpub_magic = 0x0488b21e,
	.fork_id = 0,
	.bech32_prefix = "dgb",
	.cashaddr_prefix = NULL,
	.coin_type = (20 | 0x80000000),
	.curve_name = SECP256K1_NAME,
	.curve = &secp256k1_info,
},
{
	.coin_name = "Dogecoin",
	.coin_shortcut = " DOGE",
	.maxfee_kb = 1000000000,
	.signed_message_header = "\x19" "Dogecoin Signed Message:\n",
	.has_address_type = true,
	.has_address_type_p2sh = true,
	.has_segwit = false,
	.has_fork_id = false,
	.force_bip143 = false,
	.decred = false,
	.address_type = 30,
	.address_type_p2sh = 22,
	.xpub_magic = 0x02facafd,
	.fork_id = 0,
	.bech32_prefix = NULL,
	.cashaddr_prefix = NULL,
	.coin_type = (3 | 0x80000000),
	.curve_name = SECP256K1_NAME,
	.curve = &secp256k1_info,
},
{
	.coin_name = "Fujicoin",
	.coin_shortcut = " FJC",
	.maxfee_kb = 10000000,
	.signed_message_header = "\x19" "FujiCoin Signed Message:\n",
	.has_address_type = true,
	.has_address_type_p2sh = true,
	.has_segwit = true,
	.has_fork_id = false,
	.force_bip143 = false,
	.decred = false,
	.address_type = 36,
	.address_type_p2sh = 16,
	.xpub_magic = 0x0488b21e,
	.fork_id = 0,
	.bech32_prefix = "fc",
	.cashaddr_prefix = NULL,
	.coin_type = (75 | 0x80000000),
	.curve_name = SECP256K1_NAME,
	.curve = &secp256k1_info,
},
{
	.coin_name = "Groestlcoin",
	.coin_shortcut = " GRS",
	.maxfee_kb = 100000,
	.signed_message_header = "\x1c" "GroestlCoin Signed Message:\n",
	.has_address_type = true,
	.has_address_type_p2sh = true,
	.has_segwit = true,
	.has_fork_id = false,
	.force_bip143 = false,
	.decred = false,
	.address_type = 36,
	.address_type_p2sh = 5,
	.xpub_magic = 0x0488b21e,
	.fork_id = 0,
	.bech32_prefix = "grs",
	.cashaddr_prefix = NULL,
	.coin_type = (17 | 0x80000000),
	.curve_name = SECP256K1_GROESTL_NAME,
	.curve = &secp256k1_groestl_info,
},
{
	.coin_name = "Groestlcoin Testnet",
	.coin_shortcut = " tGRS",
	.maxfee_kb = 100000,
	.signed_message_header = "\x1c" "GroestlCoin Signed Message:\n",
	.has_address_type = true,
	.has_address_type_p2sh = true,
	.has_segwit = true,
	.has_fork_id = false,
	.force_bip143 = false,
	.decred = false,
	.address_type = 111,
	.address_type_p2sh = 196,
	.xpub_magic = 0x043587cf,
	.fork_id = 0,
	.bech32_prefix = "tgrs",
	.cashaddr_prefix = NULL,
	.coin_type = (1 | 0x80000000),
	.curve_name = SECP256K1_GROESTL_NAME,
	.curve = &secp256k1_groestl_info,
},
{
	.coin_name = "Litecoin",
	.coin_shortcut = " LTC",
	.maxfee_kb = 40000000,
	.signed_message_header = "\x19" "Litecoin Signed Message:\n",
	.has_address_type = true,
	.has_address_type_p2sh = true,
	.has_segwit = true,
	.has_fork_id = false,
	.force_bip143 = false,
	.decred = false,
	.address_type = 48,
	.address_type_p2sh = 50,
	.xpub_magic = 0x019da462,
	.fork_id = 0,
	.bech32_prefix = "ltc",
	.cashaddr_prefix = NULL,
	.coin_type = (2 | 0x80000000),
	.curve_name = SECP256K1_NAME,
	.curve = &secp256k1_info,
},
{
	.coin_name = "Litecoin Testnet",
	.coin_shortcut = " TLTC",
	.maxfee_kb = 40000000,
	.signed_message_header = "\x19" "Litecoin Signed Message:\n",
	.has_address_type = true,
	.has_address_type_p2sh = true,
	.has_segwit = true,
	.has_fork_id = false,
	.force_bip143 = false,
	.decred = false,
	.address_type = 111,
	.address_type_p2sh = 58,
	.xpub_magic = 0x043587cf,
	.fork_id = 0,
	.bech32_prefix = "tltc",
	.cashaddr_prefix = NULL,
	.coin_type = (1 | 0x80000000),
	.curve_name = SECP256K1_NAME,
	.curve = &secp256k1_info,
},
{
	.coin_name = "Monacoin",
	.coin_shortcut = " MONA",
	.maxfee_kb = 5000000,
	.signed_message_header = "\x19" "Monacoin Signed Message:\n",
	.has_address_type = true,
	.has_address_type_p2sh = true,
	.has_segwit = true,
	.has_fork_id = false,
	.force_bip143 = false,
	.decred = false,
	.address_type = 50,
	.address_type_p2sh = 55,
	.xpub_magic = 0x0488b21e,
	.fork_id = 0,
	.bech32_prefix = "mona",
	.cashaddr_prefix = NULL,
	.coin_type = (22 | 0x80000000),
	.curve_name = SECP256K1_NAME,
	.curve = &secp256k1_info,
},
{
	.coin_name = "Namecoin",
	.coin_shortcut = " NMC",
	.maxfee_kb = 10000000,
	.signed_message_header = "\x19" "Namecoin Signed Message:\n",
	.has_address_type = true,
	.has_address_type_p2sh = true,
	.has_segwit = false,
	.has_fork_id = false,
	.force_bip143 = false,
	.decred = false,
	.address_type = 52,
	.address_type_p2sh = 5,
	.xpub_magic = 0x019da462,
	.fork_id = 0,
	.bech32_prefix = NULL,
	.cashaddr_prefix = NULL,
	.coin_type = (7 | 0x80000000),
	.curve_name = SECP256K1_NAME,
	.curve = &secp256k1_info,
},
{
	.coin_name = "Vertcoin",
	.coin_shortcut = " VTC",
	.maxfee_kb = 40000000,
	.signed_message_header = "\x19" "Vertcoin Signed Message:\n",
	.has_address_type = true,
	.has_address_type_p2sh = true,
	.has_segwit = true,
	.has_fork_id = false,
	.force_bip143 = false,
	.decred = false,
	.address_type = 71,
	.address_type_p2sh = 5,
	.xpub_magic = 0x0488b21e,
	.fork_id = 0,
	.bech32_prefix = "vtc",
	.cashaddr_prefix = NULL,
	.coin_type = (28 | 0x80000000),
	.curve_name = SECP256K1_NAME,
	.curve = &secp256k1_info,
},
{
	.coin_name = "Viacoin",
	.coin_shortcut = " VIA",
	.maxfee_kb = 40000000,
	.signed_message_header = "\x18" "Viacoin Signed Message:\n",
	.has_address_type = true,
	.has_address_type_p2sh = true,
	.has_segwit = true,
	.has_fork_id = false,
	.force_bip143 = false,
	.decred = false,
	.address_type = 71,
	.address_type_p2sh = 33,
	.xpub_magic = 0x0488b21e,
	.fork_id = 0,
	.bech32_prefix = "via",
	.cashaddr_prefix = NULL,
	.coin_type = (14 | 0x80000000),
	.curve_name = SECP256K1_NAME,
	.curve = &secp256k1_info,
},
{
	.coin_name = "Zcash",
	.coin_shortcut = " ZEC",
	.maxfee_kb = 1000000,
	.signed_message_header = "\x16" "Zcash Signed Message:\n",
	.has_address_type = true,
	.has_address_type_p2sh = true,
	.has_segwit = false,
	.has_fork_id = false,
	.force_bip143 = false,
	.decred = false,
	.address_type = 7352,
	.address_type_p2sh = 7357,
	.xpub_magic = 0x0488b21e,
	.fork_id = 0,
	.bech32_prefix = NULL,
	.cashaddr_prefix = NULL,
	.coin_type = (133 | 0x80000000),
	.curve_name = SECP256K1_NAME,
	.curve = &secp256k1_info,
},
{
	.coin_name = "Ycash",
	.coin_shortcut = " YEC",
	.maxfee_kb = 1000000,
	.signed_message_header = "\x16" "Ycash Signed Message:\n",
	.has_address_type = true,
	.has_address_type_p2sh = true,
	.has_segwit = false,
	.has_fork_id = false,
	.force_bip143 = false,
	.decred = false,
	.address_type = 7208,
	.address_type_p2sh = 7213,
	.xpub_magic = 0x0488b21e,
	.fork_id = 0,
	.bech32_prefix = NULL,
	.cashaddr_prefix = NULL,
	.coin_type = (347 | 0x80000000),
	.curve_name = SECP256K1_NAME,
	.curve = &secp256k1_info,
},
{
	.coin_name = "Zcash Testnet",
	.coin_shortcut = " TAZ",
	.maxfee_kb = 10000000,
	.signed_message_header = "\x16" "Zcash Signed Message:\n",
	.has_address_type = true,
	.has_address_type_p2sh = true,
	.has_segwit = false,
	.has_fork_id = false,
	.force_bip143 = false,
	.decred = false,
	.address_type = 7461,
	.address_type_p2sh = 7354,
	.xpub_magic = 0x043587cf,
	.fork_id = 0,
	.bech32_prefix = NULL,
	.cashaddr_prefix = NULL,
	.coin_type = (1 | 0x80000000),
	.curve_name = SECP256K1_NAME,
	.curve = &secp256k1_info,
},
{
	.coin_name = "Zcoin",
	.coin_shortcut = " XZC",
	.maxfee_kb = 1000000,
	.signed_message_header = "\x16" "Zcoin Signed Message:\n",
	.has_address_type = true,
	.has_address_type_p2sh = true,
	.has_segwit = false,
	.has_fork_id = false,
	.force_bip143 = false,
	.decred = false,
	.address_type = 82,
	.address_type_p2sh = 7,
	.xpub_magic = 0x0488b21e,
	.fork_id = 0,
	.bech32_prefix = NULL,
	.cashaddr_prefix = NULL,
	.coin_type = (136 | 0x80000000),
	.curve_name = SECP256K1_NAME,
	.curve = &secp256k1_info,
},
{
	.coin_name = "Zcoin Testnet",
	.coin_shortcut = " tXZC",
	.maxfee_kb = 1000000,
	.signed_message_header = "\x16" "Zcoin Signed Message:\n",
	.has_address_type = true,
	.has_address_type_p2sh = true,
	.has_segwit = false,
	.has_fork_id = false,
	.force_bip143 = false,
	.decred = false,
	.address_type = 65,
	.address_type_p2sh = 178,
	.xpub_magic = 0x043587cf,
	.fork_id = 0,
	.bech32_prefix = NULL,
	.cashaddr_prefix = NULL,
	.coin_type = (1 | 0x80000000),
	.curve_name = SECP256K1_NAME,
	.curve = &secp256k1_info,
},
};