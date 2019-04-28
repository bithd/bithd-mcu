#ifndef _EOS_ACTION_DATA_READER_H_
#define _EOS_ACTION_DATA_READER_H_
#include <stdint.h>
#include "eos_model.h"
#include "eos_reader.h"

#define VOTE_PRODUCER_MAX_COUNT 30
#define TRANSFER_MEMO_MAX_SIZE 256

void action_data_reader_init(EosReaderCTX *, uint8_t *, int);

// eosio

bool reader_get_newaccount(EosReaderCTX *, EosioNewAccount *);

bool reader_get_buyram(EosReaderCTX *, EosioBuyram *);

bool reader_get_buyram_bytes(EosReaderCTX *, EosioBuyramBytes *);

bool reader_get_sellram(EosReaderCTX *, EosioSellram *);

bool reader_get_delegage(EosReaderCTX *, EosioDelegate *);

bool reader_get_undelegate(EosReaderCTX *, EosioUndelegate *);

bool reader_get_vote_producer(EosReaderCTX *, EosioVoteProducer *);

bool reader_get_refund(EosReaderCTX *, EosioRefund *);

// eosio.token

bool reader_get_transfer(EosReaderCTX *, EosioTokenTransfer *);

// eosio.msig

bool reader_get_propose(EosReaderCTX *, EosioMsigPropose *);

bool reader_get_approve(EosReaderCTX *, EosioMsigApprove *);

bool reader_get_cancel(EosReaderCTX *, EosioMsigCancel *);

bool reader_get_exec(EosReaderCTX *, EosioMsigExec *);

bool reader_get_unapprove(EosReaderCTX *, EosioMsigUnapprove *);

bool reader_get_authority(EosReaderCTX *, EosAuthority *);

#endif