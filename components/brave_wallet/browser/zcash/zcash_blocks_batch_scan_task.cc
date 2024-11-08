// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_wallet/browser/zcash/zcash_blocks_batch_scan_task.h"

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

#include "base/containers/extend.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_wallet_service.h"
#include "brave/components/brave_wallet/common/hex_utils.h"

namespace brave_wallet {

namespace {
constexpr uint32_t kBlockDownloadBatchSize = 10u;
}

ZCashBlocksBatchScanTask::ZCashBlocksBatchScanTask(
    ZCashShieldSyncService::Context& context,
    ZCashShieldSyncService::OrchardBlockScannerProxy& scanner,
    uint32_t from,
    uint32_t to,
    ZCashBlocksBatchScanTaskCallback callback)
    : context_(context),
      scanner_(scanner),
      from_(from),
      to_(to),
      callback_(std::move(callback)) {
  frontier_block_height_ = from - 1;
}

ZCashBlocksBatchScanTask::~ZCashBlocksBatchScanTask() = default;

void ZCashBlocksBatchScanTask::Start() {
  ScheduleWorkOnTask();
}

void ZCashBlocksBatchScanTask::ScheduleWorkOnTask() {
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(&ZCashBlocksBatchScanTask::WorkOnTask,
                                weak_ptr_factory_.GetWeakPtr()));
}

void ZCashBlocksBatchScanTask::WorkOnTask() {
  if (error_) {
    std::move(callback_).Run(base::unexpected(*error_));
    return;
  }

  if (!frontier_tree_state_) {
    GetFrontierTreeState();
    return;
  }

  if (!frontier_block_) {
    GetFrontierBlock();
    return;
  }

  if (!scan_result_ && (!downloaded_blocks_ ||
                        downloaded_blocks_->size() != (to_ - from_ + 1))) {
    DownloadBlocks();
    return;
  }

  if (!scan_result_) {
    ScanBlocks();
    return;
  }

  if (!database_updated_) {
    UpdateDatabase();
    return;
  }

  std::move(callback_).Run(true);
}

void ZCashBlocksBatchScanTask::GetFrontierTreeState() {
  auto block_id = zcash::mojom::BlockID::New(frontier_block_height_,
                                             std::vector<uint8_t>({}));
  context_->zcash_rpc->GetTreeState(
      context_->chain_id, std::move(block_id),
      base::BindOnce(&ZCashBlocksBatchScanTask::OnGetFrontierTreeState,
                     weak_ptr_factory_.GetWeakPtr()));
}

void ZCashBlocksBatchScanTask::OnGetFrontierTreeState(
    base::expected<zcash::mojom::TreeStatePtr, std::string> result) {
  if (!result.has_value() || !result.value()) {
    error_ = ZCashShieldSyncService::Error{
        ZCashShieldSyncService::ErrorCode::kFailedToReceiveTreeState,
        base::StrCat({"Frontier tree state failed, ", result.error()})};
    ScheduleWorkOnTask();
    return;
  }
  frontier_tree_state_ = result.value().Clone();
  ScheduleWorkOnTask();
}

void ZCashBlocksBatchScanTask::GetFrontierBlock() {
  context_->zcash_rpc->GetCompactBlocks(
      context_->chain_id, frontier_block_height_, frontier_block_height_,
      base::BindOnce(&ZCashBlocksBatchScanTask::OnGetFrontierBlock,
                     weak_ptr_factory_.GetWeakPtr()));
}

void ZCashBlocksBatchScanTask::OnGetFrontierBlock(
    base::expected<std::vector<zcash::mojom::CompactBlockPtr>, std::string>
        result) {
  if (!result.has_value() || result.value().size() != 1) {
    error_ = ZCashShieldSyncService::Error{
        ZCashShieldSyncService::ErrorCode::kFailedToDownloadBlocks,
        result.error()};
    ScheduleWorkOnTask();
    return;
  }

  frontier_block_ = std::move(result.value()[0]);
  ScheduleWorkOnTask();
}

void ZCashBlocksBatchScanTask::DownloadBlocks() {
  uint32_t start_index =
      downloaded_blocks_ ? from_ + downloaded_blocks_->size() : from_;
  uint32_t end_index = std::min(start_index + kBlockDownloadBatchSize - 1, to_);
  auto expected_size = end_index - start_index + 1;

  context_->zcash_rpc->GetCompactBlocks(
      context_->chain_id, start_index, end_index,
      base::BindOnce(&ZCashBlocksBatchScanTask::OnBlocksDownloaded,
                     weak_ptr_factory_.GetWeakPtr(), expected_size));
}

