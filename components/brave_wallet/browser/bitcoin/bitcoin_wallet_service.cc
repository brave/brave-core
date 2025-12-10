/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_wallet_service.h"

#include <stdint.h>

#include <algorithm>
#include <map>
#include <optional>
#include <utility>
#include <vector>

#include "base/check.h"
#include "base/check_op.h"
#include "base/functional/bind.h"
#include "base/memory/scoped_refptr.h"
#include "base/notreached.h"
#include "base/numerics/checked_math.h"
#include "base/rand_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/types/expected.h"
#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_fetch_raw_transactions_task.h"
#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_knapsack_solver.h"
#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_max_send_solver.h"
#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_serializer.h"
#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_task_utils.h"
#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_transaction.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/common/bitcoin_utils.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "brave/components/brave_wallet/common/hash_utils.h"
#include "components/grit/brave_components_strings.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"
#include "ui/base/l10n/l10n_util.h"

namespace brave_wallet {

namespace {

const uint32_t kHighPriorityTargetBlock = 1;
const uint32_t kMediumPriorityTargetBlock = 4;
const double kFallbackMainnetFeeRate = 20;  // 20 sat per byte fallback rate.
const double kFallbackTestnetFeeRate = 1;   // 1 sat per byte fallback rate.
const double kDustRelayFeeRate = 3;         // 3 sat per byte rate.

std::vector<BitcoinTransaction::TxInputGroup> TxInputGroupsFromUtxoMap(
    const BitcoinWalletService::UtxoMap& utxo_map) {
  std::vector<BitcoinTransaction::TxInputGroup> groups;
  for (const auto& item : utxo_map) {
    if (item.second.empty()) {
      continue;
    }

    auto& group = groups.emplace_back();
    for (const auto& utxo : item.second) {
      if (auto input =
              BitcoinTransaction::TxInput::FromRpcUtxo(item.first, utxo)) {
        group.AddInput(std::move(*input));
      }
    }
  }

  return groups;
}

std::string MakeHwPath(const mojom::AccountIdPtr& account,
                       const mojom::BitcoinKeyIdPtr& key_id) {
  CHECK(IsBitcoinHardwareKeyring(account->keyring_id));
  if (account->keyring_id == mojom::KeyringId::kBitcoinHardware) {
    return absl::StrFormat("84'/0'/%d'/%d/%d", account->account_index,
                           key_id->change, key_id->index);
  } else if (account->keyring_id == mojom::KeyringId::kBitcoinHardwareTestnet) {
    return absl::StrFormat("84'/1'/%d'/%d/%d", account->account_index,
                           key_id->change, key_id->index);
  }
  NOTREACHED();
}
}  // namespace

class GetBalanceTask {
 public:
  using Callback = mojom::BitcoinWalletService::GetBalanceCallback;

  GetBalanceTask(BitcoinWalletService& bitcoin_wallet_service,
                 const std::string& chain_id,
                 std::vector<mojom::BitcoinAddressPtr> addresses);
  ~GetBalanceTask() = default;

  void Start(Callback callback);

 private:
  void MaybeSendRequests();
  void OnGetAddressStats(
      mojom::BitcoinAddressPtr address,
      base::expected<bitcoin_rpc::AddressStats, std::string> stats);
  void ScheduleWorkOnTask();
  void WorkOnTask();

  const raw_ref<BitcoinWalletService> bitcoin_wallet_service_;  // Owns `this`.
  std::string chain_id_;
  std::vector<mojom::BitcoinAddressPtr> addresses_;
  bool requests_sent_ = false;

  std::map<std::string, uint64_t> balances_;

  std::optional<std::string> error_;
  mojom::BitcoinBalancePtr current_balance_;
  mojom::BitcoinBalancePtr result_;
  Callback callback_;

  base::WeakPtrFactory<GetBalanceTask> weak_ptr_factory_{this};
};

GetBalanceTask::GetBalanceTask(BitcoinWalletService& bitcoin_wallet_service,
                               const std::string& chain_id,
                               std::vector<mojom::BitcoinAddressPtr> addresses)
    : bitcoin_wallet_service_(bitcoin_wallet_service),
      chain_id_(chain_id),
      addresses_(std::move(addresses)),
      current_balance_(mojom::BitcoinBalance::New()) {}

void GetBalanceTask::Start(Callback callback) {
  DCHECK(!callback_);
  callback_ = std::move(callback);
  ScheduleWorkOnTask();
}

void GetBalanceTask::ScheduleWorkOnTask() {
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(&GetBalanceTask::WorkOnTask,
                                weak_ptr_factory_.GetWeakPtr()));
}

void GetBalanceTask::MaybeSendRequests() {
  if (requests_sent_) {
    return;
  }
  requests_sent_ = true;

  if (addresses_.empty()) {
    result_ = mojom::BitcoinBalance::New();
    ScheduleWorkOnTask();
    return;
  }

  // Shuffle addresses so requests are always done in different order to
  // increase privacy a bit.
  base::RandomShuffle(addresses_.begin(), addresses_.end());

  for (const auto& address_info : addresses_) {
    bitcoin_wallet_service_->bitcoin_rpc().GetAddressStats(
        chain_id_, address_info->address_string,
        base::BindOnce(&GetBalanceTask::OnGetAddressStats,
                       weak_ptr_factory_.GetWeakPtr(), address_info->Clone()));
  }
}

