#include <stdio.h>
#include "eos.h"
#include "fsm.h"
#include "layout2.h"
#include "messages.h"
#include "ecdsa.h"
#include "protect.h"
#include "crypto.h"
#include "secp256k1.h"
#include "gettext.h"
#include "eos_reader.h"
#include "eos_writer.h"
#include "eos_model.h"
#include "eos_action_reader.h"
#include "eos_action_data_reader.h"
#include "eos_utils.h"
#include "eos_transaction_reader.h"

static bool eos_signing = false;
SHA256_CTX sha256_ctx;
static EOSTxSignature msg_tx_request;
static uint8_t privkey[32];
static uint32_t data_left;

static char _next[] = _("Next");
static char _confirm[] = _("Confirm");
static char _cancel[] = _("Cancel");

static inline void hash_bytes(const uint8_t *buf, const size_t size)
{
	sha256_Update(&sha256_ctx, buf, size);
}

static void hash_uint(const uint64_t value, const size_t size) 
{
	uint8_t buff[8];
	for (size_t i = 0; i < size; i++)
	{
		buff[i] = (uint8_t)(value >> i * 8 & 0xFF);
	}
	hash_bytes(buff, size);
}

static void hash_vint(const uint64_t value) 
{
	uint64_t val = value;
	int index = 0;
	uint8_t buff[8];
	do {
        uint8_t b = (uint8_t)((val) & 0x7f);
        val >>= 7;
        b |= ( ((val > 0) ? 1 : 0 ) << 7 );
        buff[index++] = b;
    } while( val != 0 );
	hash_bytes(buff, index);	
}

static void hash_zero(const size_t size) 
{
	uint8_t _zero[size];
	memset(_zero, 0, size);
	hash_bytes(_zero, size);
}

void eos_signing_abort(void)
{
    if (eos_signing) {
		memset(privkey, 0, sizeof(privkey));
		layoutHome();
		eos_signing = false;
	}
}

static int eos_is_canonic(uint8_t v, uint8_t signature[64])
{
	(void) signature;
	return (v & 2) == 0;
}

static void send_signature(void)
{
	uint8_t hash[32], sig[64];
	uint8_t v;
	
	sha256_Final(&sha256_ctx,hash);

	if (ecdsa_sign_digest_DER(&secp256k1, privkey, hash, sig, &v, eos_is_canonic) != 0) {
		fsm_sendFailure(FailureType_Failure_ProcessError, _("Signing failed"));
		eos_signing_abort();
		return;
	}

	layoutProgress(_("Signing"), 1000);

	memset(privkey, 0, sizeof(privkey));

	/* Send back the result */
	msg_tx_request.has_signature_v = true;
	
	msg_tx_request.signature_v = v;
	
	msg_tx_request.has_signature_r = true;
	msg_tx_request.signature_r.size = 32;
	memcpy(msg_tx_request.signature_r.bytes, sig, 32);

	msg_tx_request.has_signature_s = true;
	msg_tx_request.signature_s.size = 32;
	memcpy(msg_tx_request.signature_s.bytes, sig+32, 32);

	msg_write(MessageType_MessageType_EOSTxSignature, &msg_tx_request);

	eos_signing_abort();
}

