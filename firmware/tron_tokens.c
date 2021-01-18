#include <string.h>
#include "tron_tokens.h"

#define TRON_TOKENS_COUNT 1
const TronTokenType tron_tokens[TRON_TOKENS_COUNT] = {
    {"\xA6\x14\xF8\x03\xB6\xFD\x78\x09\x86\xA4\x2C\x78\xEC\x9C\x7F\x77\xE6\xDE\xD1\x3C", " USDT", 6},
};

ConstTronTokenPtr get_tron_token_by_address(const uint8_t *address) {
    for (int i = 0; i < TRON_TOKENS_COUNT; i++) {
        if (memcmp(address, tron_tokens[i].address, 20) != 0)
            continue;
        return &tron_tokens[i];
    }

    return NULL;
}
