/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/zcash/zcash_resolve_balance_task.h"

#include <utility>

#include "base/check.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "components/grit/brave_components_strings.h"
#include "ui/base/l10n/l10n_util.h"

namespace brave_wallet {

ZCashResolveBalanceTask::ZCashResolveBalanceTask(
    base::PassKey<ZCashWalletService> pass_key,
    ZCashWalletService& zcash_wallet_service,
    ZCashActionContext context)
    : zcash_wallet_service_(zcash_wallet_service),
      context_(std::move(context)) {}

ZCashResolveBalanceTask::~ZCashResolveBalanceTask() = default;

void ZCashResolveBalanceTask::Start(ZCashResolveBalanceTaskCallback callback) {
  DCHECK(!callback_);
  callback_ = std::move(callback);
  ScheduleWorkOnTask();
}

void ZCashResolveBalanceTask::ScheduleWorkOnTask() {
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(&ZCashResolveBalanceTask::WorkOnTask,
                                weak_ptr_factory_.GetWeakPtr()));
}

void ZCashResolveBalanceTask::WorkOnTask() {
  if (error_) {
    std::move(callback_).Run(base::unexpected(error_.value()));
    return;
  }

  if (!discovery_result_) {
    zcash_wallet_service_->RunDiscovery(
        context_.account_id.Clone(),
        base::BindOnce(&ZCashResolveBalanceTask::OnDiscoveryDoneForBalance,
                       weak_ptr_factory_.GetWeakPtr()));
    return;
  }

  if (!utxo_map_) {
    zcash_wallet_service_->GetUtxos(
        context_.account_id.Clone(),
        base::BindOnce(&ZCashResolveBalanceTask::OnUtxosResolvedForBalance,
                       weak_ptr_factory_.GetWeakPtr()));
    return;
  }

  if (IsZCashShieldedTransactionsEnabled() && !orchard_notes_result_) {
    zcash_wallet_service_->sync_state()
        .AsyncCall(&OrchardSyncState::GetSpendableNotes)
        .WithArgs(OrchardPool::kOrchard, context_.account_id.Clone(),
                  context_.account_internal_addr.value())
        .Then(base::BindOnce(&ZCashResolveBalanceTask::OnGetSpendableNotes,
                             weak_ptr_factory_.GetWeakPtr(),
                             OrchardPool::kOrchard));
    return;
  }

  if (IsZCashIronwoodEnabled() && !ironwood_notes_result_) {
    zcash_wallet_service_->sync_state()
        .AsyncCall(&OrchardSyncState::GetSpendableNotes)
        .WithArgs(OrchardPool::kIronwood, context_.account_id.Clone(),
                  context_.account_internal_addr.value())
        .Then(base::BindOnce(&ZCashResolveBalanceTask::OnGetSpendableNotes,
                             weak_ptr_factory_.GetWeakPtr(),
                             OrchardPool::kIronwood));
    return;
  }

  if (!result_) {
    CreateBalance();
    return;
  }

  std::move(callback_).Run(base::ok(std::move(result_.value())));
}

void ZCashResolveBalanceTask::OnGetSpendableNotes(
    OrchardPool pool,
    base::expected<std::optional<OrchardSyncState::SpendableNotesBundle>,
                   OrchardStorage::Error> result) {
  if (!result.has_value()) {
    error_ = result.error().message;
    ScheduleWorkOnTask();
    return;
  }
  auto& bundle = result.value();
  auto& target = (pool == OrchardPool::kIronwood) ? ironwood_notes_result_
                                                  : orchard_notes_result_;
  target = bundle ? std::move(bundle.value())
                  : OrchardSyncState::SpendableNotesBundle();
  ScheduleWorkOnTask();
}

void ZCashResolveBalanceTask::CreateBalance() {
  auto result = mojom::ZCashBalance::New();
  result->transparent_balance = 0;
  result->orchard_balance = 0;
  result->orchard_pending_balance = 0;
  result->ironwood_balance = 0;
  result->ironwood_pending_balance = 0;

  for (const auto& by_addr : utxo_map_.value()) {
    uint64_t balance_by_addr = 0;
    for (const auto& by_utxo : by_addr.second) {
      balance_by_addr += by_utxo->value_zat;
    }
    result->transparent_balance += balance_by_addr;
    result->balances[by_addr.first] = balance_by_addr;
  }
  result->total_balance = result->transparent_balance;

  auto accumulate = [](const OrchardSyncState::SpendableNotesBundle& bundle,
                       uint64_t& spendable_out, uint64_t& pending_out) -> bool {
    uint64_t spendable = 0, pending = 0;
    for (const auto& note : bundle.all_notes) {
      pending += note.amount;
    }
    for (const auto& note : bundle.spendable_notes) {
      spendable += note.amount;
    }
    if (pending < spendable) {
      return false;
    }
    spendable_out = spendable;
    pending_out = pending - spendable;
    return true;
  };

  if (orchard_notes_result_) {
    if (!accumulate(*orchard_notes_result_, result->orchard_balance,
                    result->orchard_pending_balance)) {
      error_ = "Pending balance error";
      ScheduleWorkOnTask();
      return;
    }
    result->total_balance += result->orchard_balance;
  }
  if (ironwood_notes_result_) {
    if (!accumulate(*ironwood_notes_result_, result->ironwood_balance,
                    result->ironwood_pending_balance)) {
      error_ = "Pending balance error";
      ScheduleWorkOnTask();
      return;
    }
    result->total_balance += result->ironwood_balance;
  }

  result_ = std::move(result);
  ScheduleWorkOnTask();
}

void ZCashResolveBalanceTask::OnDiscoveryDoneForBalance(
    ZCashWalletService::RunDiscoveryResult discovery_result) {
  if (!discovery_result.has_value()) {
    error_ = l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR);
    ScheduleWorkOnTask();
    return;
  }

  discovery_result_ = std::move(discovery_result);
  ScheduleWorkOnTask();
}

void ZCashResolveBalanceTask::OnUtxosResolvedForBalance(
    base::expected<ZCashWalletService::UtxoMap, std::string> result) {
  if (!result.has_value()) {
    error_ = result.error();
    ScheduleWorkOnTask();
    return;
  }

  utxo_map_ = std::move(result.value());
  ScheduleWorkOnTask();
}

}  // namespace brave_wallet
