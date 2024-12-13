// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_wallet/browser/zcash/rust/cxx_orchard_shard_tree_delegate.h"

#include <utility>
#include <vector>

#include "brave/components/brave_wallet/browser/internal/orchard_storage/orchard_storage.h"
#include "brave/components/brave_wallet/browser/zcash/rust/lib.rs.h"
#include "brave/components/brave_wallet/common/zcash_utils.h"

namespace brave_wallet::orchard {

namespace {

::brave_wallet::OrchardShardAddress FromRust(
    const CxxOrchardShardAddress& addr) {
  return ::brave_wallet::OrchardShardAddress{addr.level, addr.index};
}

CxxOrchardShardAddress ToRust(const ::brave_wallet::OrchardShardAddress& addr) {
  return CxxOrchardShardAddress{addr.level, addr.index};
}

CxxOrchardShardTreeCap ToRust(
    ::brave_wallet::OrchardShardTreeCap& shard_store_cap) {
  ::rust::Vec<uint8_t> data;
  data.reserve(shard_store_cap.size());
  base::ranges::copy(shard_store_cap, std::back_inserter(data));
  return CxxOrchardShardTreeCap{std::move(data)};
}

::brave_wallet::OrchardShardTreeCap FromRust(
    const CxxOrchardShardTreeCap& cap) {
  ::brave_wallet::OrchardShardTreeCap shard_store_cap;
  shard_store_cap.reserve(cap.data.size());
  base::ranges::copy(cap.data, std::back_inserter(shard_store_cap));
  return shard_store_cap;
}

::brave_wallet::OrchardShard FromRust(const CxxOrchardShard& tree) {
  std::optional<OrchardShardRootHash> shard_root_hash;
  if (!tree.hash.empty()) {
    CHECK_EQ(kOrchardShardTreeHashSize, tree.hash.size());
    OrchardShardRootHash hash_value;
    base::ranges::copy(tree.hash, hash_value.begin());
    shard_root_hash = hash_value;
  }

  std::vector<uint8_t> data;
  data.reserve(tree.data.size());
  base::ranges::copy(tree.data, std::back_inserter(data));

  return ::brave_wallet::OrchardShard(FromRust(tree.address), shard_root_hash,
                                      std::move(data));
}

CxxOrchardShard ToRust(const ::brave_wallet::OrchardShard& tree) {
  ::rust::Vec<uint8_t> data;
  data.reserve(tree.shard_data.size());
  base::ranges::copy(tree.shard_data, std::back_inserter(data));

  ::rust::Vec<uint8_t> hash;
  if (tree.root_hash) {
    base::ranges::copy(tree.root_hash.value(), std::back_inserter(hash));
  }
  return CxxOrchardShard{ToRust(tree.address), std::move(hash),
                         std::move(data)};
}

CxxOrchardCheckpoint ToRust(
    const ::brave_wallet::OrchardCheckpoint& checkpoint) {
  ::rust::Vec<uint32_t> marks_removed;
  base::ranges::copy(checkpoint.marks_removed,
                     std::back_inserter(marks_removed));
  return CxxOrchardCheckpoint{!checkpoint.tree_state_position.has_value(),
                              checkpoint.tree_state_position.value_or(0),
                              marks_removed};
}

CxxOrchardCheckpointBundle ToRust(
    const ::brave_wallet::OrchardCheckpointBundle& checkpoint_bundle) {
  return CxxOrchardCheckpointBundle(checkpoint_bundle.checkpoint_id,
                                    ToRust(checkpoint_bundle.checkpoint));
}

::brave_wallet::OrchardCheckpoint FromRust(
    const CxxOrchardCheckpoint& checkpoint) {
  CheckpointTreeState checkpoint_tree_state = std::nullopt;
  if (!checkpoint.empty) {
    checkpoint_tree_state = checkpoint.position;
  }
  return ::brave_wallet::OrchardCheckpoint{
      checkpoint_tree_state,
      std::vector<uint32_t>(checkpoint.mark_removed.begin(),
                            checkpoint.mark_removed.end())};
}

}  // namespace

CxxOrchardShardTreeDelegate::CxxOrchardShardTreeDelegate(
    OrchardStorage& storage,
    const mojom::AccountIdPtr& account_id)
    : storage_(storage), account_id_(account_id.Clone()) {}

CxxOrchardShardTreeDelegate::~CxxOrchardShardTreeDelegate() = default;

::rust::Box<CxxOrchardShardResultWrapper> CxxOrchardShardTreeDelegate::GetShard(
    const CxxOrchardShardAddress& addr) const {
  auto shard = storage_->GetShard(account_id_, FromRust(addr));
  if (!shard.has_value()) {
    return wrap_shard_tree_shard_error();
  } else if (!shard.value()) {
    return wrap_shard_tree_shard_none();
  }
  return wrap_shard_tree_shard(ToRust(**shard));
}

::rust::Box<CxxOrchardShardResultWrapper>
CxxOrchardShardTreeDelegate::LastShard(uint8_t shard_level) const {
  auto shard = storage_->LastShard(account_id_, shard_level);
  if (!shard.has_value()) {
    return wrap_shard_tree_shard_error();
  } else if (!shard.value()) {
    return wrap_shard_tree_shard_none();
  }
  return wrap_shard_tree_shard(ToRust(**shard));
}

::rust::Box<CxxBoolResultWrapper> CxxOrchardShardTreeDelegate::PutShard(
    const CxxOrchardShard& tree) const {
  auto result = storage_->PutShard(account_id_, FromRust(tree));
  if (!result.has_value()) {
    return wrap_bool_error();
  }

  return wrap_bool(result.value() == OrchardStorage::Result::kSuccess);
}

::rust::Box<CxxShardRootsResultWrapper>
CxxOrchardShardTreeDelegate::GetShardRoots(uint8_t shard_level) const {
  auto shard = storage_->GetShardRoots(account_id_, shard_level);
  if (!shard.has_value()) {
    return wrap_shard_tree_roots_error();
  }
  ::rust::Vec<CxxOrchardShardAddress> roots;
  for (const auto& root : *shard) {
    roots.push_back(ToRust(root));
  }
  return wrap_shard_tree_roots(std::move(roots));
}

::rust::Box<CxxBoolResultWrapper> CxxOrchardShardTreeDelegate::Truncate(
    const CxxOrchardShardAddress& address) const {
  auto result = storage_->TruncateShards(account_id_, address.index);
  if (!result.has_value()) {
    return wrap_bool_error();
  }
  return wrap_bool(result.value() == OrchardStorage::Result::kSuccess);
}

::rust::Box<CxxOrchardShardTreeCapResultWrapper>
CxxOrchardShardTreeDelegate::GetCap() const {
  auto result = storage_->GetCap(account_id_);
  if (!result.has_value()) {
    return wrap_shard_tree_cap_error();
  } else if (!result.value()) {
    return wrap_shard_tree_cap_none();
  }
  return wrap_shard_tree_cap(ToRust(**result));
}

::rust::Box<CxxBoolResultWrapper> CxxOrchardShardTreeDelegate::PutCap(
    const CxxOrchardShardTreeCap& tree) const {
  auto result = storage_->PutCap(account_id_, FromRust(tree));
  if (!result.has_value()) {
    return wrap_bool_error();
  }
  return wrap_bool(result.value() == OrchardStorage::Result::kSuccess);
}

::rust::Box<CxxCheckpointIdResultWrapper>
CxxOrchardShardTreeDelegate::MinCheckpointId() const {
  auto result = storage_->MinCheckpointId(account_id_);
  if (!result.has_value()) {
    return wrap_checkpoint_id_error();
  } else if (!result.value()) {
    return wrap_checkpoint_id_none();
  }
  return wrap_checkpoint_id(**result);
}

::rust::Box<CxxCheckpointIdResultWrapper>
CxxOrchardShardTreeDelegate::MaxCheckpointId() const {
  auto result = storage_->MaxCheckpointId(account_id_);
  if (!result.has_value()) {
    return wrap_checkpoint_id_error();
  } else if (!result.value()) {
    return wrap_checkpoint_id_none();
  }
  return wrap_checkpoint_id(**result);
}

::rust::Box<CxxBoolResultWrapper> CxxOrchardShardTreeDelegate::AddCheckpoint(
    uint32_t checkpoint_id,
    const CxxOrchardCheckpoint& checkpoint) const {
  auto result =
      storage_->AddCheckpoint(account_id_, checkpoint_id, FromRust(checkpoint));
  if (!result.has_value()) {
    return wrap_bool_error();
  }
  return wrap_bool(result.value() == OrchardStorage::Result::kSuccess);
}

::rust::Box<CxxCheckpointCountResultWrapper>
CxxOrchardShardTreeDelegate::CheckpointCount() const {
  auto result = storage_->CheckpointCount(account_id_);
  if (!result.has_value()) {
    return wrap_checkpoint_count_error();
  }
  return wrap_checkpoint_count(result.value());
}

::rust::Box<CxxCheckpointBundleResultWrapper>
CxxOrchardShardTreeDelegate::CheckpointAtDepth(size_t depth) const {
  auto checkpoint_id = storage_->GetCheckpointAtDepth(account_id_, depth);
  if (!checkpoint_id.has_value()) {
    return wrap_checkpoint_bundle_error();
  } else if (!checkpoint_id.value()) {
    return wrap_checkpoint_bundle_none();
  }

  auto checkpoint = storage_->GetCheckpoint(account_id_, **checkpoint_id);
  if (!checkpoint.has_value()) {
    return wrap_checkpoint_bundle_error();
  } else if (!checkpoint.value()) {
    return wrap_checkpoint_bundle_none();
  }
  return wrap_checkpoint_bundle(ToRust(**checkpoint));
}

::rust::Box<CxxCheckpointBundleResultWrapper>
CxxOrchardShardTreeDelegate::GetCheckpoint(uint32_t checkpoint_id) const {
  auto checkpoint = storage_->GetCheckpoint(account_id_, checkpoint_id);
  if (!checkpoint.has_value()) {
    return wrap_checkpoint_bundle_error();
  } else if (!checkpoint.value()) {
    return wrap_checkpoint_bundle_none();
  }
  return wrap_checkpoint_bundle(ToRust(**checkpoint));
}

::rust::Box<CxxBoolResultWrapper> CxxOrchardShardTreeDelegate::UpdateCheckpoint(
    uint32_t checkpoint_id,
    const CxxOrchardCheckpoint& checkpoint) const {
  auto result = storage_->UpdateCheckpoint(account_id_, checkpoint_id,
                                           FromRust(checkpoint));
  if (!result.has_value()) {
    return wrap_bool_error();
  }
  return wrap_bool(result.value() == OrchardStorage::Result::kSuccess);
}

::rust::Box<CxxBoolResultWrapper> CxxOrchardShardTreeDelegate::RemoveCheckpoint(
    uint32_t checkpoint_id) const {
  auto result = storage_->RemoveCheckpoint(account_id_, checkpoint_id);
  if (!result.has_value()) {
    return wrap_bool_error();
  }
  return wrap_bool(result.value() == OrchardStorage::Result::kSuccess);
}

::rust::Box<CxxBoolResultWrapper>
CxxOrchardShardTreeDelegate::TruncateCheckpoint(uint32_t checkpoint_id) const {
  auto result = storage_->TruncateCheckpoints(account_id_, checkpoint_id);
  if (!result.has_value()) {
    return wrap_bool_error();
  }
  return wrap_bool(result.value() == OrchardStorage::Result::kSuccess);
}

::rust::Box<CxxCheckpointsResultWrapper>
CxxOrchardShardTreeDelegate::GetCheckpoints(size_t limit) const {
  auto checkpoints = storage_->GetCheckpoints(account_id_, limit);
  if (!checkpoints.has_value()) {
    return wrap_checkpoints_error();
  }
  ::rust::Vec<CxxOrchardCheckpointBundle> result;
  for (const auto& checkpoint : checkpoints.value()) {
    result.push_back(ToRust(checkpoint));
  }
  return wrap_checkpoints(std::move(result));
}

}  // namespace brave_wallet::orchard
