/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/zcash/zcash_create_orchard_to_orchard_transaction_task.h"

#include <utility>
#include <variant>

#include "base/check.h"
#include "base/numerics/checked_math.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "components/grit/brave_components_strings.h"
#include "ui/base/l10n/l10n_util.h"

namespace brave_wallet {

ZCashCreateOrchardToOrchardTransactionTask::
    ZCashCreateOrchardToOrchardTransactionTask(
        std::variant<
            base::PassKey<class ZCashCreateOrchardToOrchardTransactionTaskTest>,
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

ZCashCreateOrchardToOrchardTransactionTask::
    ~ZCashCreateOrchardToOrchardTransactionTask() = default;

void ZCashCreateOrchardToOrchardTransactionTask::Start(
    CreateTransactionCallback callback) {
  DCHECK(!callback_);
  callback_ = std::move(callback);
  ScheduleWorkOnTask();
}

void ZCashCreateOrchardToOrchardTransactionTask::ScheduleWorkOnTask() {
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE,
      base::BindOnce(&ZCashCreateOrchardToOrchardTransactionTask::WorkOnTask,
                     weak_ptr_factory_.GetWeakPtr()));
}

void ZCashCreateOrchardToOrchardTransactionTask::WorkOnTask() {
  if (error_) {
    std::move(callback_).Run(base::unexpected(*error_));
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

void ZCashCreateOrchardToOrchardTransactionTask::GetSpendableNotes() {
  if (!context_.account_internal_addr) {
    error_ = "No internal address provided";
    ScheduleWorkOnTask();
    return;
  }
  context_.sync_state->AsyncCall(&OrchardSyncState::GetSpendableNotes)
      .WithArgs(context_.account_id.Clone(),
                context_.account_internal_addr.value())
      .Then(base::BindOnce(
          &ZCashCreateOrchardToOrchardTransactionTask::OnGetSpendableNotes,
          weak_ptr_factory_.GetWeakPtr()));
}

void ZCashCreateOrchardToOrchardTransactionTask::OnGetSpendableNotes(
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

void ZCashCreateOrchardToOrchardTransactionTask::CreateTransaction() {
  CHECK(spendable_notes_);
  auto pick_result =
      PickZCashOrchardInputs(spendable_notes_->spendable_notes, amount_);
  if (!pick_result) {
    error_ = "Can't pick inputs";
    ScheduleWorkOnTask();
    return;
  }

  ZCashTransaction zcash_transaction;
  for (const auto& note : pick_result.value().inputs) {
    OrchardInput orchard_input;
    orchard_input.note = note;
    zcash_transaction.orchard_part().inputs.push_back(std::move(orchard_input));
  }
  zcash_transaction.set_fee(pick_result->fee);

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

  // Create shielded change
  if (pick_result->change != 0) {
    OrchardOutput& orchard_output =
        zcash_transaction.orchard_part().outputs.emplace_back();
    orchard_output.value = pick_result->change;
    orchard_output.addr = context_.account_internal_addr.value();
  }

  // Create shielded output
  OrchardOutput& orchard_output =
      zcash_transaction.orchard_part().outputs.emplace_back();
  auto value =
      base::CheckSub<uint64_t>(zcash_transaction.TotalInputsAmount(),
                               zcash_transaction.fee(), pick_result->change);
  if (!value.AssignIfValid(&orchard_output.value)) {
    error_ = l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR);
    ScheduleWorkOnTask();
    return;
  }

  orchard_output.value = value.ValueOrDie();
  orchard_output.addr = receiver_;
  orchard_output.memo = memo_;
  zcash_transaction.orchard_part().anchor_block_height =
      spendable_notes_->anchor_block_id.value();

  auto orchard_unified_addr = GetOrchardUnifiedAddress(
      receiver_, context_.chain_id == mojom::kZCashTestnet);

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