bool confirm_eosio_newaccount(EosReaderCTX *ctx)
{
	EosioNewAccount new_account;
	if (!reader_get_newaccount(ctx, &new_account)) {
		return false;
	}

	char _confirm_create_desc[] = "Confirm creating";
	char _account[] = "account:";
	char new_account_name[13];
	memset(new_account_name, 0, 13);
	name_to_str(new_account.new_name, new_account_name);

	layoutDialogSwipe(
		&bmp_icon_question,
		_cancel, _confirm, NULL,
		_confirm_create_desc, _account, new_account_name, NULL, NULL, NULL
	);

	if (!protectButton(ButtonRequestType_ButtonRequest_SignTx, false)) {
		return false;
	}

	uint16_t owner_count = new_account.owner.key_size + new_account.owner.permission_size;
	uint16_t owner_weight = 0;
	for (uint8_t i = 0; i < new_account.owner.key_size; i++ ) {
		owner_weight += new_account.owner.keys[i].weight;
	}
	for (uint8_t i = 0; i < new_account.owner.permission_size; i++ ) {
		owner_weight += new_account.owner.permissions[i].weight;
	}

	uint16_t active_count = new_account.active.key_size + new_account.active.permission_size;
	uint16_t active_weight = 0;
	for (uint8_t i = 0; i < new_account.active.key_size; i++ ) {
		active_weight += new_account.active.keys[i].weight;
	}
	for (uint8_t i = 0; i < new_account.active.permission_size; i++ ) {
		active_weight += new_account.active.permissions[i].weight;
	}

	if (owner_count == 0 || active_count == 0) {
		return false;
	}

	char _confirm_owner[] = "Confirm owner";
	if (owner_count > 1) {
		char _weight[21];
		char _threshold[21];
		memset(_weight, 0, 21);
		memset(_threshold, 0, 21);

		sprintf(_weight, "weight: %u", owner_weight);
		sprintf(_threshold, "throshold: %lu", new_account.owner.threshold);

		layoutDialogSwipe(
			&bmp_icon_question,
			_cancel, _confirm, NULL,
			_confirm_owner, _weight, _threshold, NULL, NULL, NULL
		);
		if (!protectButton(ButtonRequestType_ButtonRequest_SignTx, false)) {
			return false;
		}
	}
	for (uint8_t i = 0; i < new_account.owner.key_size; i++ ) {
		char _owner_public_keys[] = "Owner public keys";
		char pubkey[61];
		format_eos_pubkey(new_account.owner.keys[i].pubkey.pubkey, 33, i + 1, pubkey);
		char _pubkey1[21];
		char _pubkey2[21];
		char _pubkey3[21];
		memset(_pubkey1, 0, 21);
		memset(_pubkey2, 0, 21);
		memset(_pubkey3, 0, 21);

		memcpy(_pubkey1, pubkey, 20);
		memcpy(_pubkey2, pubkey + 20, 20);
		memcpy(_pubkey3, pubkey + 40, 20);

		char _weight[21];
		memset(_weight, 0, 21);
		sprintf(_weight, "weight: %u", new_account.owner.keys[i].weight);

		layoutDialogSwipe(
			&bmp_icon_question,
			_cancel, _confirm, NULL,
			_owner_public_keys, _pubkey1, _pubkey2, _pubkey3, _weight, NULL
		);

		if (!protectButton(ButtonRequestType_ButtonRequest_SignTx, false)) {
			return false;
		}
	}

 	for (uint8_t i = 0; i < new_account.owner.permission_size; i ++ ) {
		char _owner_account[] = "Owner accounts";
		char _account_name[21];
		char _permission[21];
		char _weight[21];
		memset(_account_name, 0, 21);
		memset(_permission, 0, 21);
		memset(_weight, 0, 21);

		format_producer(new_account.owner.permissions[i].permission.actor, i + 1, _account_name);
		sprintf(_weight, "weight: %u", new_account.owner.permissions[i].weight);
		char per[13];
		memset(per, 0, 13);
		name_to_str(new_account.owner.permissions[i].permission.permission, per);
		sprintf(_permission, "permission: %s", per);

		layoutDialogSwipe(
			&bmp_icon_question,
			_cancel, _confirm, NULL,
			_owner_account, _account_name, _permission, _weight, NULL, NULL
		);
		if (!protectButton(ButtonRequestType_ButtonRequest_SignTx, false)) {
			return false;
		}
	 }

	char _confirm_active[] = "Confirm active";
	 if (active_count > 1) {
		char _weight[21];
		char _threshold[21];
		memset(_weight, 0, 21);
		memset(_threshold, 0, 21);

		sprintf(_weight, "weight: %u", owner_weight);
		sprintf(_threshold, "throshold: %lu", new_account.owner.threshold);

		layoutDialogSwipe(
			&bmp_icon_question,
			_cancel, _confirm, NULL,
			_confirm_active, _weight, _threshold, NULL, NULL, NULL
		);
		if (!protectButton(ButtonRequestType_ButtonRequest_SignTx, false)) {
			return false;
		}
	}

	 for (uint8_t i = 0; i < new_account.active.key_size; i++ ) {
		char _active_public_keys[] = "Active public keys";
		char pubkey[61];
		memset(pubkey, 0, 61);
		format_eos_pubkey(new_account.active.keys[i].pubkey.pubkey, 33, i + 1, pubkey);

		char _pubkey1[21];
		char _pubkey2[21];
		char _pubkey3[21];
		memset(_pubkey1, 0, 21);
		memset(_pubkey2, 0, 21);
		memset(_pubkey3, 0, 21);

		memcpy(_pubkey1, pubkey, 20);
		memcpy(_pubkey2, pubkey + 20, 20);
		memcpy(_pubkey3, pubkey + 40, 20);
		char _weight[21];
		memset(_weight, 0, 21);
		sprintf(_weight, "weight: %u", new_account.active.keys[i].weight);

		layoutDialogSwipe(
			&bmp_icon_question,
			_cancel, _confirm, NULL,
			_active_public_keys, _pubkey1, _pubkey2, _pubkey3, _weight, NULL
		);

		if (!protectButton(ButtonRequestType_ButtonRequest_SignTx, false)) {
			return false;
		}
	}

 	for (uint8_t i = 0; i < new_account.active.permission_size; i ++ ) {
		char _active_account[] = "Active accounts";
		char _account_name[21];
		char _permission[21];
		char _weight[21];
		memset(_account_name, 0, 21);
		memset(_permission, 0, 21);
		memset(_weight, 0, 21);

		format_producer(new_account.active.permissions[i].permission.actor, i + 1, _account_name);
		sprintf(_weight, "weight: %u", new_account.active.permissions[i].weight);
		char per[13];
		memset(per, 0, 13);
		name_to_str(new_account.active.permissions[i].permission.permission, per);
		sprintf(_permission, "permission: %s", per);

		layoutDialogSwipe(
			&bmp_icon_question,
			_cancel, _confirm, NULL,
			_active_account, _account_name, _permission, _weight, NULL, NULL
		);
		if (!protectButton(ButtonRequestType_ButtonRequest_SignTx, false)) {
			return false;
		}
	}

	char _relly_create_desc[] = "Really create";
	layoutDialogSwipe(
		&bmp_icon_question,
		_cancel, _confirm, NULL,
		_relly_create_desc, _account, new_account_name, NULL, NULL, NULL
	);
	if (!protectButton(ButtonRequestType_ButtonRequest_SignTx, false)) {
			return false;
	}
	return true;
}

