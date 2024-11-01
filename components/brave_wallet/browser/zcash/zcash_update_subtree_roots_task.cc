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
    ZCashShieldSyncService* sync_service,
    ZCashUpdateSubtreeRootsTaskCallback callback)
    : sync_service_(sync_service), callback_(std::move(callback)) {}

ZCashUpdateSubtreeRootsTask::~ZCashUpdateSubtreeRootsTask() {}

void ZCashUpdateSubtreeRootsTask::Start() {
  sync_service_->zcash_wallet_service_->sync_state_
      .AsyncCall(&ZCashOrchardSyncState::GetLatestShardIndex)
      .WithArgs(sync_service_->account_id_.Clone())
      .Then(base::BindOnce(&ZCashUpdateSubtreeRootsTask::OnGetLatestShardIndex,
                           weak_ptr_factory_.GetWeakPtr()));
}

void ZCashUpdateSubtreeRootsTask::OnGetLatestShardIndex(
    base::expected<std::optional<uint32_t>, ZCashOrchardStorage::Error>
        result) {
  if (!result.has_value()) {
    std::move(callback_).Run(false);
    return;
  }

  auto latest_shard_index = result.value();
  auto start = latest_shard_index ? latest_shard_index.value() + 1 : 0;
  sync_service_->zcash_wallet_service_->zcash_rpc_->GetSubtreeRoots(
      sync_service_->chain_id_, start, kSubTreeRootsResolveBatchSize,
      base::BindOnce(&ZCashUpdateSubtreeRootsTask::OnGetSubtreeRoots,
                     weak_ptr_factory_.GetWeakPtr(), start));
}

void ZCashUpdateSubtreeRootsTask::GetSubtreeRoots(uint32_t start_index) {
  sync_service_->zcash_wallet_service_->zcash_rpc_->GetSubtreeRoots(
      sync_service_->chain_id_, start_index, kSubTreeRootsResolveBatchSize,
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

  sync_service_->zcash_wallet_service_->sync_state_
      .AsyncCall(&ZCashOrchardSyncState::UpdateSubtreeRoots)
      .WithArgs(sync_service_->account_id_.Clone(), start_index,
                std::move(result.value()))
      .Then(base::BindOnce(&ZCashUpdateSubtreeRootsTask::OnSubtreeRootsUpdated,
                           weak_ptr_factory_.GetWeakPtr(), next_start_index));
}

void ZCashUpdateSubtreeRootsTask::OnSubtreeRootsUpdated(
    std::optional<size_t> next_start_index,
    base::expected<bool, ZCashOrchardStorage::Error> result) {
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
