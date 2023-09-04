/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_wallet_service.h"

#include <stdint.h>
#include <deque>
#include <set>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/memory/scoped_refptr.h"
#include "base/notreached.h"
#include "base/ranges/algorithm.h"
#include "base/strings/string_number_conversions.h"
#include "base/sys_byteorder.h"
#include "base/types/expected.h"
#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_database_synchronizer.h"
#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_transaction_database.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/common/bitcoin_utils.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "brave/components/brave_wallet/common/hash_utils.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace {

// We use version 2 per
// https://github.com/bitcoin/bips/blob/master/bip-0068.mediawiki#specification
constexpr uint32_t kTransactionsVersion = 2;
constexpr uint32_t kSigHashAll = 1;

std::vector<brave_wallet::mojom::KeyringId> BitcoinKeyringsForNetwork(
    const std::string& network_id) {
  DCHECK(brave_wallet::IsBitcoinNetwork(network_id));
  if (network_id == brave_wallet::mojom::kBitcoinMainnet) {
    return {brave_wallet::mojom::KeyringId::kBitcoin84};
  } else if (network_id == brave_wallet::mojom::kBitcoinTestnet) {
    return {brave_wallet::mojom::KeyringId::kBitcoin84Testnet};
  } else {
    NOTREACHED();
  }

  return {};
}

// TODO(apaymyshev): implement Push* as 'serialize to network' class
void Push8AsLE(uint8_t i, std::vector<uint8_t>& to) {
  to.push_back(i);
}

void Push16AsLE(uint16_t i, std::vector<uint8_t>& to) {
  i = base::ByteSwapToLE16(i);
  base::span<uint8_t> data_to_insert(reinterpret_cast<uint8_t*>(&i), sizeof(i));
  to.insert(to.end(), data_to_insert.begin(), data_to_insert.end());
}

void Push32AsLE(uint32_t i, std::vector<uint8_t>& to) {
  i = base::ByteSwapToLE32(i);
  base::span<uint8_t> data_to_insert(reinterpret_cast<uint8_t*>(&i), sizeof(i));
  to.insert(to.end(), data_to_insert.begin(), data_to_insert.end());
}

void Push64AsLE(uint64_t i, std::vector<uint8_t>& to) {
  i = base::ByteSwapToLE64(i);
  base::span<uint8_t> data_to_insert(reinterpret_cast<uint8_t*>(&i), sizeof(i));
  to.insert(to.end(), data_to_insert.begin(), data_to_insert.end());
}

void PushVectorAsLE(const std::vector<uint8_t>& v, std::vector<uint8_t>& to) {
  to.insert(to.end(), v.rbegin(), v.rend());
}

// https://developer.bitcoin.org/reference/transactions.html#compactsize-unsigned-integers
void PushVarInt(uint64_t i, std::vector<uint8_t>& to) {
  if (i < 0xfd) {
    Push8AsLE(i, to);
  } else if (i < 0xffff) {
    Push8AsLE(0xfd, to);
    Push16AsLE(i, to);
  } else if (i < 0xffffffff) {
    Push8AsLE(0xfe, to);
    Push32AsLE(i, to);
  } else {
    Push8AsLE(0xff, to);
    Push64AsLE(i, to);
  }
}

void PushVarSizeVector(const std::vector<uint8_t>& v,
                       std::vector<uint8_t>& to) {
  PushVarInt(v.size(), to);
  to.insert(to.end(), v.begin(), v.end());
}

void PushOutpoint(const brave_wallet::bitcoin::Outpoint& outpoint,
                  std::vector<uint8_t>& to) {
  PushVectorAsLE(outpoint.txid, to);
  Push32AsLE(outpoint.index, to);
}

}  // namespace

namespace brave_wallet {

struct SendToContext {
 public:
  struct TxInput {
    bitcoin::Output prev_output;  // Output we are going to spend.
    mojom::BitcoinKeyIdPtr key_id;
    // https://github.com/bitcoin/bitcoin/blob/v24.0/src/wallet/spend.cpp#L945
    uint32_t n_sequence = 0xfffffffd;
    std::vector<uint8_t> pubkey;
    std::vector<uint8_t> signature;
  };

  struct TxOutput {
    std::string address;
    std::vector<uint8_t> pubkey_hash;
    std::vector<uint8_t> script_pubkey;
    uint64_t amount = 0;
  };

