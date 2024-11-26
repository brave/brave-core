/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/internal/orchard_storage/orchard_shard_tree_delegate.h"

#include "brave/components/brave_wallet/browser/internal/orchard_storage/zcash_orchard_storage.h"

namespace brave_wallet {

OrchardTreeState::OrchardTreeState() = default;
OrchardTreeState::~OrchardTreeState() = default;
OrchardTreeState::OrchardTreeState(const OrchardTreeState&) = default;

OrchardCheckpoint::OrchardCheckpoint() = default;
OrchardCheckpoint::OrchardCheckpoint(CheckpointTreeState tree_state_position,
                                     std::vector<uint32_t> marks_removed)
    : tree_state_position(tree_state_position),
      marks_removed(std::move(marks_removed)) {}
OrchardCheckpoint::~OrchardCheckpoint() {}
OrchardCheckpoint::OrchardCheckpoint(const OrchardCheckpoint& other) = default;
OrchardCheckpoint& OrchardCheckpoint::operator=(
    const OrchardCheckpoint& other) = default;
OrchardCheckpoint::OrchardCheckpoint(OrchardCheckpoint&& other) = default;
OrchardCheckpoint& OrchardCheckpoint::operator=(OrchardCheckpoint&& other) =
    default;

OrchardCheckpointBundle::OrchardCheckpointBundle(uint32_t checkpoint_id,
                                                 OrchardCheckpoint checkpoint)
    : checkpoint_id(checkpoint_id), checkpoint(std::move(checkpoint)) {}
OrchardCheckpointBundle::~OrchardCheckpointBundle() = default;
OrchardCheckpointBundle::OrchardCheckpointBundle(
    const OrchardCheckpointBundle& other) = default;
OrchardCheckpointBundle& OrchardCheckpointBundle::operator=(
    const OrchardCheckpointBundle& other) = default;
OrchardCheckpointBundle::OrchardCheckpointBundle(
    OrchardCheckpointBundle&& other) = default;
OrchardCheckpointBundle& OrchardCheckpointBundle::operator=(
    OrchardCheckpointBundle&& other) = default;

OrchardShard::OrchardShard() = default;
OrchardShard::OrchardShard(OrchardShardAddress address,
                           std::optional<OrchardShardRootHash> root_hash,
                           std::vector<uint8_t> shard_data)
    : address(std::move(address)),
      root_hash(std::move(root_hash)),
      shard_data(std::move(shard_data)) {}
OrchardShard::~OrchardShard() = default;
OrchardShard::OrchardShard(const OrchardShard& other) = default;
OrchardShard& OrchardShard::operator=(const OrchardShard& other) = default;
OrchardShard::OrchardShard(OrchardShard&& other) = default;
OrchardShard& OrchardShard::operator=(OrchardShard&& other) = default;

OrchardShardTreeDelegate::OrchardShardTreeDelegate(
    const mojom::AccountIdPtr& account_id,
    ZCashOrchardStorage& storage)
    : account_id_(account_id.Clone()), storage_(storage) {}

OrchardShardTreeDelegate::~OrchardShardTreeDelegate() = default;

base::expected<std::optional<OrchardShardTreeCap>,
               OrchardShardTreeDelegate::Error>
OrchardShardTreeDelegate::GetCap() const {
  auto result = storage_->GetCap(account_id_);
  if (!result.has_value()) {
    return base::unexpected(OrchardShardTreeDelegate::Error::kStorageError);
  }
  return std::move(*result);
}

base::expected<bool, OrchardShardTreeDelegate::Error>
OrchardShardTreeDelegate::PutCap(const OrchardShardTreeCap& cap) {
  auto result = storage_->PutCap(account_id_, cap);
  if (!result.has_value()) {
    return base::unexpected(OrchardShardTreeDelegate::Error::kStorageError);
  }
  return *result;
}

base::expected<bool, OrchardShardTreeDelegate::Error>
OrchardShardTreeDelegate::Truncate(uint32_t block_height) {
  auto result = storage_->TruncateShards(account_id_, block_height);
  if (!result.has_value()) {
    return base::unexpected(OrchardShardTreeDelegate::Error::kStorageError);
  }
  return *result;
}

base::expected<std::optional<uint32_t>, OrchardShardTreeDelegate::Error>
OrchardShardTreeDelegate::GetLatestShardIndex() const {
  auto result = storage_->GetLatestShardIndex(account_id_);
  if (!result.has_value()) {
    return base::unexpected(OrchardShardTreeDelegate::Error::kStorageError);
  }
  return *result;
}

base::expected<bool, OrchardShardTreeDelegate::Error>
OrchardShardTreeDelegate::PutShard(const OrchardShard& shard) {
  auto result = storage_->PutShard(account_id_, shard);
  if (!result.has_value()) {
    return base::unexpected(OrchardShardTreeDelegate::Error::kStorageError);
  }
  return *result;
}

base::expected<std::optional<OrchardShard>, OrchardShardTreeDelegate::Error>
OrchardShardTreeDelegate::GetShard(const OrchardShardAddress& address) const {
  auto result = storage_->GetShard(account_id_, address);
  if (!result.has_value()) {
    return base::unexpected(OrchardShardTreeDelegate::Error::kStorageError);
  }
  return std::move(*result);
}

base::expected<std::optional<OrchardShard>, OrchardShardTreeDelegate::Error>
OrchardShardTreeDelegate::LastShard(uint8_t shard_height) const {
  auto result = storage_->LastShard(account_id_, shard_height);
  if (!result.has_value()) {
    return base::unexpected(OrchardShardTreeDelegate::Error::kStorageError);
  }
  return std::move(*result);
}

base::expected<size_t, OrchardShardTreeDelegate::Error>
OrchardShardTreeDelegate::CheckpointCount() const {
  auto result = storage_->CheckpointCount(account_id_);
  if (!result.has_value()) {
    return base::unexpected(OrchardShardTreeDelegate::Error::kStorageError);
  }
  return *result;
}

base::expected<std::optional<uint32_t>, OrchardShardTreeDelegate::Error>
OrchardShardTreeDelegate::MinCheckpointId() const {
  auto result = storage_->MinCheckpointId(account_id_);
  if (!result.has_value()) {
    return base::unexpected(OrchardShardTreeDelegate::Error::kStorageError);
  }
  return std::move(result.value());
}

base::expected<std::optional<uint32_t>, OrchardShardTreeDelegate::Error>
OrchardShardTreeDelegate::MaxCheckpointId() const {
  auto result = storage_->MaxCheckpointId(account_id_);
  if (!result.has_value()) {
    return base::unexpected(OrchardShardTreeDelegate::Error::kStorageError);
  }
  return std::move(result.value());
}

base::expected<std::optional<uint32_t>, OrchardShardTreeDelegate::Error>
OrchardShardTreeDelegate::GetCheckpointAtDepth(uint32_t depth) const {
  auto result = storage_->GetCheckpointAtDepth(account_id_, depth);
  if (!result.has_value()) {
    return base::unexpected(OrchardShardTreeDelegate::Error::kStorageError);
  }
  return std::move(result.value());
}

base::expected<std::optional<OrchardCheckpointBundle>,
               OrchardShardTreeDelegate::Error>
OrchardShardTreeDelegate::GetCheckpoint(uint32_t checkpoint_id) const {
  auto result = storage_->GetCheckpoint(account_id_, checkpoint_id);
  if (!result.has_value()) {
    return base::unexpected(OrchardShardTreeDelegate::Error::kStorageError);
  }
  return std::move(result.value());
}

base::expected<std::vector<OrchardCheckpointBundle>,
               OrchardShardTreeDelegate::Error>
OrchardShardTreeDelegate::GetCheckpoints(size_t limit) const {
  auto result = storage_->GetCheckpoints(account_id_, limit);
  if (!result.has_value()) {
    return base::unexpected(OrchardShardTreeDelegate::Error::kStorageError);
  }
  return std::move(result.value());
}

base::expected<bool, OrchardShardTreeDelegate::Error>
OrchardShardTreeDelegate::AddCheckpoint(uint32_t id,
                                        const OrchardCheckpoint& checkpoint) {
  auto result = storage_->AddCheckpoint(account_id_, id, checkpoint);
  if (!result.has_value()) {
    return base::unexpected(OrchardShardTreeDelegate::Error::kStorageError);
  }
  return std::move(result.value());
}

base::expected<bool, OrchardShardTreeDelegate::Error>
OrchardShardTreeDelegate::TruncateCheckpoints(uint32_t checkpoint_id) {
  auto result = storage_->TruncateCheckpoints(account_id_, checkpoint_id);
  if (!result.has_value()) {
    return base::unexpected(OrchardShardTreeDelegate::Error::kStorageError);
  }
  return std::move(result.value());
}

base::expected<bool, OrchardShardTreeDelegate::Error>
OrchardShardTreeDelegate::RemoveCheckpoint(uint32_t checkpoint_id) {
  auto result = storage_->RemoveCheckpoint(account_id_, checkpoint_id);
  if (!result.has_value()) {
    return base::unexpected(OrchardShardTreeDelegate::Error::kStorageError);
  }
  return std::move(result.value());
}

base::expected<std::vector<OrchardShardAddress>,
               OrchardShardTreeDelegate::Error>
OrchardShardTreeDelegate::GetShardRoots(uint8_t shard_level) const {
  auto result = storage_->GetShardRoots(account_id_, shard_level);
  if (!result.has_value()) {
    return base::unexpected(OrchardShardTreeDelegate::Error::kStorageError);
  }
  return std::move(result.value());
}

base::expected<bool, OrchardShardTreeDelegate::Error>
OrchardShardTreeDelegate::UpdateCheckpoint(
    uint32_t id,
    const OrchardCheckpoint& checkpoint) {
  auto get_checkpoint_result = GetCheckpoint(id);
  if (!get_checkpoint_result.has_value()) {
    return base::unexpected(get_checkpoint_result.error());
  }
  if (!get_checkpoint_result.value()) {
    return false;
  }

  auto remove_result = RemoveCheckpoint(id);
  if (!remove_result.has_value()) {
    return base::unexpected(remove_result.error());
  }
  if (!remove_result.value()) {
    return false;
  }

  auto add_result = AddCheckpoint(id, checkpoint);
  if (!add_result.has_value()) {
    return base::unexpected(add_result.error());
  }
  if (!add_result.value()) {
    return false;
  }
  return true;
}

}  // namespace brave_wallet