bool confirm_eosio_buyram(EosReaderCTX *ctx)
{
	EosioBuyram buyram;
	if (!reader_get_buyram(ctx, &buyram)) {
		return false;
	}

	char _confirm_buy_desc[] = "Confirm buying";
	char _amount[21];
	char _for_account[] = "ram for account:";
	char _receiver[21];

	memset(_amount, 0, 21);
	memset(_receiver, 0, 21);

	format_asset(&buyram.quantity, _amount);
	name_to_str(buyram.receiver, _receiver);

	layoutDialogSwipe(
		&bmp_icon_question,
		_cancel, _confirm, NULL,
		_confirm_buy_desc, _amount, _for_account, _receiver, NULL, NULL
	);

	if (!protectButton(ButtonRequestType_ButtonRequest_SignTx, false)) {
		return false;
	}

	char _really_buy_desc[] = "Really buy";
	char _pay_account_desc[] = "ram, Pay account:";
	char _pay_account[21];
	memset(_pay_account, 0, 21);
	name_to_str(buyram.from, _pay_account);

	layoutDialogSwipe(
		&bmp_icon_question,
		_cancel, _confirm, NULL,
		_really_buy_desc, _amount, _pay_account_desc, _pay_account, NULL, NULL
	);

	return protectButton(ButtonRequestType_ButtonRequest_SignTx, false);
}

bool confirm_eosio_buyram_bytes(EosReaderCTX *ctx)
{
	EosioBuyramBytes buyram;
	if (!reader_get_buyram_bytes(ctx, &buyram)) {
		return false;
	}

	char _confirm_buy_desc[] = "Confirm buying";
	char _amount[21];
	char _for_account[] = "ram for account:";
	char _receiver[21];

	memset(_amount, 0, 21);
	memset(_receiver, 0, 21);

	sprintf(_amount, "%lu bytes", buyram.bytes);
	name_to_str(buyram.receiver, _receiver);

	layoutDialogSwipe(
		&bmp_icon_question,
		_cancel, _confirm, NULL,
		_confirm_buy_desc, _amount, _for_account, _receiver, NULL, NULL
	);

	if (!protectButton(ButtonRequestType_ButtonRequest_SignTx, false)) {
		return false;
	}

	char _really_buy_desc[] = "Really buy";
	char _pay_account_desc[] = "ram, Pay account:";
	char _pay_account[21];

	memset(_pay_account, 0, 21);
	name_to_str(buyram.from, _pay_account);

	layoutDialogSwipe(
		&bmp_icon_question,
		_cancel, _confirm, NULL,
		_really_buy_desc, _amount, _pay_account_desc, _pay_account, NULL, NULL
	);

	return protectButton(ButtonRequestType_ButtonRequest_SignTx, false);
}

