#ifndef _EOS_READER_H_
#define _EOS_READER_H_ 
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

typedef struct eos_reader_ctx 
{
    uint8_t *_buf;
    int _index;
    int _length;
} EosReaderCTX;

void reader_init(EosReaderCTX*, uint8_t *, int);

bool reader_get(EosReaderCTX*, uint8_t *);

bool reader_get_short(EosReaderCTX*, uint16_t *);

bool reader_get_int(EosReaderCTX*, uint32_t *);

bool reader_get_long(EosReaderCTX*, uint64_t *);

bool reader_get_bytes(EosReaderCTX*, uint8_t *, size_t);

bool reader_get_variable_uint(EosReaderCTX*, uint64_t *);

#endif