void GetBalanceTask::WorkOnTask() {
  if (!callback_) {
    return;
  }

  if (error_) {
    std::move(callback_).Run(nullptr, std::move(*error_));
    return;
  }

  if (result_) {
    std::move(callback_).Run(std::move(result_), std::nullopt);
    return;
  }

  MaybeSendRequests();
}

void GetBalanceTask::OnGetAddressStats(
    mojom::BitcoinAddressPtr address,
    base::expected<bitcoin_rpc::AddressStats, std::string> stats) {
  if (!stats.has_value()) {
    error_ = stats.error();
    WorkOnTask();
    return;
  }

  UpdateBalance(current_balance_, *stats);

  CHECK(std::erase(addresses_, address));
  if (addresses_.empty()) {
    result_ = std::move(current_balance_);
  }

  WorkOnTask();
}

class GetUtxosTask {
 public:
  using Callback = BitcoinWalletService::GetUtxosCallback;

  GetUtxosTask(BitcoinWalletService& bitcoin_wallet_service,
               const std::string& chain_id,
               std::vector<mojom::BitcoinAddressPtr> addresses);
  ~GetUtxosTask() = default;

  void Start(Callback callback);

 private:
  void ScheduleWorkOnTask();
  void WorkOnTask();
  void MaybeSendRequests();
  void OnGetUtxoList(
      mojom::BitcoinAddressPtr address,
      base::expected<bitcoin_rpc::UnspentOutputs, std::string> utxos);

  const raw_ref<BitcoinWalletService> bitcoin_wallet_service_;  // Owns `this`.
  std::string chain_id_;
  std::vector<mojom::BitcoinAddressPtr> addresses_;
  bool requests_sent_ = false;

  BitcoinWalletService::UtxoMap utxos_;
  std::optional<std::string> error_;
  std::optional<BitcoinWalletService::UtxoMap> result_;
  Callback callback_;

  base::WeakPtrFactory<GetUtxosTask> weak_ptr_factory_{this};
};

GetUtxosTask::GetUtxosTask(BitcoinWalletService& bitcoin_wallet_service,
                           const std::string& chain_id,
                           std::vector<mojom::BitcoinAddressPtr> addresses)
    : bitcoin_wallet_service_(bitcoin_wallet_service),
      chain_id_(chain_id),
      addresses_(std::move(addresses)) {}

void GetUtxosTask::Start(Callback callback) {
  DCHECK(!callback_);
  callback_ = std::move(callback);
  ScheduleWorkOnTask();
}

void GetUtxosTask::ScheduleWorkOnTask() {
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(&GetUtxosTask::WorkOnTask,
                                weak_ptr_factory_.GetWeakPtr()));
}

void GetUtxosTask::MaybeSendRequests() {
  if (requests_sent_) {
    return;
  }
  requests_sent_ = true;

  if (addresses_.empty()) {
    result_ = BitcoinWalletService::UtxoMap();
    ScheduleWorkOnTask();
    return;
  }

  // Shuffle addresses so requests are always done in different order to
  // increase privacy a bit.
  base::RandomShuffle(addresses_.begin(), addresses_.end());

  for (const auto& address_info : addresses_) {
    bitcoin_wallet_service_->bitcoin_rpc().GetUtxoList(
        chain_id_, address_info->address_string,
        base::BindOnce(&GetUtxosTask::OnGetUtxoList,
                       weak_ptr_factory_.GetWeakPtr(), address_info->Clone()));
  }
}

void GetUtxosTask::OnGetUtxoList(
    mojom::BitcoinAddressPtr address,
    base::expected<bitcoin_rpc::UnspentOutputs, std::string> utxos) {
  if (!utxos.has_value()) {
    error_ = utxos.error();
    return WorkOnTask();
  }

  utxos_[address->address_string] = std::move(utxos.value());

  CHECK(std::erase(addresses_, address));
  if (addresses_.empty()) {
    result_ = std::move(utxos_);
  }

  WorkOnTask();
}

void GetUtxosTask::WorkOnTask() {
  if (!callback_) {
    return;
  }

  if (error_) {
    std::move(callback_).Run(base::unexpected(std::move(*error_)));
    return;
  }

  if (result_) {
    std::move(callback_).Run(base::ok(std::move(*result_)));
    return;
  }

  MaybeSendRequests();
}

class CreateTransactionTask {
 public:
  using UtxoMap = BitcoinWalletService::UtxoMap;
  using Callback = BitcoinWalletService::CreateTransactionCallback;

  CreateTransactionTask(BitcoinWalletService& bitcoin_wallet_service,
                        const mojom::AccountIdPtr& account_id,
                        const std::string& address_to,
                        uint64_t amount,
                        bool sending_max_amount);

  void Start(Callback callback);

  void SetArrangeTransactionForTesting();  // IN-TEST

 private:
  bool IsTestnet() { return IsBitcoinTestnetKeyring(account_id_->keyring_id); }
  void ScheduleWorkOnTask();
  void WorkOnTask();

  void SetError(const std::string& error_string) { error_ = error_string; }

  double GetFeeRate();
  double GetLongtermFeeRate();
  bool ShouldFetchRawTransactions();

  BitcoinTransaction::TxOutput CreateTargetOutput();
  BitcoinTransaction::TxOutput CreateChangeOutput();

  void OnGetChainHeight(base::expected<uint32_t, std::string> chain_height);
  void OnGetFeeEstimates(
      base::expected<std::map<uint32_t, double>, std::string> estimates);
  void OnGetUtxos(base::expected<UtxoMap, std::string> utxo_map);
  void OnDiscoverNextUnusedChangeAddress(
      base::expected<mojom::BitcoinAddressPtr, std::string> address);
  void OnFetchRawTransactions(base::expected<std::vector<std::vector<uint8_t>>,
                                             std::string> raw_transactions);