bool confirm_eosio_sell_ram(EosReaderCTX *ctx)
{
	EosioSellram sellram;
	if (!reader_get_sellram(ctx, &sellram)) {
		return false;
	}

	char _confirm_sell_desc[] = "Confirm selling";
	char _sell_bytes[21];
	char _seller_account_desc[] = "seller account:";
	char _seller_account[21];

	memset(_sell_bytes, 0, 21);
	memset(_seller_account, 0, 21);

	sprintf(_sell_bytes, "%llu bytes", sellram.bytes);
	name_to_str(sellram.from, _seller_account);

	layoutDialogSwipe(
		&bmp_icon_question,
		_cancel, _confirm, NULL,
		_confirm_sell_desc, _sell_bytes, _seller_account_desc, _seller_account, NULL, NULL
	);

	if (!protectButton(ButtonRequestType_ButtonRequest_SignTx, false)) {
		return false;
	}

	char _really_sell_desc[] = "Really sell";

	layoutDialogSwipe(
		&bmp_icon_question,
		_cancel, _confirm, NULL,
		_really_sell_desc, _sell_bytes, _seller_account_desc, _seller_account, NULL, NULL
	);


	return protectButton(ButtonRequestType_ButtonRequest_SignTx, false);
}

bool confirm_eosio_delegate(EosReaderCTX *ctx) 
{
	EosioDelegate delegate;
	if (!reader_get_delegage(ctx, &delegate)) {
		return false;
	}

	if (delegate.net_quantity.amount == 0 && delegate.cpu_quantity.amount == 0) {
		return false;
	}

	char _confirm_plage_desc[] = "Confirm placing";
	char _amount1[21];
	char _desc1[21];
	char _amount2[21];
	char _desc2[21];

	memset(_amount1, 0, 21);
	memset(_desc1, 0, 21);
	memset(_amount2, 0, 21);
	memset(_desc2, 0, 21);

	if (delegate.net_quantity.amount == 0) {
		format_asset(&delegate.cpu_quantity, _amount1);
		memcpy(_desc1, "in exchange for CPU", 19);
	} else {
		format_asset(&delegate.net_quantity, _amount1);
		memcpy(_desc1, "in exchange for NET", 19);

		if (delegate.cpu_quantity.amount > 0) {
			format_asset(&delegate.cpu_quantity, _amount2);
			memcpy(_desc2, "in exchange for CPU", 19);
		}
	}

	layoutDialogSwipe(
		&bmp_icon_question,
		_cancel, _confirm, NULL,
		_confirm_plage_desc, _amount1, _desc1, _amount2, _desc2, NULL
	);

	if (!protectButton(ButtonRequestType_ButtonRequest_SignTx, false)) {
		return false;
	}

	char _to[] = "to";
	char _account[21];
	memset(_account, 0, 21);
	name_to_str(delegate.receiver, _account);

	if (delegate.tansfer) {
		char _transfer_desc[] = "Do you want transfer";
		char _these_resouse[] = "these resources";
		layoutDialogSwipe(
			&bmp_icon_question,
			_cancel, _confirm, NULL,
			_transfer_desc, _these_resouse, _to, _account, NULL, NULL
		);

		if (!protectButton(ButtonRequestType_ButtonRequest_SignTx, false)) {
			return false;
		}
	}
	uint64_t total_amount = delegate.cpu_quantity.amount + delegate.net_quantity.amount;
	EosTypeAsset totalAsset;
	totalAsset.amount = total_amount;
	totalAsset.symbol = delegate.cpu_quantity.symbol;
	char _really_confirm[] = "Really plage";
	char _total_amount[21];
	char _for_resource[] = "for resources";

	memset(_total_amount, 0, 21);
	format_asset(&totalAsset, _total_amount);

	layoutDialogSwipe(
		&bmp_icon_question,
		_cancel, _confirm, NULL,
		_really_confirm, _total_amount, _for_resource, _to, _account, NULL
	);

	if (!protectButton(ButtonRequestType_ButtonRequest_SignTx, false)) {
			return false;
	}

	char _pay_account[] = "pay account";
	memset(_account, 0, 21);
	name_to_str(delegate.from, _account);

	layoutDialogSwipe(
		&bmp_icon_question,
		_cancel, _confirm, NULL,
		_really_confirm, _total_amount, _for_resource, _pay_account, _account, NULL
	);

	return protectButton(ButtonRequestType_ButtonRequest_SignTx, false);
}

