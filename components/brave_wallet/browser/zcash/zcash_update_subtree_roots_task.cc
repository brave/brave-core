/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/zcash/zcash_update_subtree_roots_task.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "brave/components/brave_wallet/browser/zcash/zcash_wallet_service.h"

namespace brave_wallet {

namespace {
constexpr size_t kSubTreeRootsResolveBatchSize = 1024;
}  // namespace

ZCashUpdateSubtreeRootsTask::ZCashUpdateSubtreeRootsTask(
    ZCashShieldSyncService::Context& context,
    ZCashUpdateSubtreeRootsTaskCallback callback)
    : context_(context), callback_(std::move(callback)) {}

ZCashUpdateSubtreeRootsTask::~ZCashUpdateSubtreeRootsTask() = default;

void ZCashUpdateSubtreeRootsTask::Start() {
  context_->sync_state->AsyncCall(&OrchardSyncState::GetLatestShardIndex)
      .WithArgs(context_->account_id.Clone())
      .Then(base::BindOnce(&ZCashUpdateSubtreeRootsTask::OnGetLatestShardIndex,
                           weak_ptr_factory_.GetWeakPtr()));
}

void ZCashUpdateSubtreeRootsTask::OnGetLatestShardIndex(
    base::expected<std::optional<uint32_t>, OrchardStorage::Error> result) {
  if (!result.has_value()) {
    std::move(callback_).Run(false);
    return;
  }

  auto latest_shard_index = result.value();
  auto start = latest_shard_index ? latest_shard_index.value() + 1 : 0;
  context_->zcash_rpc->GetSubtreeRoots(
      context_->chain_id, start, kSubTreeRootsResolveBatchSize,
      base::BindOnce(&ZCashUpdateSubtreeRootsTask::OnGetSubtreeRoots,
                     weak_ptr_factory_.GetWeakPtr(), start));
}

void ZCashUpdateSubtreeRootsTask::GetSubtreeRoots(uint32_t start_index) {
  context_->zcash_rpc->GetSubtreeRoots(
      context_->chain_id, start_index, kSubTreeRootsResolveBatchSize,
      base::BindOnce(&ZCashUpdateSubtreeRootsTask::OnGetSubtreeRoots,
                     weak_ptr_factory_.GetWeakPtr(), start_index));
}

void ZCashUpdateSubtreeRootsTask::OnGetSubtreeRoots(
    uint32_t start_index,
    base::expected<std::vector<zcash::mojom::SubtreeRootPtr>, std::string>
        result) {
  if (!result.has_value()) {
    std::move(callback_).Run(false);
    return;
  }

  std::optional<size_t> next_start_index;
  if (result->size() == kSubTreeRootsResolveBatchSize) {
    next_start_index = start_index + kSubTreeRootsResolveBatchSize;
  }

  context_->sync_state->AsyncCall(&OrchardSyncState::UpdateSubtreeRoots)
      .WithArgs(context_->account_id.Clone(), start_index,
                std::move(result.value()))
      .Then(base::BindOnce(&ZCashUpdateSubtreeRootsTask::OnSubtreeRootsUpdated,
                           weak_ptr_factory_.GetWeakPtr(), next_start_index));
}

void ZCashUpdateSubtreeRootsTask::OnSubtreeRootsUpdated(
    std::optional<size_t> next_start_index,
    base::expected<OrchardStorage::Result, OrchardStorage::Error> result) {
  if (!result.has_value()) {
    std::move(callback_).Run(false);
    return;
  }

  if (next_start_index) {
    GetSubtreeRoots(*next_start_index);
  } else {
    std::move(callback_).Run(true);
  }
}

}  // namespace brave_wallet
