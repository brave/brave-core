/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/zcash/zcash_scan_blocks_task.h"

#include <algorithm>
#include <utility>

#include "brave/components/brave_wallet/browser/zcash/zcash_wallet_service.h"

namespace brave_wallet {

namespace {
constexpr uint32_t kBatchSize = 1024;
}  // namespace

ZCashScanBlocksTask::ZCashScanBlocksTask(
    ZCashShieldSyncService::Context& context,
    ZCashShieldSyncService::OrchardBlockScannerProxy& scanner,
    ZCashScanBlocksTaskObserver observer,
    std::optional<uint32_t> to)
    : context_(context),
      scanner_(scanner),
      observer_(std::move(observer)),
      to_(to) {}

ZCashScanBlocksTask::~ZCashScanBlocksTask() = default;

void ZCashScanBlocksTask::Start() {
  ScheduleWorkOnTask();
}

void ZCashScanBlocksTask::ScheduleWorkOnTask() {
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(&ZCashScanBlocksTask::WorkOnTask,
                                weak_ptr_factory_.GetWeakPtr()));
}

void ZCashScanBlocksTask::WorkOnTask() {
  if (error_) {
    observer_.Run(base::unexpected(*error_));
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

  if (!scan_ranges_) {
    PrepareScanRanges();
    return;
  }

  if (!scan_ranges_->empty()) {
    ScanRanges();
    return;
  }
}

void ZCashScanBlocksTask::PrepareScanRanges() {
  CHECK(account_meta_);
  uint32_t from = account_meta_->latest_scanned_block_id
                      ? account_meta_->latest_scanned_block_id.value() + 1
                      : account_meta_->account_birthday;
  if (to_ && (chain_tip_block_.value() < to_.value() || to_.value() < from)) {
    error_ = ZCashShieldSyncService::Error{
        ZCashShieldSyncService::ErrorCode::kFailedToUpdateChainTip,
        "Scan range error"};
    ScheduleWorkOnTask();
    return;
  }
  uint32_t to = to_.value_or(chain_tip_block_.value());

  start_block_ = from;
  end_block_ = to;

  initial_ranges_count_ =
      std::ceil(static_cast<double>((to - from + 1)) / kBatchSize);
  scan_ranges_ = std::deque<ScanRange>();
  for (size_t i = 0; i < initial_ranges_count_.value(); i++) {
    scan_ranges_->push_back(ScanRange{
        static_cast<uint32_t>(from + i * kBatchSize),
        std::min(to, static_cast<uint32_t>(from + (i + 1) * kBatchSize))});
  }
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
      context_->chain_id, base::BindOnce(&ZCashScanBlocksTask::OnGetChainTip,
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

void ZCashScanBlocksTask::ScanRanges() {
  CHECK(scan_ranges_);
  auto scan_range = scan_ranges_->front();
  scan_ranges_->pop_front();
  current_block_range_ = std::make_unique<ZCashBlocksBatchScanTask>(
      context_.get(), scanner_.get(), scan_range.from, scan_range.to,
      base::BindOnce(&ZCashScanBlocksTask::OnScanningRangeComplete,
                     weak_ptr_factory_.GetWeakPtr()));
  current_block_range_->Start();
}

void ZCashScanBlocksTask::OnScanningRangeComplete(
    base::expected<bool, ZCashShieldSyncService::Error> result) {
  if (!result.has_value()) {
    error_ = result.error();
    ScheduleWorkOnTask();
    return;
  }
  ZCashShieldSyncService::ScanRangeResult scan_ranges_result;
  scan_ranges_result.end_block = end_block_.value();
  scan_ranges_result.start_block = start_block_.value();
  scan_ranges_result.total_ranges = initial_ranges_count_.value();
  scan_ranges_result.ready_ranges =
      initial_ranges_count_.value() - scan_ranges_.value().size();
  observer_.Run(scan_ranges_result);

  ScanRanges();
}

}  // namespace brave_wallet
