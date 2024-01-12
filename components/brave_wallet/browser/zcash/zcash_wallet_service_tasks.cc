/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/zcash/zcash_wallet_service_tasks.h"

#include <algorithm>
#include <utility>

#include "brave/components/brave_wallet/common/common_utils.h"
#include "brave/components/brave_wallet/common/zcash_utils.h"

namespace brave_wallet {

namespace {

const uint32_t kDefaultTransparentOutputsCount = 2;
const uint32_t kGraceActionsCount = 2;
const uint64_t kMarginalFee = 5000;

bool OutputAddressSupported(const std::string& address, bool is_testnet) {
  auto decoded_address = DecodeZCashAddress(address);
  if (!decoded_address) {
    return false;
  }
  if (decoded_address->testnet != is_testnet) {
    return false;
  }

  return true;
}

// https://zips.z.cash/zip-0317
uint64_t CalculateTxFee(const uint32_t tx_input_count) {
  // Use simplified calcultion fee form since we don't support p2psh
  // and shielded addresses
  auto actions_count =
      std::max(tx_input_count, kDefaultTransparentOutputsCount);
  return kMarginalFee * std::max(kGraceActionsCount, actions_count);
}

}  // namespace

// GetTransparentUtxosContext
GetTransparentUtxosContext::GetTransparentUtxosContext() = default;
GetTransparentUtxosContext::~GetTransparentUtxosContext() = default;

// DiscoverNextUnusedZCashAddressTask
DiscoverNextUnusedZCashAddressTask::DiscoverNextUnusedZCashAddressTask(
    base::WeakPtr<ZCashWalletService> zcash_wallet_service,
    mojom::AccountIdPtr account_id,
    mojom::ZCashAddressPtr start_address,
    ZCashWalletService::DiscoverNextUnusedAddressCallback callback)
    : zcash_wallet_service_(std::move(zcash_wallet_service)),
      account_id_(std::move(account_id)),
      start_address_(std::move(start_address)),
      callback_(std::move(callback)) {}

DiscoverNextUnusedZCashAddressTask::~DiscoverNextUnusedZCashAddressTask() =
    default;

void DiscoverNextUnusedZCashAddressTask::ScheduleWorkOnTask() {
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE,
      base::BindOnce(&DiscoverNextUnusedZCashAddressTask::WorkOnTask, this));
}

mojom::ZCashAddressPtr DiscoverNextUnusedZCashAddressTask::GetNextAddress(
    const mojom::ZCashAddressPtr& address) {
  auto* keyring_service = zcash_wallet_service_->keyring_service_.get();
  CHECK(keyring_service);
  auto next_key_id = current_address_->key_id.Clone();

  next_key_id->index++;

  return keyring_service->GetZCashAddress(*account_id_, *next_key_id);
}

