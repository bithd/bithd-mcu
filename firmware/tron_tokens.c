#include <string.h>
#include "tron_tokens.h"

#define TRON_TOKENS_COUNT 6
const TronTokenType tron_tokens[TRON_TOKENS_COUNT] = {
    {"\xA6\x14\xF8\x03\xB6\xFD\x78\x09\x86\xA4\x2C\x78\xEC\x9C\x7F\x77\xE6\xDE\xD1\x3C", " USDT", 6},
    {"\x89\x1C\xDB\x91\xD1\x49\xF2\x3B\x1A\x45\xD9\xC5\xCA\x78\xA8\x8D\x0C\xB4\x4C\x18", " WTRX", 6},
    {"\x6A\x63\x37\xAE\x47\xA0\x9A\xEA\x0B\xBD\x4F\xAE\xB2\x3C\xA9\x43\x49\xC7\xB7\x74", " WBTT", 6},
    {"\x18\xFD\x06\x26\xDA\xF3\xAF\x02\x38\x9A\xEF\x3E\xD8\x7D\xB9\xC3\x3F\x63\x8F\xFA", " JST", 18},
    {"\x74\x47\x2E\x7D\x35\x39\x5A\x6B\x5A\xDD\x42\x7E\xEC\xB7\xF4\xB6\x2A\xD2\xB0\x71", " WIN", 6},
    {"\x6B\x51\x51\x32\x03\x59\xEC\x18\xB0\x86\x07\xC7\x0A\x3B\x74\x39\xAF\x62\x6A\xA3", " SUN", 18},
};

ConstTronTokenPtr get_tron_token_by_address(const uint8_t *address) {
    for (int i = 0; i < TRON_TOKENS_COUNT; i++) {
        if (memcmp(address, tron_tokens[i].address, 20) != 0)
            continue;
        return &tron_tokens[i];
    }

    return NULL;
}
