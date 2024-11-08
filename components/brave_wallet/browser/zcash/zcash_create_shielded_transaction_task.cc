/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/zcash/zcash_create_shielded_transaction_task.h"

#include <utility>

#include "brave/components/brave_wallet/browser/zcash/zcash_transaction_utils.h"

namespace brave_wallet {

ZCashCreateShieldedTransactionTask::ZCashCreateShieldedTransactionTask(
    ZCashWalletService* zcash_wallet_service,
    const std::string& chain_id,
    const mojom::AccountIdPtr& account_id,
    const OrchardAddrRawPart& receiver,
    std::optional<OrchardMemo> memo,
    uint64_t amount,
    ZCashWalletService::CreateTransactionCallback callback)
    : zcash_wallet_service_(zcash_wallet_service),
      chain_id_(chain_id),
      account_id_(account_id.Clone()),
      receiver_(receiver),
      memo_(memo),
      amount_(amount),
      callback_(std::move(callback)) {}

ZCashCreateShieldedTransactionTask::~ZCashCreateShieldedTransactionTask() {}

void ZCashCreateShieldedTransactionTask::ScheduleWorkOnTask() {
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(&ZCashCreateShieldedTransactionTask::WorkOnTask,
                                weak_ptr_factory_.GetWeakPtr()));
}

void ZCashCreateShieldedTransactionTask::WorkOnTask() {
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

void ZCashCreateShieldedTransactionTask::GetSpendableNotes() {
  zcash_wallet_service_->sync_state_
      .AsyncCall(&OrchardSyncState::GetSpendableNotes)
      .WithArgs(account_id_.Clone())
      .Then(base::BindOnce(
          &ZCashCreateShieldedTransactionTask::OnGetSpendableNotes,
          weak_ptr_factory_.GetWeakPtr()));
}

void ZCashCreateShieldedTransactionTask::OnGetSpendableNotes(
    base::expected<std::vector<OrchardNote>, OrchardStorage::Error> result) {
  if (!result.has_value()) {
    error_ = result.error().message;
    ScheduleWorkOnTask();
    return;
  }

  spendable_notes_ = result.value();
  ScheduleWorkOnTask();
}

void ZCashCreateShieldedTransactionTask::CreateTransaction() {
  CHECK(spendable_notes_);
  auto pick_result = PickZCashOrchardInputs(spendable_notes_.value(), amount_);
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

  // Create shielded change
  if (pick_result->change != 0) {
    OrchardOutput& orchard_output =
        zcash_transaction.orchard_part().outputs.emplace_back();
    orchard_output.value = pick_result->change;
    auto change_addr =
        zcash_wallet_service_->keyring_service_->GetOrchardRawBytes(
            account_id_, mojom::ZCashKeyId::New(account_id_->account_index,
                                                1 /* internal */, 0));
    if (!change_addr) {
      error_ = l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR);
      ScheduleWorkOnTask();
      return;
    }
    orchard_output.addr = *change_addr;
  }

  // Create shielded output
  OrchardOutput& orchard_output =
      zcash_transaction.orchard_part().outputs.emplace_back();
  orchard_output.value = zcash_transaction.TotalInputsAmount() -
                         zcash_transaction.fee() - pick_result->change;
  orchard_output.addr = receiver_;
  orchard_output.memo = memo_;

  auto orchard_unified_addr =
      GetOrchardUnifiedAddress(receiver_, chain_id_ == mojom::kZCashTestnet);
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