void DiscoverNextUnusedZCashAddressTask::WorkOnTask() {
  if (!callback_) {
    return;
  }

  if (!zcash_wallet_service_) {
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

  if (!block_end_) {
    zcash_wallet_service_->zcash_rpc()->GetLatestBlock(
        GetNetworkForZCashKeyring(account_id_->keyring_id),
        base::BindOnce(&DiscoverNextUnusedZCashAddressTask::OnGetLastBlock,
                       this));
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

  zcash_wallet_service_->zcash_rpc()->IsKnownAddress(
      GetNetworkForZCashKeyring(account_id_->keyring_id),
      current_address_->address_string, 1, block_end_.value(),
      base::BindOnce(&DiscoverNextUnusedZCashAddressTask::OnGetIsKnownAddress,
                     this));
}

void DiscoverNextUnusedZCashAddressTask::OnGetLastBlock(
    base::expected<zcash::BlockID, std::string> result) {
  if (!result.has_value()) {
    error_ = result.error();
    WorkOnTask();
    return;
  }

  block_end_ = result.value().height();
  WorkOnTask();
}

void DiscoverNextUnusedZCashAddressTask::OnGetIsKnownAddress(
    base::expected<bool, std::string> result) {
  if (!result.has_value()) {
    error_ = result.error();
    WorkOnTask();
    return;
  }

  if (!result.value()) {
    result_ = current_address_->Clone();
  }

  WorkOnTask();
}

// CreateTransparentTransactionTask
CreateTransparentTransactionTask::CreateTransparentTransactionTask(
    ZCashWalletService* zcash_wallet_service,
    const std::string& chain_id,
    const mojom::AccountIdPtr& account_id,
    const std::string& address_to,
    uint64_t amount,
    CreateTransactionCallback callback)
    : zcash_wallet_service_(zcash_wallet_service),
      chain_id_(chain_id),
      account_id_(account_id.Clone()),
      callback_(std::move(callback)) {
  transaction_.set_to(address_to);
  transaction_.set_amount(amount);
}

CreateTransparentTransactionTask::~CreateTransparentTransactionTask() = default;

void CreateTransparentTransactionTask::ScheduleWorkOnTask() {
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(&CreateTransparentTransactionTask::WorkOnTask,
                                weak_ptr_factory_.GetWeakPtr()));
}

void CreateTransparentTransactionTask::WorkOnTask() {
  if (!callback_) {
    return;
  }

  if (error_) {
    std::move(callback_).Run(base::unexpected(*error_));
    zcash_wallet_service_->CreateTransactionTaskDone(this);
    return;
  }

  if (!chain_height_) {
    zcash_wallet_service_->zcash_rpc()->GetLatestBlock(
        chain_id_,
        base::BindOnce(&CreateTransparentTransactionTask::OnGetChainHeight,
                       weak_ptr_factory_.GetWeakPtr()));
    return;
  }

  if (!change_address_) {
    zcash_wallet_service_->DiscoverNextUnusedAddress(
        account_id_, true,
        base::BindOnce(&CreateTransparentTransactionTask::OnGetChangeAddress,
                       weak_ptr_factory_.GetWeakPtr()));
    return;
  }

  if (utxo_map_.empty()) {
    zcash_wallet_service_->GetUtxos(
        chain_id_, account_id_.Clone(),
        base::BindOnce(&CreateTransparentTransactionTask::OnGetUtxos,
                       weak_ptr_factory_.GetWeakPtr()));
    return;
  }

  // TODO(cypt4): random shift locktime
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

  DCHECK_EQ(kDefaultTransparentOutputsCount, transaction_.outputs().size());

  std::move(callback_).Run(base::ok(std::move(transaction_)));
  zcash_wallet_service_->CreateTransactionTaskDone(this);
}

void CreateTransparentTransactionTask::OnGetChainHeight(
    base::expected<zcash::BlockID, std::string> result) {
  if (!result.has_value()) {
    SetError(std::move(result).error());
    WorkOnTask();
    return;
  }

  chain_height_ = result.value().height();
  WorkOnTask();
}

void CreateTransparentTransactionTask::OnGetChangeAddress(
    base::expected<mojom::ZCashAddressPtr, std::string> result) {
  if (!result.has_value()) {
    SetError(std::move(result).error());
    WorkOnTask();
    return;
  }

  change_address_ = std::move(result.value());
  WorkOnTask();
}

void CreateTransparentTransactionTask::OnGetUtxos(
    base::expected<UtxoMap, std::string> utxo_map) {
  if (!utxo_map.has_value()) {
    SetError(std::move(utxo_map).error());
    WorkOnTask();
    return;
  }

  utxo_map_ = std::move(utxo_map.value());
  WorkOnTask();
}

bool CreateTransparentTransactionTask::PickInputs() {
  bool done = false;

  // TODO(apaymyshev): This just picks ouputs one by one and stops when picked
  // amount is GE to send amount plus fee. Needs something better than such
  // greedy strategy.
  std::vector<ZCashTransaction::TxInput> all_inputs;
  for (const auto& item : utxo_map_) {
    for (const auto& utxo : item.second) {
      if (auto input =
              ZCashTransaction::TxInput::FromRpcUtxo(item.first, utxo)) {
        all_inputs.emplace_back(std::move(*input));
      }
    }
  }

  base::ranges::sort(all_inputs, [](auto& input1, auto& input2) {
    return input1.utxo_value < input2.utxo_value;
  });

  for (auto& input : all_inputs) {
    transaction_.inputs().push_back(std::move(input));
    transaction_.set_fee(CalculateTxFee(transaction_.inputs().size()));

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

bool CreateTransparentTransactionTask::PrepareOutputs() {
  auto& target_output = transaction_.outputs().emplace_back();
  target_output.address = transaction_.to();
  if (!OutputAddressSupported(target_output.address, IsTestnet())) {
    return false;
  }

  target_output.amount = transaction_.amount();
  target_output.script_pubkey =
      ZCashAddressToScriptPubkey(target_output.address, IsTestnet());

  CHECK_GE(transaction_.TotalInputsAmount(),
           transaction_.amount() + transaction_.fee());
  uint64_t change_amount = transaction_.TotalInputsAmount() -
                           transaction_.amount() - transaction_.fee();
  if (change_amount == 0) {
    return true;
  }

  // TODO(cypt4): should always pick new change address.
  const auto& change_address = change_address_;
  if (!change_address) {
    return false;
  }

  CHECK(OutputAddressSupported(change_address->address_string, IsTestnet()));
  auto& change_output = transaction_.outputs().emplace_back();
  change_output.address = change_address_->address_string;
  change_output.amount = change_amount;
  change_output.script_pubkey =
      ZCashAddressToScriptPubkey(change_output.address, IsTestnet());
  return true;
}

}  // namespace brave_wallet
