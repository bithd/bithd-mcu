#include "eos_action_data_reader.h"
#include "eos_transaction_reader.h"

void action_data_reader_init(EosReaderCTX *_ctx, uint8_t *buf, int len)
{
    reader_init(_ctx, buf, len);
}

int reader_get_buyram(EosReaderCTX *_ctx, EosioBuyram *buyram) 
{
    if (!reader_get_long(_ctx, &buyram->from)) {
        return FAILED;
    }
    if (!reader_get_long(_ctx, &buyram->receiver)) {
        return FAILED;
    }
    if (!reader_get_long(_ctx, &buyram->quantity.amount)) {
        return FAILED;
    }
    if (!reader_get_long(_ctx, &buyram->quantity.symbol)) {
        return FAILED;
    }
    return SUCCESS;
}

int reader_get_buyram_bytes(EosReaderCTX *_ctx, EosioBuyramBytes *buyram_bytes) 
{
    if (!reader_get_long(_ctx, &buyram_bytes->from)) {
        return FAILED;
    }
    if (!reader_get_long(_ctx, &buyram_bytes->receiver)) {
        return FAILED;
    }
    if (!reader_get_int(_ctx, &buyram_bytes->bytes)) {
        return FAILED;
    }
    return SUCCESS;
}

int reader_get_sellram(EosReaderCTX *_ctx, EosioSellram *sellram)
{
    if (!reader_get_long(_ctx, &sellram->from)) {
        return FAILED;
    }
    if (!reader_get_long(_ctx, &sellram->bytes)) {
        return FAILED;
    }
    return SUCCESS;
}

int reader_get_delegage(EosReaderCTX *_ctx, EosioDelegate *delegate) 
{
    if (!reader_get_long(_ctx, &delegate->from)) {
        return FAILED;
    }
    if (!reader_get_long(_ctx, &delegate->receiver)) {
        return FAILED;
    }
    if (!reader_get_long(_ctx, &delegate->net_quantity.amount)) {
        return FAILED;
    }
    if (!reader_get_long(_ctx, &delegate->net_quantity.symbol)) {
        return FAILED;
    }
    if (!reader_get_long(_ctx, &delegate->cpu_quantity.amount)) {
        return FAILED;
    }
    if (!reader_get_long(_ctx, &delegate->cpu_quantity.symbol)) {
        return FAILED;
    }
    if (!reader_get(_ctx, &delegate->tansfer)) {
        return FAILED;
    }
    return SUCCESS;    
}

int reader_get_undelegate(EosReaderCTX *_ctx, EosioUndelegate *undelegate)
{
   if (!reader_get_long(_ctx, &undelegate->from)) {
        return FAILED;
    }
    if (!reader_get_long(_ctx, &undelegate->receiver)) {
        return FAILED;
    }
    if (!reader_get_long(_ctx, &undelegate->net_quantity.amount)) {
        return FAILED;
    }
    if (!reader_get_long(_ctx, &undelegate->net_quantity.symbol)) {
        return FAILED;
    }
    if (!reader_get_long(_ctx, &undelegate->cpu_quantity.amount)) {
        return FAILED;
    }
    if (!reader_get_long(_ctx, &undelegate->cpu_quantity.symbol)) {
        return FAILED;
    }
    return SUCCESS; 
}

int reader_get_vote_producer(EosReaderCTX *_ctx, EosioVoteProducer *vote)
{
    if (!reader_get_long(_ctx, &vote->voter)) {
        return FAILED;
    }
    if (!reader_get_long(_ctx, &vote->proxy)) {
        return FAILED;
    }
    if (!reader_get_variable_uint(_ctx, &vote->producer_size)) {
        return FAILED;
    }
    if (vote->producer_size > VOTE_PRODUCER_MAX_COUNT) {
        return FAILED;
    }
    for (uint8_t i = 0; i < vote->producer_size; i++) {
        if (!reader_get_long(_ctx, vote->producers + i)) {
            return FAILED;
        }
    }
    return SUCCESS;
}

int reader_get_transfer(EosReaderCTX *_ctx, EosioTokenTransfer *transfer) 
{
    if (!reader_get_long(_ctx, &transfer->from)) {
        return FAILED;
    }
    if (!reader_get_long(_ctx, &transfer->to)) {
        return FAILED;
    }
    if (!reader_get_long(_ctx, &transfer->quantity.amount)) {
        return FAILED;
    }
    if (!reader_get_long(_ctx, &transfer->quantity.symbol)) {
        return FAILED;
    }
    if (!reader_get_variable_uint(_ctx, &transfer->memo_size)) {
        return FAILED;
    }
    if (transfer->memo_size > TRANSFER_MEMO_MAX_SIZE) {
        return FAILED;
    }
    if (!reader_get_bytes(_ctx, (uint8_t *) &transfer->memo, (size_t)transfer->memo_size)) {
        return FAILED;
    }
    return SUCCESS;
}

