// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_wallet/browser/zcash/zcash_blocks_batch_scan_task.h"

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

#include "base/check.h"
#include "base/check_op.h"
#include "base/containers/extend.h"
#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_wallet_service.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "brave/components/brave_wallet/common/hex_utils.h"

namespace brave_wallet {

namespace {
constexpr uint32_t kBlockDownloadBatchSize = 10u;
}

bool ZCashBlocksBatchScanTask::ScanRange::operator==(
    const ScanRange& other) const = default;

ZCashBlocksBatchScanTask::ZCashBlocksBatchScanTask(
    ZCashActionContext& context,
    ZCashShieldSyncService::OrchardBlockScannerProxy& scanner,
    ScanRange scan_range,
    ZCashBlocksBatchScanTaskCallback callback)
    : context_(context),
      scanner_(scanner),
      scan_range_(scan_range),
      callback_(std::move(callback)) {
  CHECK_GT(scan_range_.from, kNu5BlockUpdate);
  frontier_block_height_ = scan_range_.from - 1;
}

ZCashBlocksBatchScanTask::~ZCashBlocksBatchScanTask() = default;

void ZCashBlocksBatchScanTask::Start() {
  DCHECK(!started_);
  started_ = true;
  ScheduleWorkOnTask();
}

void ZCashBlocksBatchScanTask::FinishWithResult(
    base::expected<void, ZCashShieldSyncService::Error> result) {
  DCHECK(!finished_);
  finished_ = true;
  std::move(callback_).Run(std::move(result));
}

void ZCashBlocksBatchScanTask::ScheduleWorkOnTask() {
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(&ZCashBlocksBatchScanTask::WorkOnTask,
                                weak_ptr_factory_.GetWeakPtr()));
}

void ZCashBlocksBatchScanTask::WorkOnTask() {
  if (error_) {
    FinishWithResult(base::unexpected(*error_));
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
                        downloaded_blocks_->size() != scan_range_.count)) {
    DownloadBlocks();
    return;
  }

  if (!scan_result_) {
    ScanBlocks();
    return;
  }

  FinishWithResult(base::ok());
}

void ZCashBlocksBatchScanTask::GetFrontierTreeState() {
  auto block_id = zcash::mojom::BlockID::New(frontier_block_height_,
                                             std::vector<uint8_t>({}));
  context_->zcash_rpc->GetTreeState(
      GetNetworkForZCashKeyring(context_->account_id->keyring_id),
      std::move(block_id),
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
      GetNetworkForZCashKeyring(context_->account_id->keyring_id),
      frontier_block_height_, frontier_block_height_,
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
  uint32_t downloaded_blocks_count =
      downloaded_blocks_ ? downloaded_blocks_->size() : 0;
  uint32_t remaining_blocks = scan_range_.count - downloaded_blocks_count;
  uint32_t expected_size = std::min(kBlockDownloadBatchSize, remaining_blocks);
  uint32_t start_index = scan_range_.from + downloaded_blocks_count;
  uint32_t end_index = start_index + expected_size - 1;

  context_->zcash_rpc->GetCompactBlocks(
      GetNetworkForZCashKeyring(context_->account_id->keyring_id), start_index,
      end_index,
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
        "Expected block count doesn't match actual"};
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

  OrchardTreeState tree_state;
  {
    std::vector<uint8_t> frontier_bytes;
    // Allow an empty orchardTree to simplify testing.
    // If the tree is empty, the frontier won't be inserted.
    // Otherwise, the frontier must be recalculated each time
    // based on the previous state and newly added leaves,
    // which makes the process more complex.
    if (!frontier_tree_state_.value()->orchardTree.empty() &&
        !base::HexStringToBytes(frontier_tree_state_.value()->orchardTree,
                                &frontier_bytes)) {
      error_ = ZCashShieldSyncService::Error{
          ZCashShieldSyncService::ErrorCode::kScannerError,
          "Failed to parse tree state"};
      ScheduleWorkOnTask();
      return;
    }

    tree_state.block_height = frontier_block_.value()->height;
    tree_state.tree_size =
        frontier_block_.value()->chain_metadata->orchard_commitment_tree_size;
    tree_state.frontier = std::move(frontier_bytes);
  }

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
        ZCashShieldSyncService::ErrorCode::kScannerError,
        "Failed to scan blocks"};
    ScheduleWorkOnTask();
    return;
  }

  scan_result_ = std::move(result.value());
  ScheduleWorkOnTask();
}

OrchardBlockScanner::Result ZCashBlocksBatchScanTask::TakeResult() {
  CHECK(scan_result_.has_value());
  return std::move(scan_result_.value());
}

}  // namespace brave_wallet
