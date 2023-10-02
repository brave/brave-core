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
#include "base/check_op.h"
#include "base/functional/bind.h"
#include "base/memory/scoped_refptr.h"
#include "base/notreached.h"
#include "base/rand_util.h"
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

const uint32_t kHighPriorityTargetBlock = 1;
const uint32_t kMediumPriorityTargetBlock = 4;
const double kFallbackFeeRate = 1;  // 1 sat per byte fallback rate.

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

const std::vector<uint8_t>& DummySignature() {
  static std::vector<uint8_t> dummy_signature = []() {
    constexpr size_t kRLength = 32;
    constexpr size_t kSLength = 32;
    std::vector<uint8_t> result;
    result.assign(kRLength + kSLength + 7, 0);
    result[0] = 0x30;
    result[1] = kRLength + kSLength + 4;
    result[2] = 0x02;
    result[3] = kRLength;
    result[4] = 0x01;
    result[4 + kRLength] = 0x02;
    result[5 + kRLength] = kSLength;
    result[6 + kRLength] = 0x01;
    result[6 + kRLength + kSLength] = kBitcoinSigHashAll;
    return result;
  }();
  return dummy_signature;
}

const std::vector<uint8_t>& DummyPubkey() {
  static std::vector<uint8_t> dummy_pubkey = []() {
    constexpr size_t kLenght = 33;
    std::vector<uint8_t> result(kLenght, 0);
    return result;
  }();
  return dummy_pubkey;
}

const std::vector<uint8_t>& DummyWitness() {
  static std::vector<uint8_t> dummy_witness = []() {
    return BitcoinSerializer::SerializeWitness(DummySignature(), DummyPubkey());
  }();
  return dummy_witness;
}

}  // namespace

class GetBalanceTask : public base::RefCountedThreadSafe<GetBalanceTask> {
 public:
  using GetBalanceCallback = mojom::BitcoinWalletService::GetBalanceCallback;

  GetBalanceTask(base::WeakPtr<BitcoinWalletService> bitcoin_wallet_service,
                 const std::string& chain_id,
                 std::vector<mojom::BitcoinAddressPtr> addresses,
                 GetBalanceCallback callback);
  void ScheduleWorkOnTask();

 private:
  friend class base::RefCountedThreadSafe<GetBalanceTask>;
  virtual ~GetBalanceTask() = default;

  void MaybeSendRequests();
  void OnGetAddressStats(
      mojom::BitcoinAddressPtr address,
      base::expected<bitcoin_rpc::AddressStats, std::string> stats);
  void WorkOnTask();

  base::WeakPtr<BitcoinWalletService> bitcoin_wallet_service_;
  std::string chain_id_;
  std::vector<mojom::BitcoinAddressPtr> addresses_;
  bool requests_sent_ = false;

  std::map<std::string, uint64_t> balances_;
  absl::optional<std::string> error_;
  mojom::BitcoinBalancePtr result_;
  GetBalanceCallback callback_;
};

GetBalanceTask::GetBalanceTask(
    base::WeakPtr<BitcoinWalletService> bitcoin_wallet_service,
    const std::string& chain_id,
    std::vector<mojom::BitcoinAddressPtr> addresses,
    GetBalanceCallback callback)
    : bitcoin_wallet_service_(bitcoin_wallet_service),
      chain_id_(chain_id),
      addresses_(std::move(addresses)),
      callback_(std::move(callback)) {}

void GetBalanceTask::ScheduleWorkOnTask() {
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(&GetBalanceTask::WorkOnTask, this));
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
        base::BindOnce(&GetBalanceTask::OnGetAddressStats, this,
                       address_info->Clone()));
  }
}