  const raw_ref<BitcoinWalletService> bitcoin_wallet_service_;  // Owns `this`.
  mojom::AccountIdPtr account_id_;
  Callback callback_;

  std::optional<uint32_t> chain_height_;
  BitcoinWalletService::UtxoMap utxo_map_;
  mojom::BitcoinAddressPtr change_address_;
  std::map<uint32_t, double> estimates_;  // target block -> fee rate (sat/byte)

  std::optional<std::string> error_;
  BitcoinTransaction transaction_;

  bool has_solved_transaction_ = false;
  bool raw_transactions_done_ = false;

  bool arrange_for_testing_ = false;

  base::WeakPtrFactory<CreateTransactionTask> weak_ptr_factory_{this};
};

CreateTransactionTask::CreateTransactionTask(
    BitcoinWalletService& bitcoin_wallet_service,
    const mojom::AccountIdPtr& account_id,
    const std::string& address_to,
    uint64_t amount,
    bool sending_max_amount)
    : bitcoin_wallet_service_(bitcoin_wallet_service),
      account_id_(account_id.Clone()) {
  transaction_.set_to(address_to);
  transaction_.set_amount(amount);
  transaction_.set_sending_max_amount(sending_max_amount);
}

void CreateTransactionTask::Start(Callback callback) {
  DCHECK(!callback_);
  callback_ = std::move(callback);
  ScheduleWorkOnTask();
}

void CreateTransactionTask::ScheduleWorkOnTask() {
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(&CreateTransactionTask::WorkOnTask,
                                weak_ptr_factory_.GetWeakPtr()));
}

void CreateTransactionTask::SetArrangeTransactionForTesting() {
  arrange_for_testing_ = true;
}

BitcoinTransaction::TxOutput CreateTransactionTask::CreateTargetOutput() {
  // TODO(apaymyshev): should fail if target output would be dust.

  BitcoinTransaction::TxOutput target_output;
  target_output.type = BitcoinTransaction::TxOutputType::kTarget;
  target_output.amount =
      transaction_.sending_max_amount() ? 0 : transaction_.amount();
  target_output.address = transaction_.to();
  target_output.script_pubkey = BitcoinSerializer::AddressToScriptPubkey(
      target_output.address, IsTestnet());
  CHECK(target_output.script_pubkey.size());

  return target_output;
}

BitcoinTransaction::TxOutput CreateTransactionTask::CreateChangeOutput() {
  BitcoinTransaction::TxOutput change_output;
  change_output.type = BitcoinTransaction::TxOutputType::kChange;
  change_output.amount = 0;
  change_output.address = change_address_->address_string;
  change_output.script_pubkey = BitcoinSerializer::AddressToScriptPubkey(
      change_output.address, IsTestnet());
  CHECK(change_output.script_pubkey.size());

  return change_output;
}

void CreateTransactionTask::WorkOnTask() {
  if (!callback_) {
    return;
  }

  if (error_) {
    std::move(callback_).Run(base::unexpected(*error_));
    return;
  }

  if (BitcoinSerializer::AddressToScriptPubkey(transaction_.to(), IsTestnet())
          .empty()) {
    SetError(WalletParsingErrorMessage());
    ScheduleWorkOnTask();
    return;
  }

  if (!transaction_.sending_max_amount()) {
    BitcoinTransaction::TxOutput target_output;
    target_output.type = BitcoinTransaction::TxOutputType::kTarget;
    target_output.amount = transaction_.amount();
    target_output.address = transaction_.to();
    target_output.script_pubkey = BitcoinSerializer::AddressToScriptPubkey(
        target_output.address, IsTestnet());
    const uint32_t target_vbytes =
        BitcoinSerializer::CalcOutputVBytesInTransaction(target_output);
    base::CheckedNumeric<uint64_t> dust_threshold =
        ApplyFeeRate(kDustRelayFeeRate, target_vbytes);
    if (!dust_threshold.IsValid()) {
      SetError(WalletInternalErrorMessage());
      ScheduleWorkOnTask();
      return;
    }
    if (transaction_.amount() < dust_threshold.ValueOrDie()) {
      SetError(WalletAmountTooSmallErrorMessage());
      ScheduleWorkOnTask();
      return;
    }
  }

  if (!chain_height_) {
    bitcoin_wallet_service_->bitcoin_rpc().GetChainHeight(
        GetNetworkForBitcoinAccount(account_id_),
        base::BindOnce(&CreateTransactionTask::OnGetChainHeight,
                       weak_ptr_factory_.GetWeakPtr()));
    return;
  }

  if (utxo_map_.empty()) {
    bitcoin_wallet_service_->GetUtxos(
        account_id_.Clone(), base::BindOnce(&CreateTransactionTask::OnGetUtxos,
                                            weak_ptr_factory_.GetWeakPtr()));
    return;
  }

  if (!change_address_) {
    bitcoin_wallet_service_->DiscoverNextUnusedAddress(
        account_id_.Clone(), true,
        base::BindOnce(
            &CreateTransactionTask::OnDiscoverNextUnusedChangeAddress,
            weak_ptr_factory_.GetWeakPtr()));
    return;
  }

  // TODO(apaymyshev): this captures fee estimates at transaction creation
  // moment and can become outdated when transaction is actually broadcasted
  // to network. Should handle this somehow.
  if (estimates_.empty()) {
    bitcoin_wallet_service_->bitcoin_rpc().GetFeeEstimates(
        GetNetworkForBitcoinAccount(account_id_),
        base::BindOnce(&CreateTransactionTask::OnGetFeeEstimates,
                       weak_ptr_factory_.GetWeakPtr()));
    return;
  }

  // TODO(apaymyshev): random shift locktime
  // https://github.com/bitcoin/bitcoin/blob/v24.0/src/wallet/spend.cpp#L739-L747
  transaction_.set_locktime(chain_height_.value());

  if (!has_solved_transaction_) {
    base::expected<BitcoinTransaction, std::string> solved_transaction;
    if (transaction_.sending_max_amount()) {
      transaction_.AddOutput(CreateTargetOutput());
      BitcoinMaxSendSolver solver(transaction_, GetFeeRate(),
                                  TxInputGroupsFromUtxoMap(utxo_map_));
      solved_transaction = solver.Solve();
    } else {
      transaction_.AddOutput(CreateTargetOutput());
      transaction_.AddOutput(CreateChangeOutput());

      // TODO(apaymyshev): consider moving this calculation to separate thread.
      KnapsackSolver solver(transaction_, GetFeeRate(), GetLongtermFeeRate(),
                            TxInputGroupsFromUtxoMap(utxo_map_));
      solved_transaction = solver.Solve();
    }

    if (!solved_transaction.has_value()) {
      SetError(solved_transaction.error());
      ScheduleWorkOnTask();
      return;
    }

    has_solved_transaction_ = true;
    transaction_ = std::move(*solved_transaction);
    if (arrange_for_testing_) {
      transaction_.ArrangeTransactionForTesting();  // IN-TEST
    } else {
      transaction_.ShuffleTransaction();
    }
  }

  if (ShouldFetchRawTransactions()) {
    std::vector<SHA256HashArray> txids;
    for (auto& input : transaction_.inputs()) {
      txids.push_back(input.utxo_outpoint.txid);
    }
    bitcoin_wallet_service_->FetchRawTransactions(
        GetNetworkForBitcoinAccount(account_id_), txids,
        base::BindOnce(&CreateTransactionTask::OnFetchRawTransactions,
                       weak_ptr_factory_.GetWeakPtr()));
    return;
  }

  std::move(callback_).Run(base::ok(std::move(transaction_)));
}