bool confirm_eosio_undelegate(EosReaderCTX *ctx) 
{
	EosioUndelegate undelegate;
	if (!reader_get_undelegate(ctx, &undelegate)) {
		return false;
	}

	char _confirm_redeem_desc[] = "Confirm redeeming";
	char _amount1[21];
	char _desc1[21];
	char _amount2[21];
	char _desc2[21];

	memset(_amount1, 0, 21);
	memset(_desc1, 0, 21);
	memset(_amount2, 0, 21);
	memset(_desc2, 0, 21);

	if (undelegate.net_quantity.amount == 0) {
		format_asset(&undelegate.cpu_quantity, _amount1);
		strcpy(_desc1, "from CPU");
	} else {
		format_asset(&undelegate.net_quantity, _amount1);
		strcpy(_desc1, "from NET");

		if (undelegate.cpu_quantity.amount > 0) {
			format_asset(&undelegate.cpu_quantity, _amount2);
			strcpy(_desc2, "from CPU");
		}
	}

	layoutDialogSwipe(
		&bmp_icon_question,
		_cancel, _confirm, NULL,
		_confirm_redeem_desc, _amount1, _desc1, _amount2, _desc2, NULL
	);

	if (!protectButton(ButtonRequestType_ButtonRequest_SignTx, false)) {
		return false;
	}

	char _from[] = "from account";
	char _account[21];
	memset(_account, 0, 21);
	name_to_str(undelegate.from, _account);

	layoutDialogSwipe(
		&bmp_icon_question,
		_cancel, _confirm, NULL,
		_confirm_redeem_desc, _from, _account, NULL, NULL, NULL
	);

	if (!protectButton(ButtonRequestType_ButtonRequest_SignTx, false)) {
		return false;
	}

	uint64_t total_amount = undelegate.cpu_quantity.amount + undelegate.net_quantity.amount;
	EosTypeAsset totalAsset;
	totalAsset.amount = total_amount;
	totalAsset.symbol = undelegate.cpu_quantity.symbol;
	char _really_confirm[] = "Really redeem";
	char _total_amount[21];
	char _for_resource[] = "from resources";
	char _to[] = "to";

	memset(_total_amount, 0, 21);
	memset(_account, 0, 21);

	format_asset(&totalAsset, _total_amount);
	name_to_str(undelegate.receiver, _account);

	layoutDialogSwipe(
		&bmp_icon_question,
		_cancel, _confirm, NULL,
		_really_confirm, _total_amount, _for_resource, _to, _account, NULL
	);

	return protectButton(ButtonRequestType_ButtonRequest_SignTx, false);
}

bool confirm_eosio_vote_producer(EosReaderCTX *ctx)
{
	EosioVoteProducer vote_producer;
	if (!reader_get_vote_producer(ctx, &vote_producer)) {
		return false;
	}

	char _voting_desc[] = "Confirm voting";
	char _producers[] = "producers:";
	char _producer1[21]; 
	char _producer2[21];
	char _producer3[21];

	uint8_t page_count = 0;

	while (page_count < vote_producer.producer_size) {

		memset(_producer1, 0, 21);
		memset(_producer2, 0, 21);
		memset(_producer3, 0, 21);	

		uint8_t left_count = vote_producer.producer_size - page_count;
		uint8_t page_size = left_count > 3? 3: left_count;
		switch (page_size) 
		{
			case 3: 
				format_producer(vote_producer.producers[page_count + 2], page_count + 3, _producer3);
			case 2: 
				format_producer(vote_producer.producers[page_count + 1], page_count + 2, _producer2);
			case 1: 
				format_producer(vote_producer.producers[page_count + 0], page_count + 1, _producer1);
				break;
		}

		page_count += 3;

		layoutDialogSwipe(
			&bmp_icon_question,
			_cancel, page_count >= vote_producer.producer_size? _confirm: _next, NULL,
			_voting_desc, _producers, _producer1, _producer2, _producer3, NULL
		);

		if (!protectButton(ButtonRequestType_ButtonRequest_SignTx, false)) {
			return false;
		}
	}

	char _relly_vote[] = "Relly vote for these";
	char _relly_producers[] = "producers?";
	char _voter[] = "voter:";
	char voter[13];
	memset(voter, 0, 13);
	name_to_str(vote_producer.voter, voter);

	layoutDialogSwipe(
		&bmp_icon_question,
		_cancel, _confirm, NULL,
		_relly_vote, _relly_producers, _voter, voter, NULL, NULL
	);

	return protectButton(ButtonRequestType_ButtonRequest_SignTx, false);
}