void GetBalanceTask::WorkOnTask() {
  if (!callback_) {
    return;
  }
  if (!bitcoin_wallet_service_) {
    std::move(callback_).Run(nullptr, "Internal error");
    return;
  }

  if (error_) {
    std::move(callback_).Run(nullptr, std::move(*error_));
    return;
  }

  if (result_) {
    std::move(callback_).Run(std::move(result_), absl::nullopt);
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

  auto chain_balance = GetChainBalance(stats->chain_stats);
  // TODO(apaymyshev): should show only confirmed balance?
  auto mempool_balance = GetChainBalance(stats->mempool_stats);
  balances_[address->address_string] = chain_balance + mempool_balance;

  CHECK(base::Erase(addresses_, address));
  if (addresses_.empty()) {
    result_ = mojom::BitcoinBalance::New();
    for (auto& balance : balances_) {
      result_->total_balance += balance.second;
    }
    result_->balances.insert(balances_.begin(), balances_.end());
  }

  WorkOnTask();
}

class GetUtxosTask : public base::RefCountedThreadSafe<GetUtxosTask> {
 public:
  GetUtxosTask(base::WeakPtr<BitcoinWalletService> bitcoin_wallet_service,
               const std::string& chain_id,
               std::vector<mojom::BitcoinAddressPtr> addresses,
               BitcoinWalletService::GetUtxosCallback callback);
  void ScheduleWorkOnTask();

 private:
  friend class base::RefCountedThreadSafe<GetUtxosTask>;
  virtual ~GetUtxosTask() = default;

  void WorkOnTask();
  void MaybeSendRequests();
  void OnGetUtxoList(
      mojom::BitcoinAddressPtr address,
      base::expected<bitcoin_rpc::UnspentOutputs, std::string> utxos);

  base::WeakPtr<BitcoinWalletService> bitcoin_wallet_service_;
  std::string chain_id_;
  std::vector<mojom::BitcoinAddressPtr> addresses_;
  bool requests_sent_ = false;

  BitcoinWalletService::UtxoMap utxos_;
  absl::optional<std::string> error_;
  absl::optional<BitcoinWalletService::UtxoMap> result_;
  BitcoinWalletService::GetUtxosCallback callback_;
};

GetUtxosTask::GetUtxosTask(
    base::WeakPtr<BitcoinWalletService> bitcoin_wallet_service,
    const std::string& chain_id,
    std::vector<mojom::BitcoinAddressPtr> addresses,
    BitcoinWalletService::GetUtxosCallback callback)
    : bitcoin_wallet_service_(bitcoin_wallet_service),
      chain_id_(chain_id),
      addresses_(std::move(addresses)),
      callback_(std::move(callback)) {}

void GetUtxosTask::ScheduleWorkOnTask() {
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(&GetUtxosTask::WorkOnTask, this));
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
        base::BindOnce(&GetUtxosTask::OnGetUtxoList, this,
                       address_info->Clone()));
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

  CHECK(base::Erase(addresses_, address));
  if (addresses_.empty()) {
    result_ = std::move(utxos_);
  }

  WorkOnTask();
}

