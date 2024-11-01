/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/zcash/orchard_shard_tree_delegate_impl.h"

namespace brave_wallet {

namespace {

OrchardShardTreeDelegate::Error From(ZCashOrchardStorage::Error) {
  return OrchardShardTreeDelegate::Error::kStorageError;
}

}  // namespace

OrchardShardTreeDelegateImpl::OrchardShardTreeDelegateImpl(
    mojom::AccountIdPtr account_id,
    scoped_refptr<ZCashOrchardStorage> storage)
    : account_id_(std::move(account_id)), storage_(storage) {}

OrchardShardTreeDelegateImpl::~OrchardShardTreeDelegateImpl() {}

base::expected<std::optional<OrchardCap>, OrchardShardTreeDelegate::Error>
OrchardShardTreeDelegateImpl::GetCap() const {
  auto result = storage_->GetCap(account_id_.Clone());
  if (!result.has_value()) {
    return base::unexpected(From(result.error()));
  }
  return std::move(*result);
}

base::expected<bool, OrchardShardTreeDelegate::Error>
OrchardShardTreeDelegateImpl::PutCap(OrchardCap cap) {
  auto result = storage_->PutCap(account_id_.Clone(), cap);
  if (!result.has_value()) {
    return base::unexpected(From(result.error()));
  }
  return *result;
}

base::expected<bool, OrchardShardTreeDelegate::Error>
OrchardShardTreeDelegateImpl::Truncate(uint32_t block_height) {
  auto result = storage_->TruncateShards(account_id_.Clone(), block_height);
  if (!result.has_value()) {
    return base::unexpected(From(result.error()));
  }
  return *result;
}

base::expected<std::optional<uint32_t>, OrchardShardTreeDelegate::Error>
OrchardShardTreeDelegateImpl::GetLatestShardIndex() const {
  auto result = storage_->GetLatestShardIndex(account_id_.Clone());
  if (!result.has_value()) {
    return base::unexpected(From(result.error()));
  }
  return *result;
}

base::expected<bool, OrchardShardTreeDelegate::Error>
OrchardShardTreeDelegateImpl::PutShard(OrchardShard shard) {
  auto result = storage_->PutShard(account_id_.Clone(), shard);
  if (!result.has_value()) {
    return base::unexpected(From(result.error()));
  }
  return *result;
}

base::expected<std::optional<OrchardShard>, OrchardShardTreeDelegate::Error>
OrchardShardTreeDelegateImpl::GetShard(OrchardShardAddress address) const {
  auto result = storage_->GetShard(account_id_.Clone(), address);
  if (!result.has_value()) {
    return base::unexpected(From(result.error()));
  }
  return std::move(*result);
}

base::expected<std::optional<OrchardShard>, OrchardShardTreeDelegate::Error>
OrchardShardTreeDelegateImpl::LastShard(uint8_t shard_height) const {
  auto result = storage_->LastShard(account_id_.Clone(), shard_height);
  if (!result.has_value()) {
    return base::unexpected(From(result.error()));
  }
  return std::move(*result);
}

base::expected<size_t, OrchardShardTreeDelegate::Error>
OrchardShardTreeDelegateImpl::CheckpointCount() const {
  auto result = storage_->CheckpointCount(account_id_.Clone());
  if (!result.has_value()) {
    return base::unexpected(From(result.error()));
  }
  return *result;
}

base::expected<std::optional<uint32_t>, OrchardShardTreeDelegate::Error>
OrchardShardTreeDelegateImpl::MinCheckpointId() const {
  auto result = storage_->MinCheckpointId(account_id_.Clone());
  if (!result.has_value()) {
    return base::unexpected(From(result.error()));
  }
  return std::move(result.value());
}

base::expected<std::optional<uint32_t>, OrchardShardTreeDelegate::Error>
OrchardShardTreeDelegateImpl::MaxCheckpointId() const {
  auto result = storage_->MaxCheckpointId(account_id_.Clone());
  if (!result.has_value()) {
    return base::unexpected(From(result.error()));
  }
  return std::move(result.value());
}

base::expected<std::optional<uint32_t>, OrchardShardTreeDelegate::Error>
OrchardShardTreeDelegateImpl::GetCheckpointAtDepth(uint32_t depth) const {
  auto result = storage_->GetCheckpointAtDepth(account_id_.Clone(), depth);
  if (!result.has_value()) {
    return base::unexpected(From(result.error()));
  }
  return std::move(result.value());
}

base::expected<std::optional<OrchardCheckpointBundle>,
               OrchardShardTreeDelegate::Error>
OrchardShardTreeDelegateImpl::GetCheckpoint(uint32_t checkpoint_id) const {
  auto result = storage_->GetCheckpoint(account_id_.Clone(), checkpoint_id);
  if (!result.has_value()) {
    return base::unexpected(From(result.error()));
  }
  return std::move(result.value());
}

base::expected<std::vector<OrchardCheckpointBundle>,
               OrchardShardTreeDelegate::Error>
OrchardShardTreeDelegateImpl::GetCheckpoints(size_t limit) const {
  auto result = storage_->GetCheckpoints(account_id_.Clone(), limit);
  if (!result.has_value()) {
    return base::unexpected(From(result.error()));
  }
  return std::move(result.value());
}

base::expected<bool, OrchardShardTreeDelegate::Error>
OrchardShardTreeDelegateImpl::AddCheckpoint(uint32_t id,
                                            OrchardCheckpoint checkpoint) {
  auto result = storage_->AddCheckpoint(account_id_.Clone(), id, checkpoint);
  if (!result.has_value()) {
    return base::unexpected(From(result.error()));
  }
  return std::move(result.value());
}

base::expected<bool, OrchardShardTreeDelegate::Error>
OrchardShardTreeDelegateImpl::TruncateCheckpoints(uint32_t checkpoint_id) {
  auto result =
      storage_->TruncateCheckpoints(account_id_.Clone(), checkpoint_id);
  if (!result.has_value()) {
    return base::unexpected(From(result.error()));
  }
  return std::move(result.value());
}

base::expected<bool, OrchardShardTreeDelegate::Error>
OrchardShardTreeDelegateImpl::RemoveCheckpoint(uint32_t checkpoint_id) {
  auto result = storage_->RemoveCheckpoint(account_id_.Clone(), checkpoint_id);
  if (!result.has_value()) {
    return base::unexpected(From(result.error()));
  }
  return std::move(result.value());
}

base::expected<bool, OrchardShardTreeDelegateImpl::Error>
OrchardShardTreeDelegateImpl::RemoveCheckpointAt(uint32_t depth) {
  return false;
}

base::expected<std::vector<OrchardShardAddress>,
               OrchardShardTreeDelegate::Error>
OrchardShardTreeDelegateImpl::GetShardRoots(uint8_t shard_level) const {
  auto result = storage_->GetShardRoots(account_id_.Clone(), shard_level);
  if (!result.has_value()) {
    return base::unexpected(From(result.error()));
  }
  return std::move(result.value());
}

base::expected<bool, OrchardShardTreeDelegate::Error>
OrchardShardTreeDelegateImpl::UpdateCheckpoint(uint32_t id,
                                               OrchardCheckpoint checkpoint) {
  // RemoveCheckpoint(id);
  // AddCheckpoint(id, checkpoint);
  return false;
}

}  // namespace brave_wallet
