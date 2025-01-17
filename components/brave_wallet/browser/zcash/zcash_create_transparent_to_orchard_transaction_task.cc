// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_wallet/browser/zcash/zcash_create_transparent_to_orchard_transaction_task.h"

#include "brave/components/brave_wallet/browser/zcash/zcash_serializer.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_transaction_utils.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "brave/components/brave_wallet/common/zcash_utils.h"
#include "components/grit/brave_components_strings.h"
#include "ui/base/l10n/l10n_util.h"

namespace brave_wallet {

ZCashCreateTransparentToOrchardTransactionTask::
    ZCashCreateTransparentToOrchardTransactionTask(
        absl::variant<
            base::PassKey<
                class ZCashCreateTransparentToOrchardTransactionTaskTest>,
            base::PassKey<class ZCashWalletService>> pass_key,
        ZCashWalletService& zcash_wallet_service,
        ZCashActionContext context,
        const OrchardAddrRawPart& receiver,
        std::optional<OrchardMemo> memo,
        uint64_t amount,
        ZCashWalletService::CreateTransactionCallback callback)
    : zcash_wallet_service_(zcash_wallet_service),
      context_(std::move(context)),
      receiver_(receiver),
      memo_(memo),
      amount_(amount),
      callback_(std::move(callback)) {}

ZCashCreateTransparentToOrchardTransactionTask::
    ~ZCashCreateTransparentToOrchardTransactionTask() = default;

void ZCashCreateTransparentToOrchardTransactionTask::Start() {
  CHECK(!started_);
  started_ = true;
  ScheduleWorkOnTask();
}

void ZCashCreateTransparentToOrchardTransactionTask::ScheduleWorkOnTask() {
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE,
      base::BindOnce(
          &ZCashCreateTransparentToOrchardTransactionTask::WorkOnTask,
          weak_ptr_factory_.GetWeakPtr()));
}

void ZCashCreateTransparentToOrchardTransactionTask::WorkOnTask() {
  if (error_) {
    std::move(callback_).Run(base::unexpected(error_.value()));
    zcash_wallet_service_->CreateTransactionTaskDone(this);
    return;
  }

  if (!utxo_map_) {
    GetAllUtxos();
    return;
  }

  if (amount_ != kZCashFullAmount && !change_address_) {
    GetChangeAddress();
    return;
  }

  if (!transaction_ && !CreateTransaction()) {
    std::move(callback_).Run(base::unexpected(error_.value()));
    zcash_wallet_service_->CreateTransactionTaskDone(this);
    return;
  }

  std::move(callback_).Run(std::move(transaction_.value()));
  zcash_wallet_service_->CreateTransactionTaskDone(this);
}

bool ZCashCreateTransparentToOrchardTransactionTask::CreateTransaction() {
  CHECK(utxo_map_);

  ZCashTransaction zcash_transaction;

  // Pick transparent inputs
  // TODO(cypt4): Calculate orchard actions count
  auto pick_transparent_inputs_result = PickZCashTransparentInputs(
      *utxo_map_, amount_,
      2 /* actions count for 1 orchard output no orchard inputs */);
  if (!pick_transparent_inputs_result) {
    error_ = l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR);
    return false;
  }
  zcash_transaction.transparent_part().inputs =
      pick_transparent_inputs_result->inputs;
  zcash_transaction.set_fee(pick_transparent_inputs_result->fee);

  // Pick transparent outputs for change
  if (pick_transparent_inputs_result->change != 0) {
    CHECK_NE(amount_, kZCashFullAmount);
    auto& change_output =
        zcash_transaction.transparent_part().outputs.emplace_back();
    change_output.address = change_address_->address_string;
    change_output.amount = pick_transparent_inputs_result->change;
    change_output.script_pubkey = ZCashAddressToScriptPubkey(
        change_output.address, context_.chain_id == mojom::kZCashTestnet);
  }

  // Create shielded output
  OrchardOutput& orchard_output =
      zcash_transaction.orchard_part().outputs.emplace_back();
  orchard_output.value = zcash_transaction.TotalInputsAmount() -
                         zcash_transaction.fee() -
                         pick_transparent_inputs_result->change;
  orchard_output.addr = receiver_;
  orchard_output.memo = memo_;

  auto orchard_unified_addr = GetOrchardUnifiedAddress(
      receiver_, context_.chain_id == mojom::kZCashTestnet);
  if (!orchard_unified_addr) {
    error_ = l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR);
    return false;
  }
  zcash_transaction.set_amount(orchard_output.value);
  zcash_transaction.set_to(*orchard_unified_addr);
  zcash_transaction.set_memo(memo_);

  transaction_ = std::move(zcash_transaction);

  return true;
}

void ZCashCreateTransparentToOrchardTransactionTask::GetAllUtxos() {
  zcash_wallet_service_->GetUtxos(
      context_.chain_id, context_.account_id.Clone(),
      base::BindOnce(
          &ZCashCreateTransparentToOrchardTransactionTask::OnGetUtxos,
          weak_ptr_factory_.GetWeakPtr()));
}

void ZCashCreateTransparentToOrchardTransactionTask::GetChangeAddress() {
  zcash_wallet_service_->DiscoverNextUnusedAddress(
      context_.account_id.Clone(), true,
      base::BindOnce(
          &ZCashCreateTransparentToOrchardTransactionTask::OnGetChangeAddress,
          weak_ptr_factory_.GetWeakPtr()));
}

void ZCashCreateTransparentToOrchardTransactionTask::OnGetChangeAddress(
    base::expected<mojom::ZCashAddressPtr, std::string> result) {
  if (!result.has_value()) {
    error_ = result.error();
    ScheduleWorkOnTask();
    return;
  }

  change_address_ = std::move(result.value());
  ScheduleWorkOnTask();
}

void ZCashCreateTransparentToOrchardTransactionTask::OnGetUtxos(
    base::expected<ZCashWalletService::UtxoMap, std::string> utxo_map) {
  if (!utxo_map.has_value()) {
    error_ = utxo_map.error();
  } else {
    utxo_map_ = std::move(*utxo_map);
  }

  ScheduleWorkOnTask();
}

}  // namespace brave_wallet