void GetUtxosTask::WorkOnTask() {
  if (!callback_) {
    return;
  }
  if (!bitcoin_wallet_service_) {
    std::move(callback_).Run(base::unexpected("Internal error"));
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
  using CreateTransactionCallback =
      BitcoinWalletService::CreateTransactionCallback;

  CreateTransactionTask(BitcoinWalletService* bitcoin_wallet_service,
                        const mojom::AccountIdPtr& account_id,
                        const std::string& address_to,
                        uint64_t amount,
                        CreateTransactionCallback callback);

  void ScheduleWorkOnTask();

 private:
  bool IsTestnet() { return IsBitcoinTestnetKeyring(account_id_->keyring_id); }
  void WorkOnTask();

  void SetError(const std::string& error_string) { error_ = error_string; }

  double GetFeeRate();
  bool PickInputs();
  bool PrepareOutputs();

  void OnGetChainHeight(base::expected<uint32_t, std::string> chain_height);
  void OnGetFeeEstimates(
      base::expected<std::map<uint32_t, double>, std::string> estimates);
  void OnGetUtxos(base::expected<UtxoMap, std::string> utxo_map);
  void OnDiscoverNextUnusedAddress(
      base::expected<mojom::BitcoinAddressPtr, std::string> address);

  raw_ptr<BitcoinWalletService> bitcoin_wallet_service_;  // Owns `this`.
  mojom::AccountIdPtr account_id_;
  CreateTransactionCallback callback_;

  absl::optional<uint32_t> chain_height_;
  BitcoinWalletService::UtxoMap utxo_map_;
  mojom::BitcoinAddressPtr change_address_;
  std::map<uint32_t, double> estimates_;  // target block -> fee rate (sat/byte)

  absl::optional<std::string> error_;
  BitcoinTransaction transaction_;

  base::WeakPtrFactory<CreateTransactionTask> weak_ptr_factory_{this};
};

CreateTransactionTask::CreateTransactionTask(
    BitcoinWalletService* bitcoin_wallet_service,
    const mojom::AccountIdPtr& account_id,
    const std::string& address_to,
    uint64_t amount,
    CreateTransactionCallback callback)
    : bitcoin_wallet_service_(bitcoin_wallet_service),
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
        base::BindOnce(&CreateTransactionTask::OnDiscoverNextUnusedAddress,
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

  if (!PrepareOutputs()) {
    SetError("Couldn't prepare outputs");
    ScheduleWorkOnTask();
    return;
  }

  if (!PickInputs()) {
    SetError("Insufficient funds");
    ScheduleWorkOnTask();
    return;
  }

  std::move(callback_).Run(base::ok(std::move(transaction_)));
  bitcoin_wallet_service_->CreateTransactionTaskDone(this);
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

void CreateTransactionTask::OnDiscoverNextUnusedAddress(
    base::expected<mojom::BitcoinAddressPtr, std::string> address) {
  if (!address.has_value()) {
    SetError(std::move(address.error()));
    WorkOnTask();
    return;
  }
  change_address_ = std::move(address.value());
  WorkOnTask();
}

double CreateTransactionTask::GetFeeRate() {
  DCHECK(!estimates_.empty());
  if (estimates_.contains(kMediumPriorityTargetBlock)) {
    return estimates_.at(kMediumPriorityTargetBlock);
  }
  if (estimates_.contains(kHighPriorityTargetBlock)) {
    return estimates_.at(kHighPriorityTargetBlock);
  }
  return kFallbackFeeRate;
}

bool CreateTransactionTask::PickInputs() {
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

  // TODO(apaymyshev): This just picks ouputs one by one and stops when picked
  // amount is GE to send amount plus calculated fee. Needs something better
  // than such greedy strategy.
  for (auto& input : all_inputs) {
    // TODO(apaymyshev): Should support dummy scriptsig for non-segwit inputs.
    // Adding dummy signatures so transaction's virtual size and dependant fee
    // could be calculated.
    transaction_.inputs().push_back(std::move(input));
    transaction_.inputs().back().witness = DummyWitness();

    uint32_t fee = GetFeeRate() * BitcoinSerializer::CalcVSize(transaction_);

    if (transaction_.TotalInputsAmount() >= transaction_.amount() + fee) {
      CHECK_EQ(transaction_.outputs().size(), 2u);
      // TODO(apaymyshev): should avoid empty and dust change outputs.
      transaction_.outputs()[1].amount =
          transaction_.TotalInputsAmount() - (transaction_.amount() + fee);

      DCHECK_EQ(fee, transaction_.EffectiveFeeAmount());

      // Clear dummy signatures.
      DCHECK(transaction_.IsSigned());
      transaction_.ClearSignatures();

      return true;
    }
  }

  return false;
}

bool CreateTransactionTask::PrepareOutputs() {
  auto& target_output = transaction_.outputs().emplace_back();
  target_output.address = transaction_.to();
  target_output.amount = transaction_.amount();
  if (target_output.amount == 0) {
    return false;
  }
  if (!OutputAddressSupported(target_output.address, IsTestnet())) {
    return false;
  }

  // Always add change address. Change amount is finalized when we have enough
  // inputs picked to pay target amount + fee.
  CHECK(change_address_);
  CHECK(OutputAddressSupported(change_address_->address_string, IsTestnet()));
  auto& change_output = transaction_.outputs().emplace_back();
  change_output.address = change_address_->address_string;
  change_output.amount = 0;

  return true;
}

class DiscoverNextUnusedAddressTask
    : public base::RefCounted<DiscoverNextUnusedAddressTask> {
 public:
  DiscoverNextUnusedAddressTask(
      base::WeakPtr<BitcoinWalletService> bitcoin_wallet_service,
      mojom::AccountIdPtr account_id,
      mojom::BitcoinAddressPtr start_address,
      BitcoinWalletService::DiscoverNextUnusedAddressCallback callback);
  void ScheduleWorkOnTask();

 private:
  friend class base::RefCounted<DiscoverNextUnusedAddressTask>;
  ~DiscoverNextUnusedAddressTask() = default;

  mojom::BitcoinAddressPtr GetNextAddress(
      const mojom::BitcoinAddressPtr& address);

  void WorkOnTask();
  void OnGetAddressStats(
      base::expected<bitcoin_rpc::AddressStats, std::string> stats);

  base::WeakPtr<BitcoinWalletService> bitcoin_wallet_service_;
  mojom::AccountIdPtr account_id_;
  mojom::BitcoinAddressPtr start_address_;
  mojom::BitcoinAddressPtr current_address_;
  mojom::BitcoinAddressPtr result_;
  absl::optional<std::string> error_;
  BitcoinWalletService::DiscoverNextUnusedAddressCallback callback_;
};

DiscoverNextUnusedAddressTask::DiscoverNextUnusedAddressTask(
    base::WeakPtr<BitcoinWalletService> bitcoin_wallet_service,
    mojom::AccountIdPtr account_id,
    mojom::BitcoinAddressPtr start_address,
    BitcoinWalletService::DiscoverNextUnusedAddressCallback callback)
    : bitcoin_wallet_service_(std::move(bitcoin_wallet_service)),
      account_id_(std::move(account_id)),
      start_address_(std::move(start_address)),
      callback_(std::move(callback)) {}

void DiscoverNextUnusedAddressTask::ScheduleWorkOnTask() {
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE,
      base::BindOnce(&DiscoverNextUnusedAddressTask::WorkOnTask, this));
}

mojom::BitcoinAddressPtr DiscoverNextUnusedAddressTask::GetNextAddress(
    const mojom::BitcoinAddressPtr& address) {
  const auto& keyring_service = bitcoin_wallet_service_->keyring_service_;
  CHECK(keyring_service);

  auto next_key_id = current_address_->key_id.Clone();
  next_key_id->index++;

  return keyring_service->GetBitcoinAddress(account_id_, next_key_id);
}

void DiscoverNextUnusedAddressTask::WorkOnTask() {
  if (!callback_) {
    return;
  }

  if (!bitcoin_wallet_service_) {
    std::move(callback_).Run(base::unexpected("Internal error"));
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
    error_ = "Internal error";
    ScheduleWorkOnTask();
    return;
  }

  bitcoin_wallet_service_->bitcoin_rpc().GetAddressStats(
      GetNetworkForBitcoinAccount(account_id_),
      current_address_->address_string,
      base::BindOnce(&DiscoverNextUnusedAddressTask::OnGetAddressStats, this));
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
    error_ = "Invalid response";
    WorkOnTask();
    return;
  }

  if (chain_stats_tx_count == 0 && mempool_stats_tx_count == 0) {
    result_ = current_address_->Clone();
  }

  WorkOnTask();
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

void BitcoinWalletService::GetBalance(mojom::AccountIdPtr account_id,
                                      GetBalanceCallback callback) {
  CHECK(IsBitcoinAccount(*account_id));

  auto addresses = keyring_service_->GetBitcoinAddresses(account_id);
  if (!addresses) {
    std::move(callback).Run(nullptr, "Couldn't get balance");
    return;
  }

  auto context = base::MakeRefCounted<GetBalanceTask>(
      weak_ptr_factory_.GetWeakPtr(), GetNetworkForBitcoinAccount(account_id),
      std::move(*addresses), std::move(callback));
  context->ScheduleWorkOnTask();
}

void BitcoinWalletService::GetBitcoinAccountInfo(
    mojom::AccountIdPtr account_id,
    GetBitcoinAccountInfoCallback callback) {
  std::move(callback).Run(GetBitcoinAccountInfoSync(account_id));
}

mojom::BitcoinAccountInfoPtr BitcoinWalletService::GetBitcoinAccountInfoSync(
    const mojom::AccountIdPtr& account_id) {
  return keyring_service_->GetBitcoinAccountInfo(account_id);
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

void BitcoinWalletService::OnRunDiscoveryDone(
    mojom::AccountIdPtr account_id,
    RunDiscoveryCallback callback,
    base::expected<mojom::BitcoinAddressPtr, std::string> discovered_address) {
  if (!discovered_address.has_value()) {
    std::move(callback).Run(nullptr, discovered_address.error());
    return;
  }

  UpdateNextUnusedAddressForAccount(account_id, discovered_address.value());
  std::move(callback).Run(discovered_address.value().Clone(), absl::nullopt);
}

void BitcoinWalletService::UpdateNextUnusedAddressForAccount(
    const mojom::AccountIdPtr& account_id,
    const mojom::BitcoinAddressPtr& address) {
  keyring_service_->UpdateNextUnusedAddressForBitcoinAccount(account_id,
                                                             address->key_id);
}

void BitcoinWalletService::GetUtxos(mojom::AccountIdPtr account_id,
                                    GetUtxosCallback callback) {
  CHECK(IsBitcoinAccount(*account_id));

  auto addresses = keyring_service_->GetBitcoinAddresses(account_id);
  if (!addresses) {
    NOTREACHED();
    std::move(callback).Run(base::unexpected("Couldn't get balance"));
    return;
  }

  auto task = base::MakeRefCounted<GetUtxosTask>(
      weak_ptr_factory_.GetWeakPtr(), GetNetworkForBitcoinAccount(account_id),
      std::move(*addresses), std::move(callback));
  task->ScheduleWorkOnTask();
}

void BitcoinWalletService::CreateTransaction(
    mojom::AccountIdPtr account_id,
    const std::string& address_to,
    uint64_t amount,
    CreateTransactionCallback callback) {
  CHECK(IsBitcoinAccount(*account_id));

  auto& task = create_transaction_tasks_.emplace_back(
      std::make_unique<CreateTransactionTask>(this, account_id, address_to,
                                              amount, std::move(callback)));
  task->ScheduleWorkOnTask();
}

void BitcoinWalletService::CreateTransactionTaskDone(
    CreateTransactionTask* task) {
  CHECK(create_transaction_tasks_.remove_if(
      [task](auto& item) { return item.get() == task; }));
}

void BitcoinWalletService::SignAndPostTransaction(
    const mojom::AccountIdPtr& account_id,
    BitcoinTransaction bitcoin_transaction,
    SignAndPostTransactionCallback callback) {
  CHECK(IsBitcoinAccount(*account_id));

  if (!SignTransactionInternal(bitcoin_transaction, account_id)) {
    std::move(callback).Run("", std::move(bitcoin_transaction),
                            "Couldn't sign transaciton");
    return;
  }

  auto serialized_transaction =
      BitcoinSerializer::SerializeSignedTransaction(bitcoin_transaction);

  bitcoin_rpc_->PostTransaction(
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

void BitcoinWalletService::DiscoverNextUnusedAddress(
    const mojom::AccountIdPtr& account_id,
    bool change,
    DiscoverNextUnusedAddressCallback callback) {
  CHECK(IsBitcoinAccount(*account_id));

  auto account_info = keyring_service_->GetBitcoinAccountInfo(account_id);
  if (!account_info) {
    return std::move(callback).Run(base::unexpected("Invalid account id"));
  }
  auto start_address = change ? account_info->next_change_address.Clone()
                              : account_info->next_receive_address.Clone();
  auto task = base::MakeRefCounted<DiscoverNextUnusedAddressTask>(
      weak_ptr_factory_.GetWeakPtr(), account_id.Clone(),
      std::move(start_address), std::move(callback));
  task->ScheduleWorkOnTask();
}

bool BitcoinWalletService::SignTransactionInternal(
    BitcoinTransaction& tx,
    const mojom::AccountIdPtr& account_id) {
  auto addresses = keyring_service_->GetBitcoinAddresses(account_id);
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

    auto signature = keyring_service_->SignMessageByBitcoinKeyring(
        account_id, key_id, *hash);
    if (!signature) {
      return false;
    }
    signature->push_back(tx.sighash_type());

    auto pubkey = keyring_service_->GetBitcoinPubkey(account_id, key_id);
    if (!pubkey) {
      return false;
    }
    input.witness = BitcoinSerializer::SerializeWitness(*signature, *pubkey);
  }

  return true;
}

void BitcoinWalletService::SetUrlLoaderFactoryForTesting(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory) {
  bitcoin_rpc_->SetUrlLoaderFactoryForTesting(  // IN-TEST
      std::move(url_loader_factory));
}

}  // namespace brave_wallet