  std::string network_id;
  mojom::AccountIdPtr account_id;
  std::string address_to;
  uint64_t amount = 0;
  uint64_t fee = 0;
  uint64_t amount_picked = 0;
  std::vector<bitcoin::Output> utxo_list;
  std::vector<TxInput> inputs;
  std::vector<TxOutput> outputs;
  std::vector<std::pair<std::string, mojom::BitcoinKeyIdPtr>> addresses;
  uint32_t locktime = 0;

  std::vector<uint8_t> serialized_transaction;

  mojom::BitcoinWalletService::SendToCallback callback;

  bool IsTestnet() { return network_id == mojom::kBitcoinTestnet; }
  bool UtxoListReady() { return !utxo_list.empty(); }
  bool InputsReady() { return !inputs.empty(); }
  bool OutputsReady() { return !outputs.empty(); }
  bool SignaturesReady() {
    return base::ranges::all_of(
        inputs, [](auto& input) { return !input.signature.empty(); });
  }
  bool IsTransactionSerialized() { return !serialized_transaction.empty(); }

  void ReplyFailure(const std::string& error) {
    std::move(callback).Run("", error);
  }
};

BitcoinWalletService::BitcoinWalletService(
    KeyringService* keyring_service,
    PrefService* prefs,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : keyring_service_(keyring_service),
      bitcoin_rpc_(std::make_unique<BitcoinRpc>(prefs, url_loader_factory)) {
  for (auto* network_id : {
           mojom::kBitcoinMainnet,
           mojom::kBitcoinTestnet,
       }) {
    transaction_database_[network_id] =
        std::make_unique<BitcoinTransactionDatabase>();
    database_synchronizer_[network_id] =
        std::make_unique<BitcoinDatabaseSynchronizer>(
            network_id, bitcoin_rpc_.get(),
            transaction_database_[network_id].get());

    for (auto& keyring_id : BitcoinKeyringsForNetwork(network_id)) {
      DCHECK(IsValidBitcoinNetworkKeyringPair(network_id, keyring_id));
      if (auto keyring_info =
              keyring_service_->GetKeyringInfoSync(keyring_id)) {
        for (auto& account_info : keyring_info->account_infos) {
          StartDatabaseSynchronizer(network_id, *account_info->account_id);
        }
      }
    }
  }
}

BitcoinWalletService::~BitcoinWalletService() = default;

mojo::PendingRemote<mojom::BitcoinWalletService>
BitcoinWalletService::MakeRemote() {
  mojo::PendingRemote<mojom::BitcoinWalletService> remote;
  receivers_.Add(this, remote.InitWithNewPipeAndPassReceiver());
  return remote;
}

void BitcoinWalletService::Bind(
    mojo::PendingReceiver<mojom::BitcoinWalletService> receiver) {
  receivers_.Add(this, std::move(receiver));
}

void BitcoinWalletService::GetBalance(const std::string& network_id,
                                      mojom::AccountIdPtr account_id,
                                      GetBalanceCallback callback) {
  auto account_info =
      GetBitcoinAccountInfoSync(network_id, std::move(account_id));
  if (!account_info) {
    std::move(callback).Run("0", "Couldn't get balance");
    return;
  }

  std::move(callback).Run(base::NumberToString(account_info->balance), "");
}

void BitcoinWalletService::GetBitcoinAccountInfo(
    const std::string& network_id,
    mojom::AccountIdPtr account_id,
    GetBitcoinAccountInfoCallback callback) {
  std::move(callback).Run(
      GetBitcoinAccountInfoSync(network_id, std::move(account_id)));
}

mojom::BitcoinAccountInfoPtr BitcoinWalletService::GetBitcoinAccountInfoSync(
    const std::string& network_id,
    mojom::AccountIdPtr account_id) {
  if (!IsValidBitcoinNetworkKeyringPair(network_id, account_id->keyring_id)) {
    NOTREACHED();
    return nullptr;
  }

  const auto& addresses = keyring_service_->GetBitcoinAddresses(*account_id);
  if (!addresses) {
    NOTREACHED();
    return nullptr;
  }

  auto bitcoin_account_info = mojom::BitcoinAccountInfo::New();

  for (auto& account_info : keyring_service_->GetAllAccountInfos()) {
    if (account_info->account_id == account_id) {
      account_info->name = account_info->name;
      break;
    }
  }

  for (const auto& address : addresses.value()) {
    const auto& info = bitcoin_account_info->address_infos.emplace_back(
        mojom::BitcoinAddressInfo::New());
    info->address_string = address.first;
    info->key_id = address.second.Clone();

    const auto& outputs = transaction_database_[network_id]->GetUnspentOutputs(
        info->address_string);
    for (const auto& o : outputs) {
      info->utxo_list.push_back(mojom::BitcoinUnspentOutput::New(
          o.outpoint.txid, o.outpoint.index, o.value));
      info->balance += o.value;
      bitcoin_account_info->balance += o.value;
    }
  }

  return bitcoin_account_info;
}

void BitcoinWalletService::SendTo(const std::string& network_id,
                                  mojom::AccountIdPtr account_id,
                                  const std::string& address_to,
                                  uint64_t amount,
                                  uint64_t fee,
                                  SendToCallback callback) {
  if (!IsValidBitcoinNetworkKeyringPair(network_id, account_id->keyring_id)) {
    NOTREACHED();
    std::move(callback).Run("", "Invalid (network_id,keyring_id) pair");
    return;
  }

  auto account_addresses = keyring_service_->GetBitcoinAddresses(*account_id);
  if (!account_addresses || account_addresses->empty()) {
    std::move(callback).Run("", "No bitcoin addresses for account");
    return;
  }

  auto context = std::make_unique<SendToContext>();
  context->network_id = network_id;
  context->account_id = account_id->Clone();
  context->address_to = address_to;
  context->amount = amount;
  context->fee = fee;
  context->addresses = std::move(*account_addresses);
  context->callback = std::move(callback);

  WorkOnSendTo(std::move(context));
}

bool BitcoinWalletService::FillUtxoList(SendToContext& context) {
  context.utxo_list =
      transaction_database_[context.network_id]->GetAllUnspentOutputs();
  return !context.utxo_list.empty();
}

bool BitcoinWalletService::PickInputs(SendToContext& context) {
  context.amount_picked = 0;
  bool done = false;

  // TODO(apaymyshev): This just picks ouputs one by one and stops when picked
  // amount is GE to send amount plus fee. Needs something better than such
  // greedy strategy.
  for (bitcoin::Output& utxo : context.utxo_list) {
    SendToContext::TxInput input;
    input.prev_output = utxo;

    for (const auto& address : context.addresses) {
      if (utxo.scriptpubkey_address == address.first) {
        input.key_id = address.second->Clone();
        break;
      }
    }
    if (!input.key_id) {
      NOTREACHED();
      continue;
    }

    auto pubkey =
        keyring_service_->GetBitcoinPubkey(*context.account_id, *input.key_id);
    if (!pubkey) {
      return false;
    }
    input.pubkey = std::move(*pubkey);

    context.amount_picked += input.prev_output.value;
    if (context.amount_picked >= context.amount + context.fee) {
      done = true;
    }

    context.inputs.push_back(std::move(input));

    if (done) {
      break;
    }
  }

  DCHECK(!context.inputs.empty());
  return done;
}

bool BitcoinWalletService::PrepareOutputs(SendToContext& context) {
  auto& target_output = context.outputs.emplace_back();
  target_output.address = context.address_to;
  target_output.amount = context.amount;
  auto decoded_address =
      DecodeBitcoinAddress(target_output.address, context.IsTestnet());
  // TODO(apaymyshev): support more types
  // TODO(apaymyshev): rewrite as 'convert address to lock script'
  if (decoded_address && decoded_address->address_type ==
                             BitcoinAddressType::kWitnessV0PubkeyHash) {
    target_output.pubkey_hash = std::move(decoded_address->pubkey_hash);
    CHECK_EQ(target_output.pubkey_hash.size(), 20u);

    Push8AsLE(0, target_output.script_pubkey);  // OP_0
    Push8AsLE(target_output.pubkey_hash.size(), target_output.script_pubkey);
    target_output.script_pubkey.insert(target_output.script_pubkey.end(),
                                       target_output.pubkey_hash.begin(),
                                       target_output.pubkey_hash.end());
  } else {
    return false;
  }

  CHECK_GE(context.amount_picked, context.amount + context.fee);
  if (context.amount_picked == context.amount + context.fee) {
    return true;
  }

  // TODO(apaymyshev): should always pick new change address.
  const auto& change_address = GetUnusedChangeAddress(*context.account_id);
  if (!change_address) {
    return false;
  }

  // TODO(apaymyshev): fix copypaste with above.
  auto& change_output = context.outputs.emplace_back();
  change_output.address = *change_address;
  change_output.amount = context.amount_picked - context.amount - context.fee;

  auto decoded_change_address =
      DecodeBitcoinAddress(change_output.address, context.IsTestnet());

  if (decoded_change_address && decoded_change_address->address_type ==
                                    BitcoinAddressType::kWitnessV0PubkeyHash) {
    change_output.pubkey_hash = std::move(decoded_change_address->pubkey_hash);
    CHECK_EQ(change_output.pubkey_hash.size(), 20u);

    Push8AsLE(0, change_output.script_pubkey);   // OP_0
    Push8AsLE(change_output.pubkey_hash.size(),  // OP_14
              change_output.script_pubkey);
    change_output.script_pubkey.insert(change_output.script_pubkey.end(),
                                       change_output.pubkey_hash.begin(),
                                       change_output.pubkey_hash.end());
  } else {
    NOTREACHED();
    return false;
  }

  return true;
}

bool PushHashPrevouts(SendToContext& context, std::vector<uint8_t>& to) {
  std::vector<uint8_t> data;
  for (const auto& input : context.inputs) {
    PushOutpoint(input.prev_output.outpoint, data);
  }

  const auto& double_hash = DoubleSHA256Hash(data);
  to.insert(to.end(), double_hash.begin(), double_hash.end());
  return true;
}

void PushHashSequence(SendToContext& context, std::vector<uint8_t>& to) {
  std::vector<uint8_t> data;
  for (const auto& input : context.inputs) {
    Push32AsLE(input.n_sequence, data);
  }

  const auto& double_hash = DoubleSHA256Hash(data);
  to.insert(to.end(), double_hash.begin(), double_hash.end());
}

void PushScriptCode(SendToContext& context,
                    uint32_t input_index,
                    std::vector<uint8_t>& to) {
  CHECK_LT(input_index, context.inputs.size());

  to.insert(to.end(), {0x19, 0x76, 0xa9, 0x14});
  const auto& pubkey_hash = Hash160(context.inputs[input_index].pubkey);
  to.insert(to.end(), pubkey_hash.begin(), pubkey_hash.end());
  to.insert(to.end(), {0x88, 0xac});
}

void PushHashOutputs(SendToContext& context, std::vector<uint8_t>& to) {
  std::vector<uint8_t> data;
  for (const auto& output : context.outputs) {
    Push64AsLE(output.amount, data);
    PushVarInt(output.script_pubkey.size(), data);
    data.insert(data.end(), output.script_pubkey.begin(),
                output.script_pubkey.end());
  }

  const auto& double_hash = DoubleSHA256Hash(data);
  to.insert(to.end(), double_hash.begin(), double_hash.end());
}

bool BitcoinWalletService::FillSignature(SendToContext& context,
                                         uint32_t input_index) {
  CHECK_LT(input_index, context.inputs.size());

  auto& input = context.inputs[input_index];

  // https://github.com/bitcoin/bips/blob/master/bip-0143.mediawiki#specification
  std::vector<uint8_t> data;
  Push32AsLE(2, data);              // 1.
  PushHashPrevouts(context, data);  // 2.
  PushHashSequence(context, data);  // 3.

  PushOutpoint(input.prev_output.outpoint, data);  // 4
  PushScriptCode(context, input_index, data);      // 5.
  Push64AsLE(input.prev_output.value, data);       // 6.
  Push32AsLE(input.n_sequence, data);              // 7.

  PushHashOutputs(context, data);      // 8.
  Push32AsLE(context.locktime, data);  // 9.
  Push32AsLE(kSigHashAll, data);       // 10.

  if (auto signature = keyring_service_->SignMessageByBitcoinKeyring(
          *context.account_id, *input.key_id, DoubleSHA256Hash(data))) {
    input.signature = std::move(*signature);
  } else {
    return false;
  }

  Push8AsLE(kSigHashAll, input.signature);
  return true;
}

bool BitcoinWalletService::FillSignatures(SendToContext& context) {
  for (uint32_t i = 0; i < context.inputs.size(); ++i) {
    if (!FillSignature(context, i)) {
      return false;
    }
  }

  return true;
}

void PushInputs(SendToContext& context, std::vector<uint8_t>& to) {
  PushVarInt(context.inputs.size(), to);
  for (const auto& input : context.inputs) {
    PushOutpoint(input.prev_output.outpoint, to);
    // TODO(apaymsyhev): support script for non-segwit transactions.
    PushVarInt(0, to);
    Push32AsLE(input.n_sequence, to);
  }
}

void PushOutputs(SendToContext& context, std::vector<uint8_t>& to) {
  PushVarInt(context.outputs.size(), to);
  for (const auto& output : context.outputs) {
    Push64AsLE(output.amount, to);
    PushVarInt(output.script_pubkey.size(), to);
    to.insert(to.end(), output.script_pubkey.begin(),
              output.script_pubkey.end());
  }
}

void PushWitnesses(SendToContext& context, std::vector<uint8_t>& to) {
  for (const auto& input : context.inputs) {
    // TODO(apaymyshev): only supports P2WPKH. Should support everything else.
    PushVarInt(2, to);

    PushVarSizeVector(input.signature, to);
    PushVarSizeVector(input.pubkey, to);
  }
}

// https://github.com/bitcoin/bips/blob/master/bip-0144.mediawiki#specification
bool BitcoinWalletService::SerializeTransaction(SendToContext& context) {
  std::vector<uint8_t> data;
  Push32AsLE(kTransactionsVersion, data);  // version
  Push8AsLE(0, data);                      // marker
  Push8AsLE(1, data);                      // flag
  PushInputs(context, data);
  PushOutputs(context, data);
  PushWitnesses(context, data);
  Push32AsLE(context.locktime, data);

  context.serialized_transaction = std::move(data);

  return true;
}

void BitcoinWalletService::PostTransaction(
    std::unique_ptr<SendToContext> context) {
  auto network_id = context->network_id;
  const auto transaction = context->serialized_transaction;

  bitcoin_rpc_->PostTransaction(
      network_id, transaction,
      base::BindOnce(&BitcoinWalletService::OnPostTransaction,
                     weak_ptr_factory_.GetWeakPtr(), std::move(context)));
}

void BitcoinWalletService::OnPostTransaction(
    std::unique_ptr<SendToContext> context,
    base::expected<std::string, std::string> result) {
  if (!result.has_value()) {
    return context->ReplyFailure(result.error());
  }

  auto txid = result.value();
  std::move(context->callback).Run(txid, "");
}

void BitcoinWalletService::WorkOnSendTo(
    std::unique_ptr<SendToContext> context) {
  DCHECK(context);

  if (context->locktime == 0) {
    auto height = transaction_database_[context->network_id]->GetChainHeight();
    if (!height) {
      context->ReplyFailure("Invalid chain height");
      return;
    }

    // TODO(apymyshev): random shift locktime
    // https://github.com/bitcoin/bitcoin/blob/v24.0/src/wallet/spend.cpp#L739-L747
    context->locktime = height.value();
  }

  if (!context->UtxoListReady()) {
    if (!FillUtxoList(*context)) {
      return context->ReplyFailure("No outputs to spend");
    }
  }

  if (!context->InputsReady()) {
    if (!PickInputs(*context)) {
      return context->ReplyFailure("Couldn't pick transaction inputs");
    }
  }

  if (!context->OutputsReady()) {
    if (!PrepareOutputs(*context)) {
      return context->ReplyFailure("Couldn't prepare outputs");
    }
  }

  if (!context->SignaturesReady()) {
    if (!FillSignatures(*context)) {
      return context->ReplyFailure("Couldn't fill signatures");
    }
  }

  if (!context->IsTransactionSerialized()) {
    if (!SerializeTransaction(*context)) {
      return context->ReplyFailure("Couldn't serizalize transaction");
    }
  }

  PostTransaction(std::move(context));
}

void BitcoinWalletService::StartDatabaseSynchronizer(
    const std::string& network_id,
    const mojom::AccountId& account_id) {
  CHECK(IsBitcoinAccount(account_id));
  CHECK(IsValidBitcoinNetworkKeyringPair(network_id, account_id.keyring_id));
  // TODO(apaymyshev): init address history from persistent storage.

  auto addresses = keyring_service_->GetBitcoinAddresses(account_id);
  if (!addresses) {
    NOTREACHED();
    return;
  }

  std::vector<std::string> addresses_to_watch;
  for (auto& a : addresses.value()) {
    addresses_to_watch.push_back(a.first);
  }

  database_synchronizer_[network_id]->Start(addresses_to_watch);
}

absl::optional<std::string> BitcoinWalletService::GetUnusedChangeAddress(
    const mojom::AccountId& account_id) {
  CHECK(IsBitcoinAccount(account_id));
  // TODO(apaymyshev): this always returns first change address. Should return
  // first unused change address.
  return keyring_service_->GetBitcoinAddress(
      account_id, mojom::BitcoinKeyId(account_id.bitcoin_account_index, 1, 0));
}

void BitcoinWalletService::AccountsAdded(
    std::vector<mojom::AccountInfoPtr> accounts) {
  // TODO(apaymyshev): need keyring_id here.
}

}  // namespace brave_wallet
