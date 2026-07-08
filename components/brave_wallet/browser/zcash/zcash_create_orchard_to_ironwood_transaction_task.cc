// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_wallet/browser/zcash/zcash_create_orchard_to_ironwood_transaction_task.h"

#include <utility>
#include <variant>

#include "base/check.h"
#include "base/numerics/checked_math.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "brave/components/brave_wallet/common/zcash_utils.h"
#include "components/grit/brave_components_strings.h"
#include "ui/base/l10n/l10n_util.h"

namespace brave_wallet {

ZCashCreateOrchardToIronwoodTransactionTask::
    ZCashCreateOrchardToIronwoodTransactionTask(
        std::variant<
            base::PassKey<
                class ZCashCreateOrchardToIronwoodTransactionTaskTest>,
            base::PassKey<ZCashWalletService>> pass_key,
        ZCashWalletService& zcash_wallet_service,
        ZCashActionContext context,
        const OrchardAddrRawPart& receiver,
        std::optional<OrchardMemo> memo,
        uint64_t amount)
    : zcash_wallet_service_(zcash_wallet_service),
      context_(std::move(context)),
      receiver_(receiver),
      memo_(memo),
      amount_(amount) {}

ZCashCreateOrchardToIronwoodTransactionTask::
    ~ZCashCreateOrchardToIronwoodTransactionTask() = default;

void ZCashCreateOrchardToIronwoodTransactionTask::Start(
    CreateTransactionCallback callback) {
  DCHECK(!callback_);
  callback_ = std::move(callback);
  ScheduleWorkOnTask();
}

void ZCashCreateOrchardToIronwoodTransactionTask::ScheduleWorkOnTask() {
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE,
      base::BindOnce(
          &ZCashCreateOrchardToIronwoodTransactionTask::WorkOnTask,
          weak_ptr_factory_.GetWeakPtr()));
}

void ZCashCreateOrchardToIronwoodTransactionTask::WorkOnTask() {
  if (error_) {
    std::move(callback_).Run(base::unexpected(*error_));
    return;
  }

  if (!chain_tip_height_) {
    GetLatestBlock();
    return;
  }

  if (!spendable_notes_) {
    GetSpendableNotes();
    return;
  }

  if (!transaction_) {
    CreateTransaction();
    return;
  }

  std::move(callback_).Run(base::ok(std::move(*transaction_)));
}

void ZCashCreateOrchardToIronwoodTransactionTask::GetLatestBlock() {
  context_.zcash_rpc->GetLatestBlock(
      context_.chain_id,
      base::BindOnce(&ZCashCreateOrchardToIronwoodTransactionTask::
                         OnGetLatestBlockHeight,
                     weak_ptr_factory_.GetWeakPtr()));
}

void ZCashCreateOrchardToIronwoodTransactionTask::OnGetLatestBlockHeight(
    base::expected<zcash::mojom::BlockIDPtr, std::string> result) {
  if (!result.has_value()) {
    error_ = l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR);
    ScheduleWorkOnTask();
    return;
  }
  chain_tip_height_ = result.value()->height;
  ScheduleWorkOnTask();
}

void ZCashCreateOrchardToIronwoodTransactionTask::GetSpendableNotes() {
  if (!context_.account_internal_addr) {
    error_ = "No internal address provided";
    ScheduleWorkOnTask();
    return;
  }
  context_.sync_state->AsyncCall(&OrchardSyncState::GetSpendableNotes)
      .WithArgs(OrchardPool::kOrchard, context_.account_id.Clone(),
                context_.account_internal_addr.value())
      .Then(base::BindOnce(
          &ZCashCreateOrchardToIronwoodTransactionTask::OnGetSpendableNotes,
          weak_ptr_factory_.GetWeakPtr()));
}

void ZCashCreateOrchardToIronwoodTransactionTask::OnGetSpendableNotes(
    base::expected<std::optional<OrchardSyncState::SpendableNotesBundle>,
                   OrchardStorage::Error> result) {
  if (!result.has_value()) {
    error_ = result.error().message;
    ScheduleWorkOnTask();
    return;
  }

  if (!result.value()) {
    error_ = "No spendable notes";
    ScheduleWorkOnTask();
    return;
  }

  spendable_notes_ = std::move(*result.value());
  ScheduleWorkOnTask();
}

void ZCashCreateOrchardToIronwoodTransactionTask::CreateTransaction() {
  CHECK(spendable_notes_);
  CHECK(chain_tip_height_);
  auto pick_result =
      PickZCashOrchardInputs(spendable_notes_->spendable_notes, amount_,
                             ZCashTargetOutputType::kOrchard);
  if (!pick_result) {
    error_ = "Can't pick inputs";
    ScheduleWorkOnTask();
    return;
  }
  if (!context_.account_internal_addr) {
    error_ = "Internal address error";
    ScheduleWorkOnTask();
    return;
  }
  if (!spendable_notes_->anchor_block_id) {
    error_ = "Failed to select anchor";
    ScheduleWorkOnTask();
    return;
  }

  ZCashTransaction zcash_transaction;
  zcash_transaction.ConvertToV6();

  // Spends come from the legacy Orchard pool.
  for (const auto& note : pick_result.value().inputs) {
    OrchardInput orchard_input;
    orchard_input.note = note;
    zcash_transaction.v6_part().legacy_orchard.inputs.push_back(
        std::move(orchard_input));
  }
  zcash_transaction.set_fee(pick_result->fee);
  zcash_transaction.v6_part().legacy_orchard.anchor_block_height =
      spendable_notes_->anchor_block_id.value();

  // Outputs (change + recipient) go into the Ironwood pool.
  if (pick_result->change != 0) {
    OrchardOutput& change_output =
        zcash_transaction.v6_part().ironwood.outputs.emplace_back();
    change_output.value = pick_result->change;
    change_output.addr = context_.account_internal_addr.value();
  }
  OrchardOutput& orchard_output =
      zcash_transaction.v6_part().ironwood.outputs.emplace_back();
  auto value =
      base::CheckSub<uint64_t>(zcash_transaction.TotalInputsAmount(),
                               zcash_transaction.fee(), pick_result->change);
  if (!value.AssignIfValid(&orchard_output.value)) {
    error_ = l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR);
    ScheduleWorkOnTask();
    return;
  }
  orchard_output.addr = receiver_;
  orchard_output.memo = memo_;
  zcash_transaction.v6_part().ironwood.anchor_block_height =
      chain_tip_height_.value();

  auto orchard_unified_addr = GetOrchardUnifiedAddress(
      receiver_, IsZCashTestnetKeyring(context_.account_id->keyring_id));
  if (!orchard_unified_addr) {
    error_ = l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR);
    ScheduleWorkOnTask();
    return;
  }
  zcash_transaction.set_amount(orchard_output.value);
  zcash_transaction.set_to(*orchard_unified_addr);

  transaction_ = std::move(zcash_transaction);
  ScheduleWorkOnTask();
}

}  // namespace brave_wallet
