#include <stdio.h>
#include <string.h>
#include "eos_utils.h"
#include "ripemd160.h"
#include "base58.h"

char EOS_CHAR_MAP[] = ".12345abcdefghijklmnopqrstuvwxyz";

int _len_expcet_tail_dot(char *src, int len)
{
	int i;
	for (i = 0; i <= len; i++)
	{
		if (src[i] == '.')
			continue;
		else
			break;
	}
	if(i >= len) return i;
	len = len - i;
	memmove(src, &src[i], len);
	for (i = 0; i <= len; i++)
	{
		if (src[i] != '.')
			continue;
		else
			break;
	}
	return i;
}

void _delete_tail_dot(char *src, int len, char *des, int *des_len)
{
	*des_len = _len_expcet_tail_dot(src, len);
	memcpy(des, src, *des_len);
	des[*des_len] = 0;
}

int name_to_str(uint64_t name, char* name_str)
{
	char str_tmp[MAX_NAME_IDX + 1];
	uint64_t name_tmp = name;
	for (int i = 0; i <= MAX_NAME_IDX; i++)
	{
		str_tmp[MAX_NAME_IDX-i] = EOS_CHAR_MAP[(int)(name_tmp & (i == 0 ? 0x0f : 0x1f))];
		name_tmp >>= (i == 0 ? 4 : 5);
	}
	int name_len;
	_delete_tail_dot(str_tmp, MAX_NAME_IDX + 1, name_str, &name_len);
	return name_len;
}

void symbol_to_str(uint64_t symbol, char *symbol_str) 
{
	uint64_t value = symbol;
	int i = 0;
	while(value > 0) {
		symbol_str[i] = value & 0xff;
		value >>= 8;
		i ++;
	}
	*(symbol_str + i) = '\0';
}

uint8_t format_asset(EosTypeAsset *asset, char *out) 
{
    int decimals = asset->symbol & 0xFF;
    double value = asset->amount * 1.0;
    for (int i = 0; i < decimals; i ++)
        value /= 10;
    char sym[8];
    symbol_to_str(asset->symbol, sym);
	char value_str[20];
    uint8_t len = sprintf(value_str, "%.4f", value);
	while((value_str[len-1]=='0') || (value_str[len-1]=='.')) {
		value_str[len-1] = 0;
		len--;
	}
	return sprintf(out, "%s %s", value_str, sym);
}

uint8_t format_producer(uint64_t name, int no, char *out)
{
	char prod[13];
	int _size = name_to_str(name, prod);
	prod[_size] = '\0';
	return sprintf(out, "#%02d: %s", no, prod);
}

uint8_t format_eos_pubkey(uint8_t *pubkey, uint8_t len, int no, char *out) 
{
	uint8_t hash[20];
	ripemd160(pubkey, len, hash);
	uint8_t addy[len + 4];
	memcpy(addy, pubkey, len);
	memcpy(addy + len, hash, 4);
	char b58addy[60];
	size_t b58sz = 60;
	b58enc(b58addy, &b58sz, addy, len + 4);
	return sprintf(out, "#%02d: EOS%s", no, b58addy);
}