void ZCashBlocksBatchScanTask::OnBlocksDownloaded(
    size_t expected_size,
    base::expected<std::vector<zcash::mojom::CompactBlockPtr>, std::string>
        result) {
  CHECK(frontier_block_);
  CHECK(frontier_tree_state_);
  if (!result.has_value()) {
    error_ = ZCashShieldSyncService::Error{
        ZCashShieldSyncService::ErrorCode::kFailedToDownloadBlocks,
        result.error()};
    ScheduleWorkOnTask();
    return;
  }

  if (expected_size != result.value().size()) {
    error_ = ZCashShieldSyncService::Error{
        ZCashShieldSyncService::ErrorCode::kFailedToDownloadBlocks,
        "Expected blocks count doesn't match actual"};
    ScheduleWorkOnTask();
    return;
  }

  if (!downloaded_blocks_) {
    downloaded_blocks_ = std::vector<zcash::mojom::CompactBlockPtr>();
  }
  base::Extend(downloaded_blocks_.value(), std::move(result.value()));
  ScheduleWorkOnTask();
}

void ZCashBlocksBatchScanTask::ScanBlocks() {
  if (!downloaded_blocks_ || downloaded_blocks_->empty()) {
    error_ = ZCashShieldSyncService::Error{
        ZCashShieldSyncService::ErrorCode::kScannerError, "No blocks to scan"};
    ScheduleWorkOnTask();
    return;
  }

  if (!frontier_block_.value() || !frontier_tree_state_.value() ||
      !frontier_block_.value()->chain_metadata) {
    error_ = ZCashShieldSyncService::Error{
        ZCashShieldSyncService::ErrorCode::kScannerError, "Frontier error"};
    ScheduleWorkOnTask();
    return;
  }

  auto frontier_bytes = PrefixedHexStringToBytes(
      base::StrCat({"0x", frontier_tree_state_.value()->orchardTree}));

  if (!frontier_bytes) {
    error_ = ZCashShieldSyncService::Error{
        ZCashShieldSyncService::ErrorCode::kScannerError,
        "Failed to parse tree state"};
    ScheduleWorkOnTask();
    return;
  }

  OrchardTreeState tree_state;
  tree_state.block_height = frontier_block_.value()->height;
  tree_state.tree_size =
      frontier_block_.value()->chain_metadata->orchard_commitment_tree_size;
  tree_state.frontier = *frontier_bytes;

  latest_scanned_block_ = downloaded_blocks_->back().Clone();

  scanner_->ScanBlocks(
      std::move(tree_state), std::move(downloaded_blocks_.value()),
      base::BindOnce(&ZCashBlocksBatchScanTask::OnBlocksScanned,
                     weak_ptr_factory_.GetWeakPtr()));
}

void ZCashBlocksBatchScanTask::OnBlocksScanned(
    base::expected<OrchardBlockScanner::Result, OrchardBlockScanner::ErrorCode>
        result) {
  if (!result.has_value()) {
    error_ = ZCashShieldSyncService::Error{
        ZCashShieldSyncService::ErrorCode::kScannerError, ""};
    ScheduleWorkOnTask();
    return;
  }

  scan_result_ = std::move(result.value());
  ScheduleWorkOnTask();
}

void ZCashBlocksBatchScanTask::UpdateDatabase() {
  CHECK(scan_result_.has_value());
  auto latest_scanned_block_hash = ToHex((*latest_scanned_block_)->hash);
  auto latest_scanned_block_height = (*latest_scanned_block_)->height;

  context_->sync_state->AsyncCall(&OrchardSyncState::ApplyScanResults)
      .WithArgs(context_->account_id.Clone(), std::move(scan_result_.value()),
                latest_scanned_block_height, latest_scanned_block_hash)
      .Then(base::BindOnce(&ZCashBlocksBatchScanTask::OnDatabaseUpdated,
                           weak_ptr_factory_.GetWeakPtr()));
}

void ZCashBlocksBatchScanTask::OnDatabaseUpdated(
    base::expected<OrchardStorage::Result, OrchardStorage::Error> result) {
  if (!result.has_value()) {
    error_ = ZCashShieldSyncService::Error{
        ZCashShieldSyncService::ErrorCode::kFailedToUpdateDatabase,
        result.error().message};
    ScheduleWorkOnTask();
    return;
  }
  database_updated_ = true;
  ScheduleWorkOnTask();
}

}  // namespace brave_wallet