bool confirm_eosio_refund(EosReaderCTX *ctx)
{
	EosioRefund refund;
	if (!reader_get_refund(ctx, &refund)) {
		return false;
	}
	char _refund_desc[] = "Confirm refund tokens for";
	char _owner_desc[] = "owner:";
	char _owner[21]; 
	memset(_owner, 0, 21);
	name_to_str(refund.owner, _owner);
	
	layoutDialogSwipe(
		&bmp_icon_question,
		_cancel, _confirm, NULL,
		_refund_desc, _owner_desc, _owner, NULL, NULL, NULL
	);

	return protectButton(ButtonRequestType_ButtonRequest_SignTx, false);
}

bool confirm_token_transfer(EosReaderCTX *ctx, uint64_t account) 
{
	EosioTokenTransfer transfer;
	if (!reader_get_transfer(ctx, &transfer)) {
		return false;
	}

	char _sending_desc[] = "Confirm sending";
	char _send_value[21];
	char _to_desc[21];
	char _to[21];

	if (account == EOSIO_TOKEN) {
		strcpy(_to_desc, "to:");
	} else {
		strcpy(_to_desc, "token to:");
	}

	memset(_send_value, 0, 21);
	memset(_to, 0, 21);

	name_to_str(transfer.to, _to);
	format_asset(&transfer.quantity, _send_value);

	layoutDialogSwipe(
		&bmp_icon_question,
		_cancel, _confirm, NULL,
		_sending_desc, _send_value, _to_desc, _to, NULL, NULL
	);
	if (!protectButton(ButtonRequestType_ButtonRequest_SignTx, false)) {
		return false;
	}

	char _really_send[] = "Really send";
	char _from_desc[] = "pay account:";
	char _from[21];
	memset(_from, 0, 21);
	name_to_str(transfer.from, _from);

	layoutDialogSwipe(
		&bmp_icon_question,
		_cancel, _confirm, NULL,
		_really_send, _send_value, _from_desc, _from, NULL, NULL
	);
	return protectButton(ButtonRequestType_ButtonRequest_SignTx, false); 
}

bool confirm_eosio_msig_propose(EosReaderCTX *ctx)
{
	EosioMsigPropose propose;
	if (!reader_get_propose(ctx, &propose)) {
		return false;
	}

	char _confirm_creating[] = "Confirm creating";
	char _proposal_desc[] = "proposal name:";
	char _proposal[21];
	char _proposer_desc[] = "proposer:";
	char _proposer[21];

	memset(_proposal, 0, 21);
	memset(_proposer, 0, 21);

	name_to_str(propose.proposal_name, _proposal);
	name_to_str(propose.proposer, _proposer);

	layoutDialogSwipe(
		&bmp_icon_question,
		_cancel, _confirm, NULL,
		_confirm_creating, _proposal_desc, _proposal, _proposer_desc, _proposer, NULL
	);

	if (!protectButton(ButtonRequestType_ButtonRequest_SignTx, false)) {
		return false;
	}

	EosPermissionLevel requested;
	for (uint8_t i = 0; i < propose.requested_size; i ++ ) {
		memset(&requested, 0, sizeof(EosPermissionLevel));
		if (!reader_get_long(ctx, &requested.actor)) {
             return false;
        }
        if (!reader_get_long(ctx, &requested.permission)) {
            return false;
        }
		// show requested ?
	} 

	// show inner transaction.
	EosTransaction inner_trx;
	if (!transaction_reader_get(ctx, &inner_trx)) {
		return false;
	}
	uint64_t action_count = action_reader_count(ctx);
	for (uint8_t i=0; i < action_count; i ++) {
		if (!confirm_action(ctx)) {
			return false;
		}
	}

	char _really_create[] = "Really create";
	layoutDialogSwipe(
		&bmp_icon_question,
		_cancel, _confirm, NULL,
		_really_create, _proposal_desc, _proposal, _proposer_desc, _proposer, NULL
	);

	return protectButton(ButtonRequestType_ButtonRequest_SignTx, false);
}

