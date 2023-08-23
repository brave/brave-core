/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_wallet_service.h"

#include <stdint.h>
#include <deque>
#include <map>
#include <set>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/memory/scoped_refptr.h"
#include "base/notreached.h"
#include "base/strings/string_number_conversions.h"
#include "base/types/expected.h"
#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_serializer.h"
#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_transaction.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/common/bitcoin_utils.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace brave_wallet {

namespace {

bool OutputAddressSupported(const std::string& address, bool is_testnet) {
  auto decoded_address = DecodeBitcoinAddress(address);
  if (!decoded_address) {
    return false;
  }
  if (decoded_address->testnet != is_testnet) {
    return false;
  }
  // Only segwit outputs are supported by now.
  // TODO(apaymyshev): support more types
  return decoded_address->address_type ==
         BitcoinAddressType::kWitnessV0PubkeyHash;
}

uint64_t GetChainBalance(const bitcoin_rpc::AddressChainStats& chain_stats) {
  uint64_t funded = 0;
  if (!base::StringToUint64(chain_stats.funded_txo_sum, &funded)) {
    return 0;
  }
  uint64_t spent = 0;
  if (!base::StringToUint64(chain_stats.spent_txo_sum, &spent)) {
    return 0;
  }

  return base::ClampSub(funded, spent);
}

}  // namespace

class GetBalanceContext : public base::RefCountedThreadSafe<GetBalanceContext> {
 public:
  using GetBalanceCallback = mojom::BitcoinWalletService::GetBalanceCallback;

  std::set<std::string> addresses;
  std::map<std::string, uint64_t> balances;
  absl::optional<std::string> error;
  GetBalanceCallback callback;

  bool ShouldRespond() { return callback && (error || addresses.empty()); }

  void SetError(const std::string& error_string) { error = error_string; }

 protected:
  friend class base::RefCountedThreadSafe<GetBalanceContext>;
  virtual ~GetBalanceContext() = default;
};

class GetUtxosContext : public base::RefCountedThreadSafe<GetUtxosContext> {
 public:
  using GetUtxosCallback =
      base::OnceCallback<void(BitcoinWalletService::UtxoMap utxos,
                              const absl::optional<std::string>& error)>;

  std::set<std::string> addresses;
  BitcoinWalletService::UtxoMap utxos;
  absl::optional<std::string> error;
  BitcoinWalletService::GetUtxosCallback callback;

  bool ShouldRespond() { return callback && (error || addresses.empty()); }

  void SetError(const std::string& error_string) { error = error_string; }

 protected:
  friend class base::RefCountedThreadSafe<GetUtxosContext>;
  virtual ~GetUtxosContext() = default;
};

class CreateTransactionTask {
 public:
  using UtxoMap = BitcoinWalletService::UtxoMap;
  using CreateTransactionCallback =
      BitcoinWalletService::CreateTransactionCallback;

  CreateTransactionTask(BitcoinWalletService* bitcoin_wallet_service,
                        const std::string& chain_id,
                        const mojom::AccountIdPtr& account_id,
                        const std::string& address_to,
                        uint64_t amount,
                        CreateTransactionCallback callback);

  void ScheduleWorkOnTask();

 private:
  bool IsTestnet() { return chain_id_ == mojom::kBitcoinTestnet; }
  void WorkOnTask();

  void SetError(const std::string& error_string) { error_ = error_string; }

  bool PickInputs();
  bool PrepareOutputs();

  void OnGetChainHeight(base::expected<uint32_t, std::string> chain_height);
  void OnGetUtxos(base::expected<UtxoMap, std::string> utxo_map);

  raw_ptr<BitcoinWalletService> bitcoin_wallet_service_;  // Owns `this`.
  std::string chain_id_;
  mojom::AccountIdPtr account_id_;
  CreateTransactionCallback callback_;

  absl::optional<uint32_t> chain_height_;
  BitcoinWalletService::UtxoMap utxo_map_;