int reader_get_propose(EosReaderCTX *_ctx, EosioMsigPropose *propose) 
{
    if (!reader_get_long(_ctx, &propose->proposer)) {
        return FAILED;
    }
    if (!reader_get_long(_ctx, &propose->proposal_name)) {
        return FAILED;
    }
    if (!reader_get_variable_uint(_ctx, &propose->requested_size)) {
        return FAILED;
    }
    return SUCCESS;
}

int reader_get_approve(EosReaderCTX *_ctx, EosioMsigApprove *approve)
{
    if (!reader_get_long(_ctx, &approve->proposer)) {
        return FAILED;
    }
    if (!reader_get_long(_ctx, &approve->proposal_name)) {
        return FAILED;
    }
    if (!reader_get_long(_ctx, &approve->level.actor)) {
        return FAILED;
    }
    if (!reader_get_long(_ctx, &approve->level.permission)) {
        return FAILED;
    }
    return SUCCESS;
}

int reader_get_cancel(EosReaderCTX *_ctx, EosioMsigCancel *cancel) 
{
    if (!reader_get_long(_ctx, &cancel->proposer)) {
        return FAILED;
    }
    if (!reader_get_long(_ctx, &cancel->proposal_name)) {
        return FAILED;
    }
    if (!reader_get_long(_ctx, &cancel->canceler)) {
        return FAILED;
    }
    return SUCCESS;
}

int reader_get_exec(EosReaderCTX *_ctx, EosioMsigExec *exec)
{
    if (!reader_get_long(_ctx, &exec->proposer)) {
        return FAILED;
    }
    if (!reader_get_long(_ctx, &exec->proposal_name)) {
        return FAILED;
    }
    if (!reader_get_long(_ctx, &exec->executer)) {
        return FAILED;
    }
    return SUCCESS;
}

int reader_get_unapprove(EosReaderCTX *_ctx, EosioMsigUnapprove *unapprove)
{
    if (!reader_get_long(_ctx, &unapprove->proposer)) {
        return FAILED;
    }
    if (!reader_get_long(_ctx, &unapprove->proposal_name)) {
        return FAILED;
    }
    if (!reader_get_long(_ctx, &unapprove->level.actor)) {
        return FAILED;
    }
    if (!reader_get_long(_ctx, &unapprove->level.permission)) {
        return FAILED;
    }
    return SUCCESS;  
}

int reader_get_newaccount(EosReaderCTX *_ctx, EosioNewAccount *newaccount) 
{
    if (!reader_get_long(_ctx, &newaccount->creator)) {
        return FAILED;
    }
    if (!reader_get_long(_ctx, &newaccount->new_name)) {
        return FAILED;
    }
    if (!reader_get_authority(_ctx, &newaccount->owner)) {
        return FAILED;
    }
    if (!reader_get_authority(_ctx, &newaccount->active)) {
        return FAILED;
    }
    return SUCCESS;
}

int reader_get_refund(EosReaderCTX *_ctx, EosioRefund *refund)
{
    if (!reader_get_long(_ctx, &refund->owner)) {
        return FAILED;
    }
    return SUCCESS;
}

int reader_get_authority(EosReaderCTX *_ctx, EosAuthority *authority) 
{
    if (!reader_get_int(_ctx, &authority->threshold)) {
        return FAILED;
    }
    if (!reader_get_variable_uint(_ctx, &authority->key_size)) {
        return FAILED;
    }
    if (authority->key_size > 10) {
        return FAILED;
    }
    for (uint8_t i = 0; i < authority->key_size; i ++) {
        if (!reader_get_variable_uint(_ctx, &authority->keys[i].pubkey.curve_param_type)) {
            return FAILED;
        }
        if (!reader_get_bytes(_ctx, authority->keys[i].pubkey.pubkey, 33)) {
            return FAILED;
        }
        if (!reader_get_short(_ctx, &authority->keys[i].weight)) {
            return FAILED;
        }
    }
    if (!reader_get_variable_uint(_ctx, &authority->permission_size)) {
        return FAILED;
    }
    if (authority->permission_size > 10) {
        return FAILED;
    }
    for (uint8_t i = 0; i < authority->permission_size; i++) {
        if (!reader_get_long(_ctx, &authority->permissions[i].permission.actor)) {
            return FAILED;
        }
        if (!reader_get_long(_ctx, &authority->permissions[i].permission.permission)) {
            return FAILED;
        }
        if (!reader_get_short(_ctx, &authority->permissions[i].weight)) {
            return FAILED;
        }
    }
    if (!reader_get_variable_uint(_ctx, &authority->wait_size)) {
        return FAILED;
    }
    if (authority->wait_size > 10) {
        return FAILED;
    }
    for (uint8_t i = 0; i < authority->wait_size; i++) {
        if (!reader_get_int(_ctx, &authority->waits[i].wait_sec)) {
            return FAILED;
        }
        if (!reader_get_short(_ctx, &authority->waits[i].weight)) {
            return FAILED;
        }
    }
    return SUCCESS;
}