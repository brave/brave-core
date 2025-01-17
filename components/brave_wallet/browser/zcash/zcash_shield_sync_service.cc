/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/zcash/zcash_shield_sync_service.h"

#include <algorithm>
#include <utility>

#include "base/task/thread_pool.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_scan_blocks_task.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_verify_chain_state_task.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_wallet_service.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "brave/components/brave_wallet/common/zcash_utils.h"

namespace brave_wallet {

namespace {

int GetCode(ZCashShieldSyncService::ErrorCode error) {
  return static_cast<size_t>(error);
}

}  // namespace

ZCashShieldSyncService::OrchardBlockScannerProxy::OrchardBlockScannerProxy(
    OrchardFullViewKey full_view_key) {
  background_block_scanner_.emplace(
      base::ThreadPool::CreateSequencedTaskRunner({base::MayBlock()}),
      full_view_key);
}

ZCashShieldSyncService::OrchardBlockScannerProxy::~OrchardBlockScannerProxy() =
    default;

void ZCashShieldSyncService::OrchardBlockScannerProxy::ScanBlocks(
    OrchardTreeState tree_state,
    std::vector<zcash::mojom::CompactBlockPtr> blocks,
    base::OnceCallback<void(base::expected<OrchardBlockScanner::Result,
                                           OrchardBlockScanner::ErrorCode>)>
        callback) {
  background_block_scanner_.AsyncCall(&OrchardBlockScanner::ScanBlocks)
      .WithArgs(std::move(tree_state), std::move(blocks))
      .Then(std::move(callback));
}

ZCashShieldSyncService::ZCashShieldSyncService(
    ZCashActionContext context,
    const mojom::ZCashAccountShieldBirthdayPtr& account_birthday,
    const OrchardFullViewKey& fvk,
    base::WeakPtr<Observer> observer)
    : context_(std::move(context)),
      account_birthday_(account_birthday.Clone()),
      observer_(std::move(observer)) {
  block_scanner_ = std::make_unique<OrchardBlockScannerProxy>(fvk);
}

ZCashShieldSyncService::~ZCashShieldSyncService() = default;

void ZCashShieldSyncService::SetOrchardBlockScannerProxyForTesting(
    std::unique_ptr<OrchardBlockScannerProxy> block_scanner) {
  block_scanner_ = std::move(block_scanner);
}

void ZCashShieldSyncService::StartSyncing(std::optional<uint32_t> to) {
  to_ = to;
  ScheduleWorkOnTask();
  if (observer_) {
    observer_->OnSyncStart(context_.account_id);
  }
}

mojom::ZCashShieldSyncStatusPtr ZCashShieldSyncService::GetSyncStatus() {
  return current_sync_status_.Clone();
}

void ZCashShieldSyncService::ScheduleWorkOnTask() {
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(&ZCashShieldSyncService::WorkOnTask,
                                weak_ptr_factory_.GetWeakPtr()));
}

void ZCashShieldSyncService::WorkOnTask() {
  if (error_) {
    verify_chain_state_task_.reset();
    scan_blocks_task_.reset();

    if (observer_) {
      observer_->OnSyncError(
          context_.account_id,
          base::NumberToString(GetCode(error_->code)) + ": " + error_->message);
    }
    return;
  }

  if (!account_meta_) {
    GetOrCreateAccount();
    return;
  }

  if (!chain_state_verified_) {
    VerifyChainState();
    return;
  } else {
    verify_chain_state_task_.reset();
  }

  if (!scan_blocks_task_) {
    StartBlockScanning();
    return;
  }

  if (latest_scanned_block_result_ &&
      latest_scanned_block_result_->IsFinished()) {
    scan_blocks_task_.reset();
    if (observer_) {
      observer_->OnSyncStop(context_.account_id);
    }
  }
}

void ZCashShieldSyncService::GetOrCreateAccount() {
  if (account_birthday_->value < kNu5BlockUpdate) {
    error_ =
        Error{ErrorCode::kFailedToInitAccount, "Wrong birthday block height"};
    ScheduleWorkOnTask();
    return;
  }
  sync_state()
      .AsyncCall(&OrchardSyncState::GetAccountMeta)
      .WithArgs(context_.account_id.Clone())
      .Then(base::BindOnce(&ZCashShieldSyncService::OnGetAccountMeta,
                           weak_ptr_factory_.GetWeakPtr()));
}

void ZCashShieldSyncService::OnGetAccountMeta(
    base::expected<std::optional<OrchardStorage::AccountMeta>,
                   OrchardStorage::Error> result) {
  if (!result.has_value()) {
    error_ = Error{ErrorCode::kFailedToInitAccount, "Database error"};
    ScheduleWorkOnTask();
    return;
  }
  if (!result.value()) {
    InitAccount();
    return;
  }
  account_meta_ = **result;
  if (account_meta_->latest_scanned_block_id &&
      (account_meta_->latest_scanned_block_id.value() <
       account_meta_->account_birthday)) {
    error_ = Error{ErrorCode::kFailedToRetrieveAccount, ""};
  }
  ScheduleWorkOnTask();
}