  absl::optional<std::string> error_;
  BitcoinTransaction transaction_;

  base::WeakPtrFactory<CreateTransactionTask> weak_ptr_factory_{this};
};

CreateTransactionTask::CreateTransactionTask(
    BitcoinWalletService* bitcoin_wallet_service,
    const std::string& chain_id,
    const mojom::AccountIdPtr& account_id,
    const std::string& address_to,
    uint64_t amount,
    CreateTransactionCallback callback)
    : bitcoin_wallet_service_(bitcoin_wallet_service),
      chain_id_(chain_id),
      account_id_(account_id.Clone()),
      callback_(std::move(callback)) {
  transaction_.set_to(address_to);
  transaction_.set_amount(amount);
}

void CreateTransactionTask::ScheduleWorkOnTask() {
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(&CreateTransactionTask::WorkOnTask,
                                weak_ptr_factory_.GetWeakPtr()));
}

void CreateTransactionTask::WorkOnTask() {
  if (!callback_) {
    return;
  }

  if (error_) {
    std::move(callback_).Run(base::unexpected(*error_));
    bitcoin_wallet_service_->CreateTransactionTaskDone(this);
    return;
  }

  if (!chain_height_) {
    bitcoin_wallet_service_->bitcoin_rpc().GetChainHeight(
        chain_id_, base::BindOnce(&CreateTransactionTask::OnGetChainHeight,
                                  weak_ptr_factory_.GetWeakPtr()));
    return;
  }

  if (utxo_map_.empty()) {
    bitcoin_wallet_service_->GetUtxos(
        chain_id_, account_id_.Clone(),
        base::BindOnce(&CreateTransactionTask::OnGetUtxos,
                       weak_ptr_factory_.GetWeakPtr()));
    return;
  }

  // TODO(apaymyshev): fetch fee estimations, calculate transaction size and set
  // transaction fee
  transaction_.set_fee(2000);

  // TODO(apaymyshev): random shift locktime
  // https://github.com/bitcoin/bitcoin/blob/v24.0/src/wallet/spend.cpp#L739-L747
  transaction_.set_locktime(chain_height_.value());

  if (!PickInputs()) {
    SetError("Couldn't pick transaction inputs");
    ScheduleWorkOnTask();
    return;
  }

  if (!PrepareOutputs()) {
    SetError("Couldn't prepare outputs");
    ScheduleWorkOnTask();
    return;
  }

  std::move(callback_).Run(base::ok(std::move(transaction_)));
  bitcoin_wallet_service_->CreateTransactionTaskDone(this);
}

void CreateTransactionTask::OnGetChainHeight(
    base::expected<uint32_t, std::string> chain_height) {
  if (!chain_height.has_value()) {
    SetError(std::move(chain_height).error());
    WorkOnTask();
    return;
  }

  chain_height_ = chain_height.value();
  WorkOnTask();
}

void CreateTransactionTask::OnGetUtxos(
    base::expected<UtxoMap, std::string> utxo_map) {
  if (!utxo_map.has_value()) {
    SetError(std::move(utxo_map).error());
    WorkOnTask();
    return;
  }

  utxo_map_ = std::move(utxo_map.value());
  WorkOnTask();
}

bool CreateTransactionTask::PickInputs() {
  bool done = false;

  // TODO(apaymyshev): This just picks ouputs one by one and stops when picked
  // amount is GE to send amount plus fee. Needs something better than such
  // greedy strategy.
  std::vector<BitcoinTransaction::TxInput> all_inputs;
  for (const auto& item : utxo_map_) {
    for (const auto& utxo : item.second) {
      if (auto input =
              BitcoinTransaction::TxInput::FromRpcUtxo(item.first, utxo)) {
        all_inputs.emplace_back(std::move(*input));
      }
    }
  }

  base::ranges::sort(all_inputs, [](auto& input1, auto& input2) {
    return input1.utxo_value < input2.utxo_value;
  });

  for (auto& input : all_inputs) {
    transaction_.inputs().push_back(std::move(input));
    if (transaction_.TotalInputsAmount() >=
        transaction_.amount() + transaction_.fee()) {
      done = true;
    }

    if (done) {
      break;
    }
  }

  DCHECK(!transaction_.inputs().empty());
  return done;
}

