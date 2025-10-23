// Copyright (c) 2024 The Brave Authors. All rights reserved.
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
    LOG(ERROR)
        << "XXXZZZ ZCashCreateOrchardToTransparentTransactionTask error: "
        << error_.value() << ", amount: " << amount_
        << ", to: " << transparent_address_;
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
    LOG(ERROR) << "XXXZZZ ZCashCreateOrchardToTransparentTransactionTask - No "
                  "internal address provided, amount: "
               << amount_ << ", to: " << transparent_address_;
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
    LOG(ERROR) << "XXXZZZ ZCashCreateOrchardToTransparentTransactionTask - "
                  "Failed to get spendable notes: "
               << result.error().message << ", amount: " << amount_
               << ", to: " << transparent_address_;
    error_ = result.error().message;
    ScheduleWorkOnTask();
    return;
  }

  if (!result.value()) {
    LOG(ERROR) << "XXXZZZ ZCashCreateOrchardToTransparentTransactionTask - No "
                  "spendable notes available, amount: "
               << amount_ << ", to: " << transparent_address_;
    error_ = "No spendable notes";
    ScheduleWorkOnTask();
    return;
  }

  spendable_notes_ = std::move(*result.value());

  LOG(ERROR) << "XXXZZZ spendable notes bundle ";
  LOG(ERROR) << "XXXZZZ spendable notes bundle anchor_block_id "
             << spendable_notes_->anchor_block_id.value_or(0);

  for (const auto& note : spendable_notes_->spendable_notes) {
    LOG(ERROR) << "XXXZZZ block_id " << note.block_id;
    LOG(ERROR) << "XXXZZZ addr " << ToHex(note.addr);
    LOG(ERROR) << "XXXZZZ nullifier " << ToHex(note.nullifier);
    LOG(ERROR) << "XXXZZZ amount " << note.amount;
    LOG(ERROR) << "XXXZZZ orchard_commitment_tree_position "
               << note.orchard_commitment_tree_position;
    LOG(ERROR) << "XXXZZZ rho " << ToHex(note.rho);
    LOG(ERROR) << "XXXZZZ seed " << ToHex(note.seed);
  }

  ScheduleWorkOnTask();
}

void ZCashCreateOrchardToTransparentTransactionTask::CreateTransaction() {
  CHECK(spendable_notes_);
  LOG(ERROR) << "XXXZZZ CreateTransaction - Starting Orchard to Transparent "
                "transaction creation, amount: "
             << amount_;

  ZCashTransaction zcash_transaction;

  // Pick Orchard inputs
  auto pick_result =
      PickZCashOrchardInputs(spendable_notes_->spendable_notes, amount_,
                             ZCashTargetOutputType::kTransparent);
  if (!pick_result) {
    LOG(ERROR) << "XXXZZZ ZCashCreateOrchardToTransparentTransactionTask - "
                  "Can't pick inputs, amount: "
               << amount_ << ", to: " << transparent_address_;
    error_ = "Can't pick inputs";
    ScheduleWorkOnTask();
    return;
  }

  // Add Orchard inputs to transaction
  for (const auto& note : pick_result.value().inputs) {
    OrchardInput orchard_input;
    orchard_input.note = note;
    zcash_transaction.orchard_part().inputs.push_back(std::move(orchard_input));
  }
  zcash_transaction.set_fee(pick_result->fee);

  if (!context_.account_internal_addr) {
    LOG(ERROR) << "XXXZZZ ZCashCreateOrchardToTransparentTransactionTask - "
                  "Internal address error, amount: "
               << amount_ << ", to: " << transparent_address_;
    error_ = "Internal address error";
    ScheduleWorkOnTask();
    return;
  }

  if (!spendable_notes_->anchor_block_id) {
    LOG(ERROR) << "XXXZZZ ZCashCreateOrchardToTransparentTransactionTask - "
                  "Failed to select anchor, amount: "
               << amount_ << ", to: " << transparent_address_;
    error_ = "Failed to select anchor";
    ScheduleWorkOnTask();
    return;
  }

  zcash_transaction.orchard_part().anchor_block_height =
      spendable_notes_->anchor_block_id.value();

  // Create transparent output for the recipient
  auto& transparent_output =
      zcash_transaction.transparent_part().outputs.emplace_back();
  transparent_output.address = transparent_address_;

  // Calculate the amount to send
  uint64_t send_amount = amount_;
  if (amount_ == kZCashFullAmount) {
    // For max amount, send everything minups fees and change
    send_amount = zcash_transaction.TotalInputsAmount() -
                  zcash_transaction.fee() - pick_result->change;
  }

  transparent_output.amount = send_amount;
  transparent_output.script_pubkey = ZCashAddressToScriptPubkey(
      transparent_output.address,
      IsZCashTestnetKeyring(context_.account_id->keyring_id));

  // Create Orchard change output if needed
  LOG(ERROR) << "XXXZZZ OrchardToTransparent - pick_result->change: "
             << pick_result->change << ", send_amount: " << send_amount
             << ", total_inputs: " << zcash_transaction.TotalInputsAmount()
             << ", fee: " << zcash_transaction.fee();

  if (pick_result->change != 0) {
    OrchardOutput& orchard_output =
        zcash_transaction.orchard_part().outputs.emplace_back();
    orchard_output.value = pick_result->change;
    orchard_output.addr = context_.account_internal_addr.value();
    LOG(ERROR) << "XXXZZZ OrchardToTransparent - Created change output: "
               << pick_result->change;
  } else {
    LOG(ERROR) << "XXXZZZ OrchardToTransparent - No change, this might cause "
                  "Orchard bundle creation to fail";
  }

  // Set transaction metadata
  zcash_transaction.set_amount(send_amount);
  zcash_transaction.set_to(transparent_address_);

  LOG(ERROR)
      << "XXXZZZ CreateTransaction - Transaction created successfully with "
      << zcash_transaction.orchard_part().inputs.size()
      << " Orchard inputs and "
      << zcash_transaction.orchard_part().outputs.size() << " Orchard outputs";

  transaction_ = std::move(zcash_transaction);
  ScheduleWorkOnTask();
}

}  // namespace brave_wallet
