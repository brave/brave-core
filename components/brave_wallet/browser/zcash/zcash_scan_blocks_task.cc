/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/zcash/zcash_scan_blocks_task.h"

#include <algorithm>
#include <utility>

#include "base/check.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_wallet_service.h"
#include "brave/components/brave_wallet/common/common_utils.h"

namespace brave_wallet {

namespace {
constexpr uint32_t kBatchSize = 1024u;
constexpr uint32_t kMaxPendingResultsToInserts = 10u;
}  // namespace

ZCashScanBlocksTask::ZCashScanBlocksTask(
    ZCashActionContext& context,
    ZCashShieldSyncService::OrchardBlockScannerProxy& scanner,
    ZCashScanBlocksTaskObserver observer,
    std::optional<uint32_t> to)
    : context_(context),
      scanner_(scanner),
      observer_(std::move(observer)),
      to_(to) {}

ZCashScanBlocksTask::~ZCashScanBlocksTask() = default;

void ZCashScanBlocksTask::Start() {
  DCHECK(!started_);
  started_ = true;
  ScheduleWorkOnTask();
}

void ZCashScanBlocksTask::ScheduleWorkOnTask() {
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(&ZCashScanBlocksTask::WorkOnTask,
                                weak_ptr_factory_.GetWeakPtr()));
}

void ZCashScanBlocksTask::WorkOnTask() {
  if (finished_) {
    return;
  }

  if (error_) {
    scan_tasks_in_progress_.clear();
    observer_.Run(base::unexpected(*error_));
    finished_ = true;
    return;
  }

  if (!account_meta_) {
    GetAccountMeta();
    return;
  }

  if (!chain_tip_block_) {
    GetChainTip();
    return;
  }

  if (!pending_scan_ranges_) {
    PrepareScanRanges();
    return;
  }

  MaybeScanRanges();
  MaybeInsertResult();
}

void ZCashScanBlocksTask::NotifyObserver() {
  ZCashShieldSyncService::ScanRangeResult scan_ranges_result;
  scan_ranges_result.end_block = end_block_.value();
  scan_ranges_result.start_block = start_block_.value();
  scan_ranges_result.total_ranges = initial_ranges_count_.value();
  scan_ranges_result.ready_ranges = initial_ranges_count_.value() -
                                    pending_scan_ranges_->size() -
                                    scan_tasks_in_progress_.size();
  observer_.Run(std::move(scan_ranges_result));
}

void ZCashScanBlocksTask::PrepareScanRanges() {
  CHECK(account_meta_);
  uint32_t from = account_meta_->latest_scanned_block_id
                      ? account_meta_->latest_scanned_block_id.value() + 1
                      : account_meta_->account_birthday;
  uint32_t to = to_.value_or(chain_tip_block_.value());

  if (from > to || to > chain_tip_block_.value()) {
    error_ = ZCashShieldSyncService::Error{
        ZCashShieldSyncService::ErrorCode::kFailedToUpdateChainTip,
        "Scan range error"};
    ScheduleWorkOnTask();
    return;
  }

  start_block_ = from;
  end_block_ = to;

  initial_ranges_count_ =
      std::ceil(static_cast<double>((to - from + 1)) / kBatchSize);
  pending_scan_ranges_ = std::deque<ScanRange>();
  for (uint32_t i = 0; i < initial_ranges_count_.value(); i++) {
    uint32_t batch_from = from + i * kBatchSize;
    uint32_t batch_count = std::min(kBatchSize, to - batch_from + 1);
    pending_scan_ranges_->push_back(ScanRange{batch_from, batch_count});
  }

  NotifyObserver();
  ScheduleWorkOnTask();
}

void ZCashScanBlocksTask::GetAccountMeta() {
  context_->sync_state->AsyncCall(&OrchardSyncState::GetAccountMeta)
      .WithArgs(context_->account_id.Clone())
      .Then(base::BindOnce(&ZCashScanBlocksTask::OnGetAccountMeta,
                           weak_ptr_factory_.GetWeakPtr()));
}