void ZCashShieldSyncService::InitAccount() {
  sync_state()
      .AsyncCall(&OrchardSyncState::RegisterAccount)
      .WithArgs(context_.account_id.Clone(), account_birthday_->value)
      .Then(base::BindOnce(&ZCashShieldSyncService::OnAccountInit,
                           weak_ptr_factory_.GetWeakPtr()));
}

void ZCashShieldSyncService::OnAccountInit(
    base::expected<OrchardStorage::AccountMeta, OrchardStorage::Error> result) {
  if (!result.has_value()) {
    error_ = Error{ErrorCode::kFailedToInitAccount, result.error().message};
  } else {
    account_meta_ = *result;
  }
  ScheduleWorkOnTask();
}

void ZCashShieldSyncService::VerifyChainState() {
  CHECK(!verify_chain_state_task_.get());
  verify_chain_state_task_ = std::make_unique<ZCashVerifyChainStateTask>(
      context_, base::BindOnce(&ZCashShieldSyncService::OnChainStateVerified,
                               weak_ptr_factory_.GetWeakPtr()));
  verify_chain_state_task_->Start();
}

void ZCashShieldSyncService::OnChainStateVerified(
    base::expected<bool, ZCashShieldSyncService::Error> result) {
  if (!result.has_value()) {
    error_ = result.error();
    ScheduleWorkOnTask();
    return;
  }

  if (!result.value()) {
    error_ = Error{ErrorCode::kFailedToVerifyChainState, ""};
    ScheduleWorkOnTask();
    return;
  }

  chain_state_verified_ = true;
  ScheduleWorkOnTask();
}

void ZCashShieldSyncService::StartBlockScanning() {
  CHECK(!scan_blocks_task_);
  scan_blocks_task_ = std::make_unique<ZCashScanBlocksTask>(
      context_, *block_scanner_,
      base::BindRepeating(&ZCashShieldSyncService::OnScanRangeResult,
                          weak_ptr_factory_.GetWeakPtr()),
      to_);
  scan_blocks_task_->Start();
}

void ZCashShieldSyncService::OnScanRangeResult(
    base::expected<ScanRangeResult, ZCashShieldSyncService::Error> result) {
  if (!result.has_value()) {
    error_ = result.error();
    ScheduleWorkOnTask();
    return;
  }

  latest_scanned_block_result_ = result.value();
  UpdateSpendableNotes();
}

uint32_t ZCashShieldSyncService::GetSpendableBalance() {
  CHECK(spendable_notes_.has_value());
  uint32_t balance = 0;
  for (const auto& note : spendable_notes_.value()) {
    balance += note.amount;
  }
  return balance;
}

void ZCashShieldSyncService::UpdateSpendableNotes() {
  spendable_notes_ = std::nullopt;
  sync_state()
      .AsyncCall(&OrchardSyncState::GetSpendableNotes)
      .WithArgs(context_.account_id.Clone())
      .Then(base::BindOnce(&ZCashShieldSyncService::OnGetSpendableNotes,
                           weak_ptr_factory_.GetWeakPtr()));
}

void ZCashShieldSyncService::OnGetSpendableNotes(
    base::expected<std::vector<OrchardNote>, OrchardStorage::Error> result) {
  if (!result.has_value()) {
    error_ = Error{ErrorCode::kFailedToRetrieveSpendableNotes,
                   result.error().message};
    ScheduleWorkOnTask();
    return;
  }

  spendable_notes_ = result.value();

  if (latest_scanned_block_result_) {
    current_sync_status_ = mojom::ZCashShieldSyncStatus::New(
        latest_scanned_block_result_->start_block,
        latest_scanned_block_result_->end_block,
        latest_scanned_block_result_->total_ranges,
        latest_scanned_block_result_->ready_ranges, spendable_notes_->size(),
        GetSpendableBalance());
  } else {
    current_sync_status_ = mojom::ZCashShieldSyncStatus::New(
        latest_scanned_block_.value_or(0), latest_scanned_block_.value_or(0), 0,
        0, spendable_notes_->size(), GetSpendableBalance());
  }

  if (observer_) {
    observer_->OnSyncStatusUpdate(context_.account_id,
                                  current_sync_status_.Clone());
  }

  ScheduleWorkOnTask();
}

ZCashRpc& ZCashShieldSyncService::zcash_rpc() {
  return context_.zcash_rpc.get();
}

base::SequenceBound<OrchardSyncState>& ZCashShieldSyncService::sync_state() {
  return context_.sync_state.get();
}

}  // namespace brave_wallet
