#ifndef _EOS_DATA_READER_H_
#define _EOS_DATA_READER_H_
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "eos_model.h"
#include "eos_reader.h"

void action_reader_init(EosReaderCTX *, uint8_t *, int);

uint64_t action_reader_count(EosReaderCTX *_ctx);

bool action_reader_next(EosReaderCTX *_ctx, EosAction *);
#endif