void ZCashScanBlocksTask::OnGetAccountMeta(
    base::expected<std::optional<OrchardStorage::AccountMeta>,
                   OrchardStorage::Error> result) {
  if (!result.has_value() || !result.value()) {
    error_ = ZCashShieldSyncService::Error{
        ZCashShieldSyncService::ErrorCode::kFailedToRetrieveAccount,
        "Failed to retrieve account"};
    ScheduleWorkOnTask();
    return;
  }

  account_meta_ = **result;
  ScheduleWorkOnTask();
}

void ZCashScanBlocksTask::GetChainTip() {
  context_->zcash_rpc->GetLatestBlock(
      GetNetworkForZCashKeyring(context_->account_id->keyring_id),
      base::BindOnce(&ZCashScanBlocksTask::OnGetChainTip,
                     weak_ptr_factory_.GetWeakPtr()));
}

void ZCashScanBlocksTask::OnGetChainTip(
    base::expected<zcash::mojom::BlockIDPtr, std::string> result) {
  if (!result.has_value()) {
    error_ = ZCashShieldSyncService::Error{
        ZCashShieldSyncService::ErrorCode::kFailedToUpdateChainTip,
        result.error()};
    ScheduleWorkOnTask();
    return;
  }

  chain_tip_block_ = (*result)->height;
  ScheduleWorkOnTask();
}

size_t ZCashScanBlocksTask::ReadyScanTasks() {
  return std::ranges::count_if(scan_tasks_in_progress_,
                               [](auto& task) { return task.finished(); });
}

void ZCashScanBlocksTask::MaybeScanRanges() {
  CHECK(pending_scan_ranges_);
  auto ready_scan_tasks = ReadyScanTasks();
  auto total_scan_tasks = scan_tasks_in_progress_.size();
  auto in_progress_scan_tasks = total_scan_tasks - ready_scan_tasks;

  while (!pending_scan_ranges_->empty() &&
         in_progress_scan_tasks < max_tasks_in_progress_ &&
         ready_scan_tasks < kMaxPendingResultsToInserts) {
    auto scan_range = pending_scan_ranges_->front();
    pending_scan_ranges_->pop_front();
    auto& task = scan_tasks_in_progress_.emplace_back(
        context_.get(), scanner_.get(), scan_range,
        base::BindOnce(&ZCashScanBlocksTask::OnScanningRangeComplete,
                       weak_ptr_factory_.GetWeakPtr()));
    task.Start();
    in_progress_scan_tasks++;
  }
}

void ZCashScanBlocksTask::OnScanningRangeComplete(
    base::expected<void, ZCashShieldSyncService::Error> result) {
  if (!result.has_value()) {
    error_ = result.error();
  }
  ScheduleWorkOnTask();
}

void ZCashScanBlocksTask::MaybeInsertResult() {
  if (scan_tasks_in_progress_.size() != 0 && !inserting_in_progress_) {
    // Since insertion is done sequentally we need to wait until the first scan
    // range in the order is ready.
    auto& front_task = scan_tasks_in_progress_.front();
    if (!front_task.finished()) {
      return;
    }
    inserting_in_progress_ = true;
    context_->sync_state->AsyncCall(&OrchardSyncState::ApplyScanResults)
        .WithArgs(context_->account_id.Clone(), front_task.TakeResult())
        .Then(base::BindOnce(&ZCashScanBlocksTask::OnResultInserted,
                             weak_ptr_factory_.GetWeakPtr(),
                             front_task.scan_range()));
  }
}

void ZCashScanBlocksTask::OnResultInserted(
    ScanRange scan_range,
    base::expected<OrchardStorage::Result, OrchardStorage::Error> result) {
  inserting_in_progress_ = false;
  scan_tasks_in_progress_.pop_front();
  if (!result.has_value()) {
    error_ = ZCashShieldSyncService::Error{
        ZCashShieldSyncService::ErrorCode::kFailedToUpdateDatabase,
        result.error().message};
    ScheduleWorkOnTask();
    return;
  }

  NotifyObserver();
  ScheduleWorkOnTask();
}

}  // namespace brave_wallet
