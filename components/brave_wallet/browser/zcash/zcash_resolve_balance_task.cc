/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/zcash/zcash_resolve_balance_task.h"

#include <utility>

#include "brave/components/brave_wallet/common/common_utils.h"
#include "components/grit/brave_components_strings.h"
#include "ui/base/l10n/l10n_util.h"

namespace brave_wallet {

ZCashResolveBalanceTask::ZCashResolveBalanceTask(
    base::PassKey<class ZCashWalletService> pass_key,
    ZCashWalletService& zcash_wallet_service,
    const std::string& chain_id,
    mojom::AccountIdPtr account_id,
    ZCashResolveBalanceTaskCallback callback)
    : zcash_wallet_service_(zcash_wallet_service),
      chain_id_(chain_id),
      account_id_(std::move(account_id)),
      callback_(std::move(callback)) {}

ZCashResolveBalanceTask::~ZCashResolveBalanceTask() = default;

void ZCashResolveBalanceTask::Start() {
  CHECK(!started_);
  started_ = true;
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
    zcash_wallet_service_->ResolveBalanceTaskDone(this);
    return;
  }

  if (!discovery_result_) {
    zcash_wallet_service_->RunDiscovery(
        account_id_.Clone(),
        base::BindOnce(&ZCashResolveBalanceTask::OnDiscoveryDoneForBalance,
                       weak_ptr_factory_.GetWeakPtr()));
    return;
  }

  if (!utxo_map_) {
    zcash_wallet_service_->GetUtxos(
        chain_id_, account_id_.Clone(),
        base::BindOnce(&ZCashResolveBalanceTask::OnUtxosResolvedForBalance,
                       weak_ptr_factory_.GetWeakPtr()));
    return;
  }

#if BUILDFLAG(ENABLE_ORCHARD)
  if (IsZCashShieldedTransactionsEnabled()) {
    if (!orchard_notes_) {
      zcash_wallet_service_->sync_state()
          .AsyncCall(&OrchardSyncState::GetSpendableNotes)
          .WithArgs(account_id_.Clone())
          .Then(base::BindOnce(&ZCashResolveBalanceTask::OnGetSpendableNotes,
                               weak_ptr_factory_.GetWeakPtr()));
      return;
    }
  }
#endif  // BUILDFLAG(ENABLE_ORCHARD)

  if (!result_) {
    CreateBalance();
    return;
  }

  std::move(callback_).Run(base::ok(std::move(result_.value())));
  zcash_wallet_service_->ResolveBalanceTaskDone(this);
}

#if BUILDFLAG(ENABLE_ORCHARD)
void ZCashResolveBalanceTask::OnGetSpendableNotes(
    base::expected<std::vector<OrchardNote>, OrchardStorage::Error> result) {
  if (!result.has_value()) {
    error_ = result.error().message;
    ScheduleWorkOnTask();
    return;
  }

  orchard_notes_ = std::move(result.value());
  ScheduleWorkOnTask();
}

#endif  // BUILDFLAG(ENABLE_ORCHARD)

void ZCashResolveBalanceTask::CreateBalance() {
  auto result = mojom::ZCashBalance::New();
  result->transparent_balance = 0;
  result->shielded_balance = 0;

  for (const auto& by_addr : utxo_map_.value()) {
    uint64_t balance_by_addr = 0;
    for (const auto& by_utxo : by_addr.second) {
      balance_by_addr += by_utxo->value_zat;
    }
    result->transparent_balance += balance_by_addr;
    result->balances[by_addr.first] = balance_by_addr;
  }

  result->total_balance = result->transparent_balance;

#if BUILDFLAG(ENABLE_ORCHARD)
  if (orchard_notes_) {
    for (const auto& note : orchard_notes_.value()) {
      result->shielded_balance += note.amount;
    }
    result->total_balance += result->shielded_balance;
  }
#endif

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
