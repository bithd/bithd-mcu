#ifndef _EOS_ACTION_DATA_READER_H_
#define _EOS_ACTION_DATA_READER_H_
#include <stdint.h>
#include "eos_model.h"
#include "eos_reader.h"

#define VOTE_PRODUCER_MAX_COUNT 30
#define TRANSFER_MEMO_MAX_SIZE 256

void action_data_reader_init(EosReaderCTX *, uint8_t *, int);

// eosio

int reader_get_newaccount(EosReaderCTX *, EosioNewAccount *);

int reader_get_buyram(EosReaderCTX *, EosioBuyram *);

int reader_get_buyram_bytes(EosReaderCTX *, EosioBuyramBytes *);

int reader_get_sellram(EosReaderCTX *, EosioSellram *);

int reader_get_delegage(EosReaderCTX *, EosioDelegate *);

int reader_get_undelegate(EosReaderCTX *, EosioUndelegate *);

int reader_get_vote_producer(EosReaderCTX *, EosioVoteProducer *);

int reader_get_refund(EosReaderCTX *, EosioRefund *);

// eosio.token

int reader_get_transfer(EosReaderCTX *, EosioTokenTransfer *);

// eosio.msig

int reader_get_propose(EosReaderCTX *, EosioMsigPropose *);

int reader_get_approve(EosReaderCTX *, EosioMsigApprove *);

int reader_get_cancel(EosReaderCTX *, EosioMsigCancel *);

int reader_get_exec(EosReaderCTX *, EosioMsigExec *);

int reader_get_unapprove(EosReaderCTX *, EosioMsigUnapprove *);

int reader_get_authority(EosReaderCTX *, EosAuthority *);

#endif