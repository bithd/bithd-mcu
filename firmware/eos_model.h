#ifndef _EOS_MODEL_H_
#define _EOS_MODEL_H_

#include <stdint.h>


#define MAX_NAME_IDX 			13

#define EOSIO  			        0x5530ea0000000000
#define ACTION_NEW_ACCOUNT      0x9ab864229a9e4000
#define ACTION_VOTE_PRODUCER    0xdd32aade89d21570
#define ACTION_DELEGATE 	    0x4aa2a61b2a3f0000
#define ACTION_UNDELEGATE 	    0xd4d2a8a986ca8fc0
#define ACTION_BUY_RAM		    0x3ebd734800000000
#define ACTION_SELL_RAM		    0xc2a31b9a40000000
#define ACTION_BUY_RAM_BYTES    0x3ebd7348fecab000
#define ACTION_REFUND           0xba97a9a400000000

#define EOSIO_TOKEN             0x5530ea033482a600
#define ACTION_TRANSMFER 	    0xcdcd3c2d57000000

#define EOSIO_MSIG              0x5530ea0258730000
#define ACTION_PROPOSE          0xade95a6140000000
#define ACTION_APPROVE          0x356b7a6d40000000
#define ACTION_UNAPPROVE        0xd4cd5ade9b500000
#define ACTION_CANCEL           0x41a6854400000000
#define ACTION_EXEC             0x5754800000000000

typedef uint64_t EosAccountName;
typedef uint64_t EosTypeName;
typedef uint64_t EosPermissionName;
typedef uint64_t EosActionName;

typedef struct eos_type_asset
{
	uint64_t amount;
	uint64_t symbol;
} EosTypeAsset;

typedef struct eos_permission_level {
	EosAccountName actor;
	EosPermissionName permission;
} EosPermissionLevel;

typedef struct eos_public_key
{
    uint64_t curve_param_type;
    uint8_t pubkey[33];
} EosPubKey;

typedef struct eos_key_weight
{
    EosPubKey pubkey;
    uint16_t weight;
} EosKeyWeight;

typedef struct eos_wait_weight
{
    uint32_t wait_sec;
    uint16_t weight;
} EosWaitWeight;

typedef struct eos_permission_weight
{
    EosPermissionLevel permission;
    uint16_t weight;
} EosPermissionWeight;

typedef struct eos_authority
{
    uint32_t threshold;
    uint64_t key_size;
    EosKeyWeight keys[10];
    uint64_t permission_size;
    EosPermissionWeight permissions[10];
    uint64_t wait_size;
    EosWaitWeight waits[10];
} EosAuthority;

typedef struct eos_action
{
	EosAccountName account;
	EosActionName name;
    uint64_t authorization_size;
	EosPermissionLevel authorization[4];
    uint64_t data_size;
} EosAction;

typedef struct eos_transaction
{
    uint32_t expiration;
    uint16_t ref_block_num;
    uint32_t ref_block_prefix;
    uint64_t max_net_usage_words;
    uint64_t max_cpu_usage_ms;
    uint64_t delay_sec;
    uint64_t contract_free_actions_size;
    EosAction contract_free_actions[1];
} EosTransaction;

// eosio

typedef struct eosio_new_account
{
    EosAccountName creator;
    EosAccountName new_name;
    EosAuthority owner;
    EosAuthority active;
} EosioNewAccount;

typedef struct eosio_buyram
{
    EosAccountName from;
    EosAccountName receiver;
    EosTypeAsset quantity;
} EosioBuyram;

typedef struct eosio_buyram_bytes
{
    EosAccountName from;
    EosAccountName receiver;
    uint32_t bytes;
} EosioBuyramBytes;

typedef struct eoiso_sellram
{
    EosAccountName from;
    uint64_t bytes;
} EosioSellram;

typedef struct eosio_delegatebw 
{
    EosAccountName from;
    EosAccountName receiver;
    EosTypeAsset net_quantity;
    EosTypeAsset cpu_quantity;
    uint8_t tansfer;
} EosioDelegate;

typedef struct eosio_undelegatebw 
{
    EosAccountName from;
    EosAccountName receiver;
    EosTypeAsset net_quantity;
    EosTypeAsset cpu_quantity;
} EosioUndelegate;

typedef struct eosio_vote_producer
{
    EosAccountName voter;
    EosAccountName proxy;
    uint64_t producer_size;
    EosAccountName producers[30];
} EosioVoteProducer;

typedef struct eosio_refund
{
    EosAccountName owner;
} EosioRefund;

// eosio.token

typedef struct eosio_token_transfer
{
    EosAccountName from;
    EosAccountName to;
    EosTypeAsset quantity;
    uint64_t memo_size;
    char memo[256];
} EosioTokenTransfer;

// eosio.msig

typedef struct eosio_msig_propose 
{
    EosAccountName proposer;
    EosTypeName proposal_name;
    uint64_t requested_size;
    // EosPermissionLevel requested[4];
} EosioMsigPropose;

typedef struct eosio_msig_approve
{
    EosAccountName proposer;
    EosTypeName proposal_name;
    EosPermissionLevel level;
} EosioMsigApprove;

typedef struct eosio_msig_cancel
{
    EosAccountName proposer;
    EosTypeName proposal_name;
    EosAccountName canceler;
} EosioMsigCancel;

typedef struct eosio_msig_exec
{
    EosAccountName proposer;
    EosTypeName proposal_name;
    EosAccountName executer;
} EosioMsigExec;

typedef struct eosio_msig_unapprove
{
    EosAccountName proposer;
    EosTypeName proposal_name;
    EosPermissionLevel level;
} EosioMsigUnapprove;
#endif