bool CreateTransactionTask::PrepareOutputs() {
  auto& target_output = transaction_.outputs().emplace_back();
  target_output.address = transaction_.to();
  target_output.amount = transaction_.amount();
  if (!OutputAddressSupported(target_output.address, IsTestnet())) {
    return false;
  }

  CHECK_GE(transaction_.TotalInputsAmount(),
           transaction_.amount() + transaction_.fee());
  uint64_t change_amount = transaction_.TotalInputsAmount() -
                           transaction_.amount() - transaction_.fee();
  if (change_amount == 0) {
    return true;
  }

  // TODO(apaymyshev): should always pick new change address.
  const auto& change_address =
      bitcoin_wallet_service_->GetUnusedChangeAddress(*account_id_);
  if (!change_address) {
    return false;
  }
  CHECK(OutputAddressSupported(*change_address, IsTestnet()));
  auto& change_output = transaction_.outputs().emplace_back();
  change_output.address = *change_address;
  change_output.amount = change_amount;

  return true;
}

BitcoinWalletService::BitcoinWalletService(
    KeyringService* keyring_service,
    PrefService* prefs,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : keyring_service_(keyring_service),
      bitcoin_rpc_(
          std::make_unique<bitcoin_rpc::BitcoinRpc>(prefs,
                                                    url_loader_factory)) {}

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

void BitcoinWalletService::GetBalance(const std::string& chain_id,
                                      mojom::AccountIdPtr account_id,
                                      GetBalanceCallback callback) {
  if (!IsBitcoinNetwork(chain_id)) {
    // Desktop frontend sometimes does that.
    std::move(callback).Run(nullptr, "Invalid bitcoin chain id " + chain_id);
    return;
  }

  const auto& addresses = keyring_service_->GetBitcoinAddresses(*account_id);
  if (!addresses) {
    std::move(callback).Run(nullptr, "Couldn't get balance");
    return;
  }

  auto context = base::MakeRefCounted<GetBalanceContext>();
  context->callback = std::move(callback);
  for (const auto& address : addresses.value()) {
    context->addresses.insert(address.first);
  }

  for (const auto& address : addresses.value()) {
    bitcoin_rpc_->GetAddressStats(
        chain_id, address.first,
        base::BindOnce(&BitcoinWalletService::OnGetAddressStatsForBalance,
                       weak_ptr_factory_.GetWeakPtr(), context, address.first));
  }
}

void BitcoinWalletService::GetBitcoinAccountInfo(
    const std::string& chain_id,
    mojom::AccountIdPtr account_id,
    GetBitcoinAccountInfoCallback callback) {
  std::move(callback).Run(
      GetBitcoinAccountInfoSync(chain_id, std::move(account_id)));
}

mojom::BitcoinAccountInfoPtr BitcoinWalletService::GetBitcoinAccountInfoSync(
    const std::string& chain_id,
    mojom::AccountIdPtr account_id) {
  if (!IsValidBitcoinNetworkKeyringPair(chain_id, account_id->keyring_id)) {
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

    // TODO(apaymyshev): fill utxo list in case we need GetBitcoinAccountInfo.
  }

  return bitcoin_account_info;
}

void BitcoinWalletService::SendTo(const std::string& chain_id,
                                  mojom::AccountIdPtr account_id,
                                  const std::string& address_to,
                                  uint64_t amount,
                                  uint64_t fee,
                                  SendToCallback callback) {
  std::move(callback).Run("", "Not implemented");
}

void BitcoinWalletService::OnGetAddressStatsForBalance(
    scoped_refptr<GetBalanceContext> context,
    std::string address,
    base::expected<bitcoin_rpc::AddressStats, std::string> stats) {
  DCHECK(context->addresses.contains(address));
  DCHECK(!context->balances.contains(address));

  if (!stats.has_value()) {
    context->SetError(stats.error());
    WorkOnGetBalance(std::move(context));
    return;
  }

  context->addresses.erase(address);
  auto chain_balance = GetChainBalance(stats->chain_stats);
  // TODO(apaymyshev): should show only confirmed balance?
  auto mempool_balance = GetChainBalance(stats->mempool_stats);
  context->balances[address] = chain_balance + mempool_balance;
  WorkOnGetBalance(std::move(context));
}

void BitcoinWalletService::WorkOnGetBalance(
    scoped_refptr<GetBalanceContext> context) {
  if (!context->ShouldRespond()) {
    return;
  }

  if (context->error) {
    std::move(context->callback).Run(nullptr, std::move(*context->error));
    return;
  }

  auto result = mojom::BitcoinBalance::New();
  for (auto& balance : context->balances) {
    result->total_balance += balance.second;
  }
  result->balances.insert(context->balances.begin(), context->balances.end());
  std::move(context->callback).Run(std::move(result), absl::nullopt);
}

absl::optional<std::string> BitcoinWalletService::GetUnusedChangeAddress(
    const mojom::AccountId& account_id) {
  CHECK(IsBitcoinAccount(account_id));
  // TODO(apaymyshev): this always returns first change address. Should return
  // first unused change address.
  return keyring_service_->GetBitcoinAddress(
      account_id, mojom::BitcoinKeyId(account_id.bitcoin_account_index, 1, 0));
}

void BitcoinWalletService::GetUtxos(const std::string& chain_id,
                                    mojom::AccountIdPtr account_id,
                                    GetUtxosCallback callback) {
  const auto& addresses = keyring_service_->GetBitcoinAddresses(*account_id);
  if (!addresses) {
    NOTREACHED();
    std::move(callback).Run(base::unexpected("Couldn't get balance"));
    return;
  }

  auto context = base::MakeRefCounted<GetUtxosContext>();
  context->callback = std::move(callback);
  for (const auto& address : addresses.value()) {
    context->addresses.insert(address.first);
  }

  for (const auto& address : addresses.value()) {
    bitcoin_rpc_->GetUtxoList(
        chain_id, address.first,
        base::BindOnce(&BitcoinWalletService::OnGetUtxos,
                       weak_ptr_factory_.GetWeakPtr(), context, address.first));
  }
}

void BitcoinWalletService::OnGetUtxos(
    scoped_refptr<GetUtxosContext> context,
    std::string address,
    base::expected<std::vector<bitcoin_rpc::UnspentOutput>, std::string>
        utxos) {
  if (!utxos.has_value()) {
    context->SetError(utxos.error());
    WorkOnGetUtxos(std::move(context));
    return;
  }

  context->addresses.erase(address);
  context->utxos[address] = std::move(utxos.value());
  WorkOnGetUtxos(std::move(context));
}

void BitcoinWalletService::WorkOnGetUtxos(
    scoped_refptr<GetUtxosContext> context) {
  if (!context->ShouldRespond()) {
    return;
  }

  if (context->error) {
    std::move(context->callback).Run(base::unexpected(*context->error));
    return;
  }

  std::move(context->callback).Run(base::ok(std::move(context->utxos)));
}

void BitcoinWalletService::CreateTransaction(
    const std::string& chain_id,
    mojom::AccountIdPtr account_id,
    const std::string& address_to,
    uint64_t amount,
    CreateTransactionCallback callback) {
  auto& task = create_transaction_tasks_.emplace_back(
      std::make_unique<CreateTransactionTask>(
          this, chain_id, account_id, address_to, amount, std::move(callback)));
  task->ScheduleWorkOnTask();
}

void BitcoinWalletService::CreateTransactionTaskDone(
    CreateTransactionTask* task) {
  CHECK(create_transaction_tasks_.remove_if(
      [task](auto& item) { return item.get() == task; }));
}

void BitcoinWalletService::SignAndPostTransaction(
    const std::string& chain_id,
    const mojom::AccountIdPtr& account_id,
    BitcoinTransaction bitcoin_transaction,
    SignAndPostTransactionCallback callback) {
  if (!SignTransactionInternal(bitcoin_transaction, account_id)) {
    std::move(callback).Run("", std::move(bitcoin_transaction),
                            "Couldn't sign transaciton");
    return;
  }

  auto serialized_transaction =
      BitcoinSerializer::SerializeSignedTransaction(bitcoin_transaction);

  bitcoin_rpc_->PostTransaction(
      chain_id, serialized_transaction,
      base::BindOnce(&BitcoinWalletService::OnPostTransaction,
                     weak_ptr_factory_.GetWeakPtr(),
                     std::move(bitcoin_transaction), std::move(callback)));
}

void BitcoinWalletService::OnPostTransaction(
    BitcoinTransaction bitcoin_transaction,
    SignAndPostTransactionCallback callback,
    base::expected<std::string, std::string> txid) {
  if (!txid.has_value()) {
    std::move(callback).Run("", std::move(bitcoin_transaction), txid.error());
    return;
  }

  std::move(callback).Run(txid.value(), std::move(bitcoin_transaction), "");
}

void BitcoinWalletService::GetTransactionStatus(
    const std::string& chain_id,
    const std::string& txid,
    GetTransactionStatusCallback callback) {
  bitcoin_rpc_->GetTransaction(
      chain_id, txid,
      base::BindOnce(&BitcoinWalletService::OnGetTransaction,
                     weak_ptr_factory_.GetWeakPtr(), txid,
                     std::move(callback)));
}

void BitcoinWalletService::OnGetTransaction(
    const std::string& txid,
    GetTransactionStatusCallback callback,
    base::expected<bitcoin_rpc::Transaction, std::string> transaction) {
  if (!transaction.has_value()) {
    std::move(callback).Run(base::unexpected(transaction.error()));
    return;
  }

  if (transaction.value().txid != txid) {
    std::move(callback).Run(base::unexpected("Invalid txid"));
    return;
  }

  std::move(callback).Run(base::ok(transaction.value().status.confirmed));
}

bool BitcoinWalletService::SignTransactionInternal(
    BitcoinTransaction& tx,
    const mojom::AccountIdPtr& account_id) {
  auto addresses = keyring_service_->GetBitcoinAddresses(*account_id);
  if (!addresses || addresses->empty()) {
    return false;
  }

  std::map<std::string, mojom::BitcoinKeyIdPtr> address_map;
  for (auto& addr : *addresses) {
    address_map.emplace(std::move(addr));
  }

  for (size_t input_index = 0; input_index < tx.inputs().size();
       ++input_index) {
    auto hash = BitcoinSerializer::SerializeInputForSign(tx, input_index);
    if (!hash) {
      return false;
    }
    auto& input = tx.inputs()[input_index];

    if (!address_map.contains(input.utxo_address)) {
      return false;
    }
    auto& key_id = address_map.at(input.utxo_address);

    auto signature = keyring_service_->SignMessageByBitcoinKeyring(
        *account_id, *key_id, *hash);
    if (!signature) {
      return false;
    }
    signature->push_back(tx.sighash_type());

    auto pubkey = keyring_service_->GetBitcoinPubkey(*account_id, *key_id);
    if (!pubkey) {
      return false;
    }
    input.witness = BitcoinSerializer::SerializeWitness(*signature, *pubkey);
  }

  return true;
}

}  // namespace brave_wallet
