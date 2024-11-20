/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/zcash/orchard_shard_tree_delegate_impl.h"

namespace brave_wallet {

OrchardShardTreeDelegateImpl::OrchardShardTreeDelegateImpl(
    const mojom::AccountIdPtr& account_id,
    scoped_refptr<ZCashOrchardStorage> storage)
    : account_id_(account_id.Clone()), storage_(storage) {}

OrchardShardTreeDelegateImpl::~OrchardShardTreeDelegateImpl() {}

base::expected<std::optional<OrchardShardTreeCap>,
               OrchardShardTreeDelegate::Error>
OrchardShardTreeDelegateImpl::GetCap() const {
  auto result = storage_->GetCap(account_id_);
  if (!result.has_value()) {
    return base::unexpected(OrchardShardTreeDelegate::Error::kStorageError);
  }
  return std::move(*result);
}

base::expected<bool, OrchardShardTreeDelegate::Error>
OrchardShardTreeDelegateImpl::PutCap(const OrchardShardTreeCap& cap) {
  auto result = storage_->PutCap(account_id_, cap);
  if (!result.has_value()) {
    return base::unexpected(OrchardShardTreeDelegate::Error::kStorageError);
  }
  return *result;
}

base::expected<bool, OrchardShardTreeDelegate::Error>
OrchardShardTreeDelegateImpl::Truncate(uint32_t block_height) {
  auto result = storage_->TruncateShards(account_id_, block_height);
  if (!result.has_value()) {
    return base::unexpected(OrchardShardTreeDelegate::Error::kStorageError);
  }
  return *result;
}

base::expected<std::optional<uint32_t>, OrchardShardTreeDelegate::Error>
OrchardShardTreeDelegateImpl::GetLatestShardIndex() const {
  auto result = storage_->GetLatestShardIndex(account_id_);
  if (!result.has_value()) {
    return base::unexpected(OrchardShardTreeDelegate::Error::kStorageError);
  }
  return *result;
}

base::expected<bool, OrchardShardTreeDelegate::Error>
OrchardShardTreeDelegateImpl::PutShard(const OrchardShard& shard) {
  auto result = storage_->PutShard(account_id_, shard);
  if (!result.has_value()) {
    return base::unexpected(OrchardShardTreeDelegate::Error::kStorageError);
  }
  return *result;
}

base::expected<std::optional<OrchardShard>, OrchardShardTreeDelegate::Error>
OrchardShardTreeDelegateImpl::GetShard(
    const OrchardShardAddress& address) const {
  auto result = storage_->GetShard(account_id_, address);
  if (!result.has_value()) {
    return base::unexpected(OrchardShardTreeDelegate::Error::kStorageError);
  }
  return std::move(*result);
}

base::expected<std::optional<OrchardShard>, OrchardShardTreeDelegate::Error>
OrchardShardTreeDelegateImpl::LastShard(uint8_t shard_height) const {
  auto result = storage_->LastShard(account_id_, shard_height);
  if (!result.has_value()) {
    return base::unexpected(OrchardShardTreeDelegate::Error::kStorageError);
  }
  return std::move(*result);
}

base::expected<size_t, OrchardShardTreeDelegate::Error>
OrchardShardTreeDelegateImpl::CheckpointCount() const {
  auto result = storage_->CheckpointCount(account_id_);
  if (!result.has_value()) {
    return base::unexpected(OrchardShardTreeDelegate::Error::kStorageError);
  }
  return *result;
}

base::expected<std::optional<uint32_t>, OrchardShardTreeDelegate::Error>
OrchardShardTreeDelegateImpl::MinCheckpointId() const {
  auto result = storage_->MinCheckpointId(account_id_);
  if (!result.has_value()) {
    return base::unexpected(OrchardShardTreeDelegate::Error::kStorageError);
  }
  return std::move(result.value());
}

base::expected<std::optional<uint32_t>, OrchardShardTreeDelegate::Error>
OrchardShardTreeDelegateImpl::MaxCheckpointId() const {
  auto result = storage_->MaxCheckpointId(account_id_);
  if (!result.has_value()) {
    return base::unexpected(OrchardShardTreeDelegate::Error::kStorageError);
  }
  return std::move(result.value());
}

base::expected<std::optional<uint32_t>, OrchardShardTreeDelegate::Error>
OrchardShardTreeDelegateImpl::GetCheckpointAtDepth(uint32_t depth) const {
  auto result = storage_->GetCheckpointAtDepth(account_id_, depth);
  if (!result.has_value()) {
    return base::unexpected(OrchardShardTreeDelegate::Error::kStorageError);
  }
  return std::move(result.value());
}

base::expected<std::optional<OrchardCheckpointBundle>,
               OrchardShardTreeDelegate::Error>
OrchardShardTreeDelegateImpl::GetCheckpoint(uint32_t checkpoint_id) const {
  auto result = storage_->GetCheckpoint(account_id_, checkpoint_id);
  if (!result.has_value()) {
    return base::unexpected(OrchardShardTreeDelegate::Error::kStorageError);
  }
  return std::move(result.value());
}

base::expected<std::vector<OrchardCheckpointBundle>,
               OrchardShardTreeDelegate::Error>
OrchardShardTreeDelegateImpl::GetCheckpoints(size_t limit) const {
  auto result = storage_->GetCheckpoints(account_id_, limit);
  if (!result.has_value()) {
    return base::unexpected(OrchardShardTreeDelegate::Error::kStorageError);
  }
  return std::move(result.value());
}

base::expected<bool, OrchardShardTreeDelegate::Error>
OrchardShardTreeDelegateImpl::AddCheckpoint(
    uint32_t id,
    const OrchardCheckpoint& checkpoint) {
  auto result = storage_->AddCheckpoint(account_id_, id, checkpoint);
  if (!result.has_value()) {
    return base::unexpected(OrchardShardTreeDelegate::Error::kStorageError);
  }
  return std::move(result.value());
}

base::expected<bool, OrchardShardTreeDelegate::Error>
OrchardShardTreeDelegateImpl::TruncateCheckpoints(uint32_t checkpoint_id) {
  auto result = storage_->TruncateCheckpoints(account_id_, checkpoint_id);
  if (!result.has_value()) {
    return base::unexpected(OrchardShardTreeDelegate::Error::kStorageError);
  }
  return std::move(result.value());
}

base::expected<bool, OrchardShardTreeDelegate::Error>
OrchardShardTreeDelegateImpl::RemoveCheckpoint(uint32_t checkpoint_id) {
  auto result = storage_->RemoveCheckpoint(account_id_, checkpoint_id);
  if (!result.has_value()) {
    return base::unexpected(OrchardShardTreeDelegate::Error::kStorageError);
  }
  return std::move(result.value());
}

base::expected<std::vector<OrchardShardAddress>,
               OrchardShardTreeDelegate::Error>
OrchardShardTreeDelegateImpl::GetShardRoots(uint8_t shard_level) const {
  auto result = storage_->GetShardRoots(account_id_, shard_level);
  if (!result.has_value()) {
    return base::unexpected(OrchardShardTreeDelegate::Error::kStorageError);
  }
  return std::move(result.value());
}

base::expected<bool, OrchardShardTreeDelegate::Error>
OrchardShardTreeDelegateImpl::UpdateCheckpoint(
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
