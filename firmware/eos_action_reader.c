#include <string.h>
#include "eos_action_reader.h"

void action_reader_init(EosReaderCTX *_ctx, uint8_t * buf, int len) 
{
    reader_init(_ctx, buf, len);
}

uint64_t action_reader_count(EosReaderCTX *_ctx)
{
    uint64_t count;
    if (reader_get_variable_uint(_ctx, &count)) {
        return count;
    }
    return 0;
}

bool action_reader_next(EosReaderCTX *_ctx, EosAction *action) 
{
    if (!reader_get_long(_ctx, &action->account)) {
        return false;
    }
    if (!reader_get_long(_ctx, &action->name)) {
        return false;
    }
    if (!reader_get_variable_uint(_ctx, &(action->authorization_size))) {
        return false;
    }
    if (action->authorization_size > 4) {
        return false;
    }
    for (uint8_t i = 0; i < action->authorization_size && i < 4; i++)
    {
        if (!reader_get_long(_ctx, &(action->authorization[i].actor))) {
            return false;
        }
        if (!reader_get_long(_ctx, &(action->authorization[i].permission))) {
            return false;
        }
    }
    if (!reader_get_variable_uint(_ctx, &(action->data_size))) {
        return false;
    }
    if (action->data_size > 512) {
        return false;
    }
    // if (action->data_size > 0) {
    //     if (!reader_get_bytes(_ctx, action->data, action->data_size)) {
    //         return FAILED;
    //     }
    // }
    return true;
}