void CreateTransactionTask::OnGetChainHeight(
    base::expected<uint32_t, std::string> chain_height) {
  if (!chain_height.has_value()) {
    SetError(std::move(chain_height.error()));
    WorkOnTask();
    return;
  }

  chain_height_ = chain_height.value();
  WorkOnTask();
}

void CreateTransactionTask::OnGetFeeEstimates(
    base::expected<std::map<uint32_t, double>, std::string> estimates) {
  if (!estimates.has_value()) {
    SetError(std::move(estimates.error()));
    WorkOnTask();
    return;
  }

  estimates_ = std::move(estimates.value());
  DCHECK(!estimates_.empty());
  WorkOnTask();
}

void CreateTransactionTask::OnGetUtxos(
    base::expected<UtxoMap, std::string> utxo_map) {
  if (!utxo_map.has_value()) {
    SetError(std::move(utxo_map.error()));
    WorkOnTask();
    return;
  }

  utxo_map_ = std::move(utxo_map.value());
  WorkOnTask();
}

void CreateTransactionTask::OnDiscoverNextUnusedChangeAddress(
    base::expected<mojom::BitcoinAddressPtr, std::string> address) {
  if (!address.has_value()) {
    SetError(std::move(address.error()));
    WorkOnTask();
    return;
  }
  DCHECK_EQ(address.value()->key_id->change, kBitcoinChangeIndex);
  bitcoin_wallet_service_->UpdateNextUnusedAddressForAccount(account_id_,
                                                             address.value());
  change_address_ = std::move(address.value());
  WorkOnTask();
}

void CreateTransactionTask::OnFetchRawTransactions(
    base::expected<std::vector<std::vector<uint8_t>>, std::string>
        raw_transactions) {
  if (!raw_transactions.has_value()) {
    SetError(std::move(raw_transactions.error()));
    WorkOnTask();
    return;
  }

  CHECK_EQ(raw_transactions.value().size(), transaction_.inputs().size());
  for (auto i = 0u; i < raw_transactions.value().size(); ++i) {
    transaction_.SetInputRawTransaction(i,
                                        std::move(raw_transactions.value()[i]));
  }
  raw_transactions_done_ = true;
  WorkOnTask();
}

double CreateTransactionTask::GetFeeRate() {
  DCHECK(!estimates_.empty());
  double rate = 0.0;
  if (estimates_.contains(kMediumPriorityTargetBlock)) {
    rate = estimates_.at(kMediumPriorityTargetBlock);
  } else if (estimates_.contains(kHighPriorityTargetBlock)) {
    rate = estimates_.at(kHighPriorityTargetBlock);
  } else {
    rate = IsTestnet() ? kFallbackTestnetFeeRate : kFallbackMainnetFeeRate;
  }
  return std::max(rate, kDustRelayFeeRate);
}

double CreateTransactionTask::GetLongtermFeeRate() {
  DCHECK(!estimates_.empty());
  double result = estimates_.begin()->second;
  for (auto& e : estimates_) {
    result = std::min(result, e.second);
  }
  return std::max(result, kDustRelayFeeRate);
}