bool confirm_eosio_msig_cancel(EosReaderCTX *ctx)
{
	EosioMsigCancel cancel;
	if (!reader_get_cancel(ctx, &cancel)) {
		return false;
	}

	char _confirm_cancel[] = "Confirm cancel";
	char _proposal_desc[] = "proposal name:";
	char _proposal[21];
	char _proposer_desc[] = "proposer:";
	char _proposer[21];

	memset(_proposal, 0, 21);
	memset(_proposer, 0, 21);

	name_to_str(cancel.proposal_name, _proposal);
	name_to_str(cancel.proposer, _proposer);

	layoutDialogSwipe(
		&bmp_icon_question,
		_cancel, _confirm, NULL,
		_confirm_cancel, _proposal_desc, _proposal, _proposer_desc, _proposer, NULL
	);

	if (!protectButton(ButtonRequestType_ButtonRequest_SignTx, false)) {
		return false;
	}

	char _really_cancel[] = "Really cancel";
	char _canceler_desc[] = "canceler:";
	char _canceler[21];

	memset(_canceler, 0, 21);
	name_to_str(cancel.canceler, _canceler);

	layoutDialogSwipe(
		&bmp_icon_question,
		_cancel, _confirm, NULL,
		_really_cancel, _proposal_desc, _proposal, _canceler_desc, _canceler, NULL
	);

	return protectButton(ButtonRequestType_ButtonRequest_SignTx, false);
}

bool confirm_eosio_msig_approve(EosReaderCTX *ctx)
{
	EosioMsigApprove approve;
	if (!reader_get_approve(ctx, &approve)) {
		return false;
	}

	char _confirm_approve[] = "Confirm approve";
	char _proposal_desc[] = "proposal name:";
	char _proposal[21];
	char _proposer_desc[] = "proposer:";
	char _proposer[21];

	memset(_proposal, 0, 21);
	memset(_proposer, 0, 21);

	name_to_str(approve.proposal_name, _proposal);
	name_to_str(approve.proposer, _proposer);

	layoutDialogSwipe(
		&bmp_icon_question,
		_cancel, _confirm, NULL,
		_confirm_approve, _proposal_desc, _proposal, _proposer_desc, _proposer, NULL
	);

	if (!protectButton(ButtonRequestType_ButtonRequest_SignTx, false)) {
		return false;
	}

	char _really_approve[] = "Really approve";
	char _approver_desc[] = "approver:";
	char _approver[21];

	memset(_approver, 0, 21);
	name_to_str(approve.level.actor, _approver);

	layoutDialogSwipe(
		&bmp_icon_question,
		_cancel, _confirm, NULL,
		_really_approve, _proposal_desc, _proposal, _approver_desc, _approver, NULL
	);

	return protectButton(ButtonRequestType_ButtonRequest_SignTx, false);
}

bool confirm_eosio_msig_unapprove(EosReaderCTX *ctx)
{
	EosioMsigUnapprove unapprove;
	if (!reader_get_unapprove(ctx, &unapprove)) {
		return false;
	}
	char _confirm_unapprove[] = "Confirm unapprove";
	char _proposal_desc[] = "proposal name:";
	char _proposal[21];
	char _proposer_desc[] = "proposer:";
	char _proposer[21];

	memset(_proposal, 0, 21);
	memset(_proposer, 0, 21);

	name_to_str(unapprove.proposal_name, _proposal);
	name_to_str(unapprove.proposer, _proposer);

	layoutDialogSwipe(
		&bmp_icon_question,
		_cancel, _confirm, NULL,
		_confirm_unapprove, _proposal_desc, _proposal, _proposer_desc, _proposer, NULL
	);

	if (!protectButton(ButtonRequestType_ButtonRequest_SignTx, false)) {
		return false;
	}

	char _really_unapprove[] = "Really unapprove";
	char _unapprover_desc[] = "unapprover:";
	char _unapprover[21];

	memset(_unapprover, 0, 21);
	name_to_str(unapprove.level.actor, _unapprover);

	layoutDialogSwipe(
		&bmp_icon_question,
		_cancel, _confirm, NULL,
		_really_unapprove, _proposal_desc, _proposal, _unapprover_desc, _unapprover, NULL
	);

	return protectButton(ButtonRequestType_ButtonRequest_SignTx, false);
}

