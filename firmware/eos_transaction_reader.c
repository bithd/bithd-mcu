#include "eos_transaction_reader.h"
#include "eos_action_reader.h"

void transcation_reader_init(EosReaderCTX *_ctx, uint8_t *buf, int len)
{
    reader_init(_ctx, buf, len);
}

bool transaction_reader_get(EosReaderCTX *_ctx, EosTransaction *trx)
{
    if (!reader_get_int(_ctx, &trx->expiration)) {
        return false;
    }
    if (!reader_get_short(_ctx, &trx->ref_block_num)) {
        return false;
    }
    if (!reader_get_int(_ctx, &trx->ref_block_prefix)) {
        return false;
    }
    if (!reader_get_variable_uint(_ctx, &trx->max_net_usage_words)) {
        return false;
    }
    if (!reader_get_variable_uint(_ctx, &trx->max_cpu_usage_ms)) {
        return false;
    }
    if (!reader_get_variable_uint(_ctx, &trx->delay_sec)) {
        return false;
    }
    if (!reader_get_variable_uint(_ctx, &trx->contract_free_actions_size)) {
        return false;
    }
    if (trx->contract_free_actions_size > 1) {
        return false;
    }
    for (uint8_t i = 0; i < trx->contract_free_actions_size; i++) {
        if (!action_reader_next(_ctx, trx->contract_free_actions + i)) {
            return false;
        }
    }
    
    return true;
}