bool CreateTransactionTask::ShouldFetchRawTransactions() {
  return IsBitcoinHardwareKeyring(account_id_->keyring_id) &&
         !raw_transactions_done_;
}

class DiscoverNextUnusedAddressTask {
 public:
  using Callback = BitcoinWalletService::DiscoverNextUnusedAddressCallback;

  DiscoverNextUnusedAddressTask(BitcoinWalletService& bitcoin_wallet_service,
                                mojom::AccountIdPtr account_id,
                                mojom::BitcoinAddressPtr start_address);
  ~DiscoverNextUnusedAddressTask() = default;
  void Start(Callback callback);

 private:
  mojom::BitcoinAddressPtr GetNextAddress(
      const mojom::BitcoinAddressPtr& address);

  void ScheduleWorkOnTask();
  void WorkOnTask();
  void OnGetAddressStats(
      base::expected<bitcoin_rpc::AddressStats, std::string> stats);

  const raw_ref<BitcoinWalletService> bitcoin_wallet_service_;  // Owns `this`.
  mojom::AccountIdPtr account_id_;
  mojom::BitcoinAddressPtr start_address_;
  mojom::BitcoinAddressPtr current_address_;
  mojom::BitcoinAddressPtr result_;
  std::optional<std::string> error_;
  Callback callback_;

  base::WeakPtrFactory<DiscoverNextUnusedAddressTask> weak_ptr_factory_{this};
};

DiscoverNextUnusedAddressTask::DiscoverNextUnusedAddressTask(
    BitcoinWalletService& bitcoin_wallet_service,
    mojom::AccountIdPtr account_id,
    mojom::BitcoinAddressPtr start_address)
    : bitcoin_wallet_service_(bitcoin_wallet_service),
      account_id_(std::move(account_id)),
      start_address_(std::move(start_address)) {}

void DiscoverNextUnusedAddressTask::Start(Callback callback) {
  DCHECK(!callback_);
  callback_ = std::move(callback);
  ScheduleWorkOnTask();
}

void DiscoverNextUnusedAddressTask::ScheduleWorkOnTask() {
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(&DiscoverNextUnusedAddressTask::WorkOnTask,
                                weak_ptr_factory_.GetWeakPtr()));
}

mojom::BitcoinAddressPtr DiscoverNextUnusedAddressTask::GetNextAddress(
    const mojom::BitcoinAddressPtr& address) {
  auto next_key_id = current_address_->key_id.Clone();
  next_key_id->index++;

  return bitcoin_wallet_service_->keyring_service().GetBitcoinAddress(
      account_id_, next_key_id);
}

void DiscoverNextUnusedAddressTask::WorkOnTask() {
  if (!callback_) {
    return;
  }

  if (error_) {
    std::move(callback_).Run(base::unexpected(std::move(*error_)));
    return;
  }

  if (result_) {
    std::move(callback_).Run(base::ok(std::move(result_)));
    return;
  }

  if (start_address_) {
    current_address_ = std::move(start_address_);
  } else {
    current_address_ = GetNextAddress(current_address_);
  }

  if (!current_address_) {
    error_ = WalletInternalErrorMessage();
    ScheduleWorkOnTask();
    return;
  }

  bitcoin_wallet_service_->bitcoin_rpc().GetAddressStats(
      GetNetworkForBitcoinAccount(account_id_),
      current_address_->address_string,
      base::BindOnce(&DiscoverNextUnusedAddressTask::OnGetAddressStats,
                     weak_ptr_factory_.GetWeakPtr()));
}

void DiscoverNextUnusedAddressTask::OnGetAddressStats(
    base::expected<bitcoin_rpc::AddressStats, std::string> stats) {
  if (!stats.has_value()) {
    error_ = stats.error();
    WorkOnTask();
    return;
  }

  uint32_t chain_stats_tx_count = 0;
  uint32_t mempool_stats_tx_count = 0;
  if (!base::StringToUint(stats->chain_stats.tx_count, &chain_stats_tx_count) ||
      !base::StringToUint(stats->mempool_stats.tx_count,
                          &mempool_stats_tx_count)) {
    error_ = WalletParsingErrorMessage();
    WorkOnTask();
    return;
  }

  if (chain_stats_tx_count == 0 && mempool_stats_tx_count == 0) {
    result_ = current_address_->Clone();
  }

  WorkOnTask();
}