bool confirm_eosio_msig_exec(EosReaderCTX *ctx)
{
	EosioMsigExec exec;
	if (!reader_get_exec(ctx, &exec)) {
		return false;
	}

	char _confirm_execute[] = "Confirm execute";
	char _proposal_desc[] = "proposal name:";
	char _proposal[21];
	char _proposer_desc[] = "proposer:";
	char _proposer[21];

	memset(_proposal, 0, 21);
	memset(_proposer, 0, 21);

	name_to_str(exec.proposal_name, _proposal);
	name_to_str(exec.proposer, _proposer);

	layoutDialogSwipe(
		&bmp_icon_question,
		_cancel, _confirm, NULL,
		_confirm_execute, _proposal_desc, _proposal, _proposer_desc, _proposer, NULL
	);

	if (!protectButton(ButtonRequestType_ButtonRequest_SignTx, false)) {
		return false;
	}

	char _really_execute[] = "Really execute";
	char _executer_desc[] = "executer:";
	char _executer[21];
	
	memset(_executer, 0, 21);
	name_to_str(exec.executer, _executer);

	layoutDialogSwipe(
		&bmp_icon_question,
		_cancel, _confirm, NULL,
		_really_execute, _proposal_desc, _proposal, _executer_desc, _executer, NULL
	);

	return protectButton(ButtonRequestType_ButtonRequest_SignTx, false);
}

bool confirm_action(EosReaderCTX *ctx)
{
	EosAction action;
	if (!action_reader_next(ctx, &action)) {
		return false;
	}
	if (action.account == EOSIO) {
		switch (action.name) 
		{
			case ACTION_NEW_ACCOUNT:
				return confirm_eosio_newaccount(ctx);
			case ACTION_BUY_RAM:
				return confirm_eosio_buyram(ctx);
			case ACTION_SELL_RAM: 
				return confirm_eosio_sell_ram(ctx);
			case ACTION_BUY_RAM_BYTES: 
				return confirm_eosio_buyram_bytes(ctx);
			case ACTION_DELEGATE: 
				return confirm_eosio_delegate(ctx);
			case ACTION_UNDELEGATE: 
			 	return confirm_eosio_undelegate(ctx);
			case ACTION_VOTE_PRODUCER:
				return confirm_eosio_vote_producer(ctx); 
			case ACTION_REFUND:
				return confirm_eosio_refund(ctx);
			default:
				break;
		}
	} else if (action.account == EOSIO_MSIG) {
		switch (action.name)
		{
			case ACTION_PROPOSE: 
				return confirm_eosio_msig_propose(ctx);
			case ACTION_CANCEL: 
				return confirm_eosio_msig_cancel(ctx);
			case ACTION_UNAPPROVE: 
				return confirm_eosio_msig_unapprove(ctx);
			case ACTION_APPROVE: 
				return confirm_eosio_msig_approve(ctx);
			case ACTION_EXEC: 
				return confirm_eosio_msig_exec(ctx);
			default:
			break;
		}
	} else {
		if (action.name == ACTION_TRANSMFER) {
			return confirm_token_transfer(ctx, action.account);
		} else {
			// unsupport actions.
		}
	}
	return false;
}

void eos_signing_init(EOSSignTx *msg, const HDNode *node)
{
	EosReaderCTX reader_ctx;
	action_reader_init(&reader_ctx, msg->actions.bytes, msg->actions.size + 1);
	uint64_t action_count = action_reader_count(&reader_ctx);
	for (uint8_t i=0; i < action_count; i ++) {
		if (!confirm_action(&reader_ctx)) {
			fsm_sendFailure(FailureType_Failure_UnexpectedMessage, _("Error"));
			return;
		}
	}

	eos_signing = true;
	sha256_Init(&sha256_ctx);

	//step1
	layoutProgress(_("Signing"), 0);
	hash_bytes(msg->chain_id.bytes, msg->chain_id.size); // chain_id.
	hash_uint(msg->expiration, 4); // expiration
	hash_uint(msg->ref_block_num, 2); // ref_block_num
	hash_uint(msg->ref_block_prefix, 4); // ref_block_prefix.
	hash_vint(msg->max_net_usage_words); // max_net_usage_words
	hash_vint(msg->max_cpu_usage_ms); // max_cpu_usage_ms
	hash_vint(msg->delay_sec);		// dalay_sec
	hash_bytes(msg->free_actions.bytes, msg->free_actions.size);
	hash_bytes(msg->actions.bytes,msg->actions.size);
	hash_zero(33);

	//step2
	layoutProgress(_("Signing"), 100);
    memcpy(privkey, node->private_key, 32);

    send_signature();
}

static void send_request_chunk(void)
{

}
void eos_signing_txack(EOSTxAck *tx)
{
    tx->has_data = false;

    if (!eos_signing) {
		fsm_sendFailure(FailureType_Failure_UnexpectedMessage, _("Not in Eos signing mode"));
		layoutHome();
		return;
	}

	if (data_left > 0) {
		send_request_chunk();
	} else {
		send_signature();
	}
}