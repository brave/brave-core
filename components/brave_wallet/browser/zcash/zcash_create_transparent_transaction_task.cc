// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_wallet/browser/zcash/zcash_create_transparent_transaction_task.h"

#include <utility>

#include "base/containers/extend.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_transaction_utils.h"
#include "brave/components/brave_wallet/common/zcash_utils.h"
#include "components/grit/brave_components_strings.h"
#include "ui/base/l10n/l10n_util.h"

namespace brave_wallet {

// CreateTransparentTransactionTask
ZCashCreateTransparentTransactionTask::ZCashCreateTransparentTransactionTask(
    ZCashWalletService& zcash_wallet_service,
    const std::string& chain_id,
    const mojom::AccountIdPtr& account_id,
    const std::string& address_to,
    uint64_t amount,
    CreateTransactionCallback callback)
    : zcash_wallet_service_(zcash_wallet_service),
      chain_id_(chain_id),
      account_id_(account_id.Clone()),
      amount_(amount),
      callback_(std::move(callback)) {
  transaction_.set_to(address_to);
  transaction_.set_amount(amount);
}

ZCashCreateTransparentTransactionTask::
    ~ZCashCreateTransparentTransactionTask() = default;

void ZCashCreateTransparentTransactionTask::ScheduleWorkOnTask() {
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE,
      base::BindOnce(&ZCashCreateTransparentTransactionTask::WorkOnTask,
                     weak_ptr_factory_.GetWeakPtr()));
}

void ZCashCreateTransparentTransactionTask::WorkOnTask() {
  if (!callback_) {
    return;
  }

  if (error_) {
    std::move(callback_).Run(base::unexpected(*error_));
    zcash_wallet_service_->CreateTransactionTaskDone(this);
    return;
  }

  if (!chain_height_) {
    zcash_wallet_service_->zcash_rpc().GetLatestBlock(
        chain_id_,
        base::BindOnce(&ZCashCreateTransparentTransactionTask::OnGetChainHeight,
                       weak_ptr_factory_.GetWeakPtr()));
    return;
  }

  if (!change_address_) {
    zcash_wallet_service_->DiscoverNextUnusedAddress(
        account_id_, true,
        base::BindOnce(
            &ZCashCreateTransparentTransactionTask::OnGetChangeAddress,
            weak_ptr_factory_.GetWeakPtr()));
    return;
  }

  if (utxo_map_.empty()) {
    zcash_wallet_service_->GetUtxos(
        chain_id_, account_id_.Clone(),
        base::BindOnce(&ZCashCreateTransparentTransactionTask::OnGetUtxos,
                       weak_ptr_factory_.GetWeakPtr()));
    return;
  }

  // TODO(cypt4): random shift locktime
  // https://github.com/bitcoin/bitcoin/blob/v24.0/src/wallet/spend.cpp#L739-L747
  transaction_.set_locktime(chain_height_.value());

  auto pick_inputs_result = PickZCashTransparentInputs(utxo_map_, amount_, 0);
  if (!pick_inputs_result) {
    // TODO(cypt4) : switch to IDS_BRAVE_WALLET_INSUFFICIENT_BALANCE when ready
    SetError(l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    WorkOnTask();
    return;
  }
  transaction_.set_fee(pick_inputs_result->fee);
  base::Extend(transaction_.transparent_part().inputs,
               pick_inputs_result->inputs);

  if (!PrepareOutputs()) {
    SetError(l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    WorkOnTask();
    return;
  }

  DCHECK_EQ(kDefaultTransparentOutputsCount,
            transaction_.transparent_part().outputs.size());

  std::move(callback_).Run(base::ok(std::move(transaction_)));
  zcash_wallet_service_->CreateTransactionTaskDone(this);
}

void ZCashCreateTransparentTransactionTask::OnGetChainHeight(
    base::expected<zcash::mojom::BlockIDPtr, std::string> result) {
  if (!result.has_value() || !result.value()) {
    SetError(std::move(result).error());
    WorkOnTask();
    return;
  }

  chain_height_ = (*result)->height;
  WorkOnTask();
}

void ZCashCreateTransparentTransactionTask::OnGetChangeAddress(
    base::expected<mojom::ZCashAddressPtr, std::string> result) {
  if (!result.has_value()) {
    SetError(std::move(result).error());
    WorkOnTask();
    return;
  }

  change_address_ = std::move(result.value());
  WorkOnTask();
}

void ZCashCreateTransparentTransactionTask::OnGetUtxos(
    base::expected<UtxoMap, std::string> utxo_map) {
  if (!utxo_map.has_value()) {
    SetError(std::move(utxo_map).error());
    WorkOnTask();
    return;
  }

  utxo_map_ = std::move(utxo_map.value());
  WorkOnTask();
}

bool ZCashCreateTransparentTransactionTask::PrepareOutputs() {
  auto& target_output = transaction_.transparent_part().outputs.emplace_back();
  target_output.address = transaction_.to();
  if (!OutputZCashAddressSupported(target_output.address, IsTestnet())) {
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

  const auto& change_address = change_address_;
  if (!change_address) {
    return false;
  }

  CHECK(
      OutputZCashAddressSupported(change_address->address_string, IsTestnet()));
  auto& change_output = transaction_.transparent_part().outputs.emplace_back();
  change_output.address = change_address_->address_string;
  change_output.amount = change_amount;
  change_output.script_pubkey =
      ZCashAddressToScriptPubkey(change_output.address, IsTestnet());
  return true;
}

}  // namespace brave_wallet