BitcoinWalletService::BitcoinWalletService(
    KeyringService& keyring_service,
    NetworkManager& network_manager,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : keyring_service_(keyring_service),
      bitcoin_rpc_(network_manager, url_loader_factory) {
  keyring_service_->AddObserver(
      keyring_service_observer_receiver_.BindNewPipeAndPassRemote());
}

BitcoinWalletService::~BitcoinWalletService() = default;

void BitcoinWalletService::Bind(
    mojo::PendingReceiver<mojom::BitcoinWalletService> receiver) {
  receivers_.Add(this, std::move(receiver));
}

void BitcoinWalletService::Reset() {
  weak_ptr_factory_.InvalidateWeakPtrs();
}

void BitcoinWalletService::GetBalance(mojom::AccountIdPtr account_id,
                                      GetBalanceCallback callback) {
  CHECK(IsBitcoinAccount(account_id));

  auto addresses = keyring_service().GetBitcoinAddresses(account_id);
  if (!addresses) {
    std::move(callback).Run(nullptr, WalletInternalErrorMessage());
    return;
  }

  auto [task_it, inserted] =
      get_balance_tasks_.insert(std::make_unique<GetBalanceTask>(
          *this, GetNetworkForBitcoinAccount(account_id),
          std::move(*addresses)));
  CHECK(inserted);
  auto* task_ptr = task_it->get();

  task_ptr->Start(base::BindOnce(&BitcoinWalletService::OnGetBalanceTaskDone,
                                 weak_ptr_factory_.GetWeakPtr(), task_ptr,
                                 std::move(callback)));
}

void BitcoinWalletService::OnGetBalanceTaskDone(
    GetBalanceTask* task,
    GetBalanceCallback callback,
    mojom::BitcoinBalancePtr balance,
    const std::optional<std::string>& error) {
  CHECK(get_balance_tasks_.erase(task));

  std::move(callback).Run(std::move(balance), error);
}

void BitcoinWalletService::GetExtendedKeyAccountBalance(
    const std::string& chain_id,
    const std::string& extended_key,
    GetExtendedKeyAccountBalanceCallback callback) {
  CHECK(IsBitcoinNetwork(chain_id));

  auto [task_it, inserted] = discover_extended_key_account_tasks_.insert(
      std::make_unique<DiscoverExtendedKeyAccountTask>(*this, chain_id,
                                                       extended_key));
  CHECK(inserted);
  auto* task_ptr = task_it->get();

  task_ptr->Start(base::BindOnce(
      &BitcoinWalletService::OnGetExtendedKeyAccountBalanceDone,
      weak_ptr_factory_.GetWeakPtr(), task_ptr, std::move(callback)));
}

void BitcoinWalletService::OnGetExtendedKeyAccountBalanceDone(
    DiscoverExtendedKeyAccountTask* task,
    GetExtendedKeyAccountBalanceCallback callback,
    base::expected<DiscoveredBitcoinAccount, std::string> result) {
  CHECK(discover_extended_key_account_tasks_.erase(task));

  if (!result.has_value()) {
    std::move(callback).Run(nullptr, WalletInternalErrorMessage());
    return;
  }
  std::move(callback).Run(std::move(result.value().balance), std::nullopt);
}

void BitcoinWalletService::GetBitcoinAccountInfo(
    mojom::AccountIdPtr account_id,
    GetBitcoinAccountInfoCallback callback) {
  std::move(callback).Run(GetBitcoinAccountInfoSync(account_id));
}

mojom::BitcoinAccountInfoPtr BitcoinWalletService::GetBitcoinAccountInfoSync(
    const mojom::AccountIdPtr& account_id) {
  return keyring_service().GetBitcoinAccountInfo(account_id);
}

void BitcoinWalletService::RunDiscovery(mojom::AccountIdPtr account_id,
                                        bool change,
                                        RunDiscoveryCallback callback) {
  DiscoverNextUnusedAddress(
      account_id, change,
      base::BindOnce(&BitcoinWalletService::OnRunDiscoveryDone,
                     weak_ptr_factory_.GetWeakPtr(), account_id.Clone(),
                     std::move(callback)));
}

void BitcoinWalletService::AccountsAdded(
    std::vector<mojom::AccountInfoPtr> accounts) {
  for (auto& account : accounts) {
    // For new bitcoin account search for transacted and/or funded addresses.
    if (IsBitcoinKeyring(account->account_id->keyring_id)) {
      auto [task_it, inserted] = discover_wallet_account_tasks_.insert(
          std::make_unique<DiscoverWalletAccountTask>(
              *this, account->account_id->keyring_id,
              account->account_id->account_index));
      CHECK(inserted);
      auto* task_ptr = task_it->get();

      task_ptr->Start(
          base::BindOnce(&BitcoinWalletService::OnAddedAccountDiscoveryDone,
                         weak_ptr_factory_.GetWeakPtr(), task_ptr,
                         account->account_id.Clone()));
    }
  }
}

void BitcoinWalletService::OnAddedAccountDiscoveryDone(
    DiscoverWalletAccountTask* task,
    mojom::AccountIdPtr account_id,
    base::expected<DiscoveredBitcoinAccount, std::string> result) {
  CHECK(discover_wallet_account_tasks_.erase(task));

  if (!result.has_value()) {
    return;
  }
  keyring_service_->UpdateNextUnusedAddressForBitcoinAccount(
      account_id, result.value().next_unused_receive_index,
      result.value().next_unused_change_index);
}

void BitcoinWalletService::OnRunDiscoveryDone(
    mojom::AccountIdPtr account_id,
    RunDiscoveryCallback callback,
    base::expected<mojom::BitcoinAddressPtr, std::string> discovered_address) {
  if (!discovered_address.has_value()) {
    std::move(callback).Run(nullptr, discovered_address.error());
    return;
  }

  UpdateNextUnusedAddressForAccount(account_id, discovered_address.value());
  std::move(callback).Run(discovered_address.value().Clone(), std::nullopt);
}

void BitcoinWalletService::UpdateNextUnusedAddressForAccount(
    const mojom::AccountIdPtr& account_id,
    const mojom::BitcoinAddressPtr& address) {
  std::optional<uint32_t> next_receive_index = address->key_id->change
                                                   ? std::optional<uint32_t>()
                                                   : address->key_id->index;
  std::optional<uint32_t> next_change_index = !address->key_id->change
                                                  ? std::optional<uint32_t>()
                                                  : address->key_id->index;
  keyring_service().UpdateNextUnusedAddressForBitcoinAccount(
      account_id, next_receive_index, next_change_index);
}

void BitcoinWalletService::GetUtxos(mojom::AccountIdPtr account_id,
                                    GetUtxosCallback callback) {
  CHECK(IsBitcoinAccount(account_id));

  auto addresses = keyring_service().GetBitcoinAddresses(account_id);
  if (!addresses) {
    std::move(callback).Run(base::unexpected(WalletInternalErrorMessage()));
    return;
  }

  auto [task_it, inserted] =
      get_utxos_tasks_.insert(std::make_unique<GetUtxosTask>(
          *this, GetNetworkForBitcoinAccount(account_id),
          std::move(*addresses)));
  CHECK(inserted);
  auto* task_ptr = task_it->get();

  task_ptr->Start(base::BindOnce(&BitcoinWalletService::OnGetUtxosTaskDone,
                                 weak_ptr_factory_.GetWeakPtr(), task_ptr,
                                 std::move(callback)));
}

void BitcoinWalletService::OnGetUtxosTaskDone(
    GetUtxosTask* task,
    GetUtxosCallback callback,
    base::expected<UtxoMap, std::string> result) {
  CHECK(get_utxos_tasks_.erase(task));

  std::move(callback).Run(std::move(result));
}

void BitcoinWalletService::CreateTransaction(
    mojom::AccountIdPtr account_id,
    const std::string& address_to,
    uint64_t amount,
    bool sending_max_amount,
    CreateTransactionCallback callback) {
  CHECK(IsBitcoinAccount(account_id));

  auto [task_it, inserted] =
      create_transaction_tasks_.insert(std::make_unique<CreateTransactionTask>(
          *this, account_id, address_to, amount, sending_max_amount));
  CHECK(inserted);
  auto* task_ptr = task_it->get();

  if (arrange_transactions_for_testing_) {
    task_ptr->SetArrangeTransactionForTesting();  // IN-TEST
  }
  task_ptr->Start(base::BindOnce(
      &BitcoinWalletService::OnCreateTransactionTaskDone,
      weak_ptr_factory_.GetWeakPtr(), task_ptr, std::move(callback)));
}

void BitcoinWalletService::OnCreateTransactionTaskDone(
    CreateTransactionTask* task,
    CreateTransactionCallback callback,
    base::expected<BitcoinTransaction, std::string> result) {
  CHECK(create_transaction_tasks_.erase(task));

  std::move(callback).Run(std::move(result));
}

void BitcoinWalletService::SignAndPostTransaction(
    const mojom::AccountIdPtr& account_id,
    BitcoinTransaction bitcoin_transaction,
    SignAndPostTransactionCallback callback) {
  CHECK(IsBitcoinAccount(account_id));

  if (!SignTransactionInternal(bitcoin_transaction, account_id)) {
    std::move(callback).Run("", std::move(bitcoin_transaction),
                            WalletInternalErrorMessage());
    return;
  }

  auto serialized_transaction =
      BitcoinSerializer::SerializeSignedTransaction(bitcoin_transaction);

  bitcoin_rpc_.PostTransaction(
      GetNetworkForBitcoinAccount(account_id), serialized_transaction,
      base::BindOnce(&BitcoinWalletService::OnPostTransaction,
                     weak_ptr_factory_.GetWeakPtr(),
                     std::move(bitcoin_transaction), std::move(callback)));
}

void BitcoinWalletService::PostHwSignedTransaction(
    const mojom::AccountIdPtr& account_id,
    BitcoinTransaction bitcoin_transaction,
    const mojom::BitcoinSignature& hw_signature,
    PostHwSignedTransactionCallback callback) {
  CHECK(IsBitcoinAccount(account_id));

  if (!ApplyHwSignatureInternal(bitcoin_transaction, hw_signature)) {
    std::move(callback).Run("", std::move(bitcoin_transaction),
                            WalletInternalErrorMessage());
    return;
  }

  auto serialized_transaction =
      BitcoinSerializer::SerializeSignedTransaction(bitcoin_transaction);

  bitcoin_rpc_.PostTransaction(
      GetNetworkForBitcoinAccount(account_id), serialized_transaction,
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
  bitcoin_rpc_.GetTransaction(
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
    std::move(callback).Run(base::unexpected(WalletInternalErrorMessage()));
    return;
  }

  std::move(callback).Run(base::ok(transaction.value().status.confirmed));
}

void BitcoinWalletService::FetchRawTransactions(
    const std::string& network_id,
    const std::vector<SHA256HashArray>& txids,
    FetchRawTransactionsCallback callback) {
  auto [task_it, inserted] = fetch_raw_transactions_tasks_.insert(
      std::make_unique<FetchRawTransactionsTask>(*this, network_id, txids));
  CHECK(inserted);
  auto* task_ptr = task_it->get();

  task_ptr->Start(base::BindOnce(
      &BitcoinWalletService::OnFetchRawTransactionsDone,
      weak_ptr_factory_.GetWeakPtr(), task_ptr, std::move(callback)));
}

void BitcoinWalletService::OnFetchRawTransactionsDone(
    FetchRawTransactionsTask* task,
    FetchRawTransactionsCallback callback,
    base::expected<std::vector<std::vector<uint8_t>>, std::string> result) {
  CHECK(fetch_raw_transactions_tasks_.erase(task));

  std::move(callback).Run(std::move(result));
}

void BitcoinWalletService::DiscoverNextUnusedAddress(
    const mojom::AccountIdPtr& account_id,
    bool change,
    DiscoverNextUnusedAddressCallback callback) {
  CHECK(IsBitcoinAccount(account_id));

  auto account_info = keyring_service().GetBitcoinAccountInfo(account_id);
  if (!account_info) {
    return std::move(callback).Run(
        base::unexpected(WalletInternalErrorMessage()));
  }
  auto start_address = change ? account_info->next_change_address.Clone()
                              : account_info->next_receive_address.Clone();

  auto [task_it, inserted] = discover_next_unused_address_tasks_.insert(
      std::make_unique<DiscoverNextUnusedAddressTask>(
          *this, account_id.Clone(), std::move(start_address)));
  CHECK(inserted);
  auto* task_ptr = task_it->get();

  task_ptr->Start(base::BindOnce(
      &BitcoinWalletService::OnDiscoverNextUnusedAddressDone,
      weak_ptr_factory_.GetWeakPtr(), task_ptr, std::move(callback)));
}

void BitcoinWalletService::OnDiscoverNextUnusedAddressDone(
    DiscoverNextUnusedAddressTask* task,
    DiscoverNextUnusedAddressCallback callback,
    base::expected<mojom::BitcoinAddressPtr, std::string> result) {
  CHECK(discover_next_unused_address_tasks_.erase(task));

  std::move(callback).Run(std::move(result));
}

void BitcoinWalletService::DiscoverWalletAccount(
    mojom::KeyringId keyring_id,
    uint32_t account_index,
    DiscoverWalletAccountCallback callback) {
  auto [task_it, inserted] = discover_wallet_account_tasks_.insert(
      std::make_unique<DiscoverWalletAccountTask>(*this, keyring_id,
                                                  account_index));
  CHECK(inserted);
  auto* task_ptr = task_it->get();

  task_ptr->Start(base::BindOnce(
      &BitcoinWalletService::OnDiscoverWalletAccountDone,
      weak_ptr_factory_.GetWeakPtr(), task_ptr, std::move(callback)));
}

void BitcoinWalletService::OnDiscoverWalletAccountDone(
    DiscoverWalletAccountTask* task,
    DiscoverWalletAccountCallback callback,
    base::expected<DiscoveredBitcoinAccount, std::string> result) {
  CHECK(discover_wallet_account_tasks_.erase(task));

  std::move(callback).Run(std::move(result));
}

mojom::BtcHardwareTransactionSignDataPtr
BitcoinWalletService::GetBtcHardwareTransactionSignData(
    const BitcoinTransaction& tx,
    const mojom::AccountIdPtr& account_id) {
  auto addresses = keyring_service_->GetBitcoinAddresses(account_id);
  if (!addresses || addresses->empty()) {
    return nullptr;
  }

  std::map<std::string, mojom::BitcoinKeyIdPtr> address_map;
  for (auto& addr : *addresses) {
    address_map.emplace(std::move(addr->address_string),
                        std::move(addr->key_id));
  }

  auto sign_data = mojom::BtcHardwareTransactionSignData::New();
  for (auto& input : tx.inputs()) {
    auto input_data = mojom::BtcHardwareTransactionSignInputData::New();
    if (!input.raw_outpoint_tx) {
      return nullptr;
    }
    auto& key_id = address_map[input.utxo_address];
    input_data->tx_bytes = *input.raw_outpoint_tx;
    input_data->output_index = input.utxo_outpoint.index;
    input_data->associated_path = MakeHwPath(account_id, key_id);
    sign_data->inputs.push_back(std::move(input_data));
  }
  sign_data->output_script =
      BitcoinSerializer::SerializeOutputsForHardwareSigning(tx);
  if (tx.ChangeOutput()) {
    auto& key_id = address_map[tx.ChangeOutput()->address];
    sign_data->change_path = MakeHwPath(account_id, key_id);
  }
  sign_data->lock_time = tx.locktime();

  return sign_data;
}

bool BitcoinWalletService::SignTransactionInternal(
    BitcoinTransaction& tx,
    const mojom::AccountIdPtr& account_id) {
  auto addresses = keyring_service().GetBitcoinAddresses(account_id);
  if (!addresses || addresses->empty()) {
    return false;
  }

  std::map<std::string, mojom::BitcoinKeyIdPtr> address_map;
  for (auto& addr : *addresses) {
    address_map.emplace(std::move(addr->address_string),
                        std::move(addr->key_id));
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

    auto signature = keyring_service().SignMessageByBitcoinKeyring(
        account_id, key_id, *hash);
    if (!signature) {
      return false;
    }
    signature->push_back(tx.sighash_type());

    auto pubkey = keyring_service().GetBitcoinPubkey(account_id, key_id);
    if (!pubkey) {
      return false;
    }
    tx.SetInputWitness(
        input_index, BitcoinSerializer::SerializeWitness(*signature, *pubkey));
  }

  return true;
}

bool BitcoinWalletService::ApplyHwSignatureInternal(
    BitcoinTransaction& tx,
    const mojom::BitcoinSignature& hw_signature) {
  if (tx.inputs().size() != hw_signature.witness_array.size()) {
    return false;
  }

  for (size_t input_index = 0; input_index < tx.inputs().size();
       ++input_index) {
    tx.SetInputWitness(input_index, hw_signature.witness_array[input_index]);
  }

  return true;
}

void BitcoinWalletService::SetUrlLoaderFactoryForTesting(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory) {
  bitcoin_rpc_.SetUrlLoaderFactoryForTesting(  // IN-TEST
      std::move(url_loader_factory));
}

void BitcoinWalletService::SetArrangeTransactionsForTesting(bool arrange) {
  arrange_transactions_for_testing_ = arrange;
}

}  // namespace brave_wallet
