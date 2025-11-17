// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_wallet/browser/zcash/zcash_create_orchard_to_transparent_transaction_task.h"

#include <utility>
#include <variant>

#include "base/check.h"
#include "base/logging.h"
#include "base/numerics/checked_math.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_transaction_utils.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "brave/components/brave_wallet/common/zcash_utils.h"
#include "components/grit/brave_components_strings.h"
#include "ui/base/l10n/l10n_util.h"

namespace brave_wallet {

ZCashCreateOrchardToTransparentTransactionTask::
    ZCashCreateOrchardToTransparentTransactionTask(
        std::variant<
            base::PassKey<
                class ZCashCreateOrchardToTransparentTransactionTaskTest>,
            base::PassKey<ZCashWalletService>> pass_key,
        ZCashWalletService& zcash_wallet_service,
        ZCashActionContext context,
        const std::string& transparent_address,
        uint64_t amount)
    : zcash_wallet_service_(zcash_wallet_service),
      context_(std::move(context)),
      transparent_address_(transparent_address),
      amount_(amount) {}

ZCashCreateOrchardToTransparentTransactionTask::
    ~ZCashCreateOrchardToTransparentTransactionTask() = default;

void ZCashCreateOrchardToTransparentTransactionTask::Start(
    CreateTransactionCallback callback) {
  DCHECK(!callback_);
  callback_ = std::move(callback);
  ScheduleWorkOnTask();
}

void ZCashCreateOrchardToTransparentTransactionTask::ScheduleWorkOnTask() {
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE,
      base::BindOnce(
          &ZCashCreateOrchardToTransparentTransactionTask::WorkOnTask,
          weak_ptr_factory_.GetWeakPtr()));
}

void ZCashCreateOrchardToTransparentTransactionTask::WorkOnTask() {
  if (error_) {
    std::move(callback_).Run(base::unexpected(error_.value()));
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

  std::move(callback_).Run(std::move(transaction_.value()));
}

void ZCashCreateOrchardToTransparentTransactionTask::GetSpendableNotes() {
  if (!context_.account_internal_addr) {
    error_ = "No internal address provided";
    ScheduleWorkOnTask();
    return;
  }
  context_.sync_state->AsyncCall(&OrchardSyncState::GetSpendableNotes)
      .WithArgs(context_.account_id.Clone(),
                context_.account_internal_addr.value())
      .Then(base::BindOnce(
          &ZCashCreateOrchardToTransparentTransactionTask::OnGetSpendableNotes,
          weak_ptr_factory_.GetWeakPtr()));
}

void ZCashCreateOrchardToTransparentTransactionTask::OnGetSpendableNotes(
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

void ZCashCreateOrchardToTransparentTransactionTask::CreateTransaction() {
  CHECK(spendable_notes_);

  ZCashTransaction zcash_transaction;

  // Pick Orchard inputs.
  auto pick_result =
      PickZCashOrchardInputs(spendable_notes_->spendable_notes, amount_,
                             ZCashTargetOutputType::kTransparent);
  if (!pick_result) {
    error_ = "Can't pick inputs";
    ScheduleWorkOnTask();
    return;
  }

  // Add Orchard inputs to transaction.
  for (const auto& note : pick_result.value().inputs) {
    OrchardInput orchard_input;
    orchard_input.note = note;
    zcash_transaction.orchard_part().inputs.push_back(std::move(orchard_input));
  }
  zcash_transaction.set_fee(pick_result->fee);

  if (!spendable_notes_->anchor_block_id) {
    error_ = "Failed to select anchor";
    ScheduleWorkOnTask();
    return;
  }

  zcash_transaction.orchard_part().anchor_block_height =
      spendable_notes_->anchor_block_id.value();

  // Create transparent output for the recipient.
  auto& transparent_output =
      zcash_transaction.transparent_part().outputs.emplace_back();
  transparent_output.address = transparent_address_;

  // Change should be 0 when sending full amount.
  CHECK(!(amount_ == kZCashFullAmount) || (pick_result->change == 0));

  // Calculate the amount to send.
  uint64_t actual_send_amount =
      base::CheckSub<uint64_t>(zcash_transaction.TotalInputsAmount(),
                               zcash_transaction.fee(), pick_result->change)
          .ValueOrDie();
  transparent_output.amount = actual_send_amount;
  transparent_output.script_pubkey = ZCashAddressToScriptPubkey(
      transparent_output.address,
      IsZCashTestnetKeyring(context_.account_id->keyring_id));

  // Create Orchard change output if needed.
  CHECK(context_.account_internal_addr);
  if (pick_result->change != 0) {
    OrchardOutput& orchard_output =
        zcash_transaction.orchard_part().outputs.emplace_back();
    orchard_output.value = pick_result->change;
    orchard_output.addr = context_.account_internal_addr.value();
  }

  // Set transaction metadata.
  zcash_transaction.set_amount(actual_send_amount);
  zcash_transaction.set_to(transparent_address_);

  transaction_ = std::move(zcash_transaction);
  ScheduleWorkOnTask();
}

}  // namespace brave_wallet
