// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_wallet/browser/zcash/zcash_create_ironwood_to_transparent_transaction_task.h"

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

ZCashCreateIronwoodToTransparentTransactionTask::
    ZCashCreateIronwoodToTransparentTransactionTask(
        std::variant<
            base::PassKey<
                class ZCashCreateIronwoodToTransparentTransactionTaskTest>,
            base::PassKey<ZCashWalletService>> pass_key,
        ZCashWalletService& zcash_wallet_service,
        ZCashActionContext context,
        const std::string& transparent_address,
        uint64_t amount)
    : zcash_wallet_service_(zcash_wallet_service),
      context_(std::move(context)),
      transparent_address_(transparent_address),
      amount_(amount) {}

ZCashCreateIronwoodToTransparentTransactionTask::
    ~ZCashCreateIronwoodToTransparentTransactionTask() = default;

void ZCashCreateIronwoodToTransparentTransactionTask::Start(
    CreateTransactionCallback callback) {
  DCHECK(!callback_);
  callback_ = std::move(callback);
  ScheduleWorkOnTask();
}

void ZCashCreateIronwoodToTransparentTransactionTask::ScheduleWorkOnTask() {
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE,
      base::BindOnce(
          &ZCashCreateIronwoodToTransparentTransactionTask::WorkOnTask,
          weak_ptr_factory_.GetWeakPtr()));
}

void ZCashCreateIronwoodToTransparentTransactionTask::WorkOnTask() {
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

void ZCashCreateIronwoodToTransparentTransactionTask::GetSpendableNotes() {
  if (!context_.account_internal_addr) {
    LOG(ERROR) << "XXXZZZ GetSpendableNotes no internal address provided";
    error_ = "No internal address provided";
    ScheduleWorkOnTask();
    return;
  }
  context_.sync_state->AsyncCall(&OrchardSyncState::GetSpendableNotes)
      .WithArgs(OrchardPool::kIronwood, context_.account_id.Clone(),
                context_.account_internal_addr.value())
      .Then(base::BindOnce(
          &ZCashCreateIronwoodToTransparentTransactionTask::OnGetSpendableNotes,
          weak_ptr_factory_.GetWeakPtr()));
}

void ZCashCreateIronwoodToTransparentTransactionTask::OnGetSpendableNotes(
    base::expected<std::optional<OrchardSyncState::SpendableNotesBundle>,
                   OrchardStorage::Error> result) {
  if (!result.has_value()) {
    LOG(ERROR) << "XXXZZZ OnGetSpendableNotes GetSpendableNotes failed: "
               << result.error().message;
    error_ = result.error().message;
    ScheduleWorkOnTask();
    return;
  }

  if (!result.value()) {
    LOG(ERROR) << "XXXZZZ OnGetSpendableNotes no spendable notes";
    error_ = "No spendable notes";
    ScheduleWorkOnTask();
    return;
  }

  spendable_notes_ = std::move(*result.value());

  if (!spendable_notes_->anchor_block_id) {
    LOG(ERROR) << "XXXZZZ OnGetSpendableNotes failed to select anchor, "
                  "spendable_notes="
               << spendable_notes_->spendable_notes.size();
    error_ = "Failed to select anchor";
    ScheduleWorkOnTask();
    return;
  }

  ScheduleWorkOnTask();
}

void ZCashCreateIronwoodToTransparentTransactionTask::CreateTransaction() {
  LOG(ERROR) << "XXXZZZ CreateTransaction start amount=" << amount_
             << " spendable_notes_=" << !!spendable_notes_;
  CHECK(spendable_notes_);

  ZCashTransaction zcash_transaction;
  zcash_transaction.ConvertToV6();

  // Pick Ironwood inputs.
  auto pick_result =
      PickZCashOrchardInputs(spendable_notes_->spendable_notes, amount_,
                             ZCashTargetOutputType::kTransparent);
  if (!pick_result) {
    LOG(ERROR) << "XXXZZZ CreateTransaction can't pick inputs, amount="
               << amount_
               << " spendable_notes=" << spendable_notes_->spendable_notes.size();
    error_ = "Can't pick inputs";
    ScheduleWorkOnTask();
    return;
  }

  // Add Ironwood inputs to transaction.
  for (const auto& note : pick_result.value().inputs) {
    OrchardInput orchard_input;
    orchard_input.note = note;
    zcash_transaction.v6_part().ironwood.inputs.push_back(
        std::move(orchard_input));
  }
  zcash_transaction.set_fee(pick_result->fee);

  LOG(ERROR) << "XXXZZZ CreateTransaction anchor_block_id="
             << !!spendable_notes_->anchor_block_id
             << " picked_inputs=" << pick_result->inputs.size()
             << " fee=" << pick_result->fee
             << " change=" << pick_result->change;
  CHECK(spendable_notes_->anchor_block_id);
  zcash_transaction.v6_part().ironwood.anchor_block_height =
      spendable_notes_->anchor_block_id.value();

  // Create transparent output for the recipient.
  auto& transparent_output =
      zcash_transaction.transparent_part().outputs.emplace_back();
  transparent_output.address = transparent_address_;

  // Change should be 0 when sending full amount.
  LOG(ERROR) << "XXXZZZ CreateTransaction full_amount="
             << (amount_ == kZCashFullAmount)
             << " change=" << pick_result->change;
  CHECK(!(amount_ == kZCashFullAmount) || (pick_result->change == 0));

  uint64_t actual_send_amount =
      base::CheckSub<uint64_t>(zcash_transaction.TotalInputsAmount(),
                               zcash_transaction.fee(), pick_result->change)
          .ValueOrDie();
  transparent_output.amount = actual_send_amount;
  transparent_output.script_pubkey =
      ZCashAddressToScriptPubkey(
          transparent_output.address,
          IsZCashTestnetKeyring(context_.account_id->keyring_id))
          .value();

  // Create Ironwood change output if needed.
  LOG(ERROR) << "XXXZZZ CreateTransaction account_internal_addr="
             << !!context_.account_internal_addr;
  CHECK(context_.account_internal_addr);
  if (pick_result->change != 0) {
    OrchardOutput& orchard_output =
        zcash_transaction.v6_part().ironwood.outputs.emplace_back();
    orchard_output.value = pick_result->change;
    orchard_output.addr = context_.account_internal_addr.value();
  }

  zcash_transaction.set_amount(actual_send_amount);
  zcash_transaction.set_to(transparent_address_);

  transaction_ = std::move(zcash_transaction);
  ScheduleWorkOnTask();
}

}  // namespace brave_wallet
