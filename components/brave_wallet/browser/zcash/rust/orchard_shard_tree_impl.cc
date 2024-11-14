/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/zcash/rust/orchard_shard_tree_impl.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/memory/ptr_util.h"
#include "base/ranges/algorithm.h"
#include "brave/components/brave_wallet/browser/zcash/rust/cxx/src/shard_store.h"
#include "brave/components/brave_wallet/browser/zcash/rust/orchard_decoded_blocks_bunde_impl.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "brave/components/brave_wallet/common/zcash_utils.h"

namespace brave_wallet::orchard {

::brave_wallet::OrchardShardAddress From(const ShardTreeAddress& addr) {
  return ::brave_wallet::OrchardShardAddress{addr.level, addr.index};
}

ShardTreeAddress From(const ::brave_wallet::OrchardShardAddress& addr) {
  return ShardTreeAddress{addr.level, addr.index};
}

ShardTreeCap From(::brave_wallet::OrchardShardTreeCap& shard_store_cap) {
  ::rust::Vec<uint8_t> data;
  data.reserve(shard_store_cap.size());
  base::ranges::copy(shard_store_cap, std::back_inserter(data));
  return ShardTreeCap{std::move(data)};
}

::brave_wallet::OrchardShardTreeCap From(const ShardTreeCap& cap) {
  ::brave_wallet::OrchardShardTreeCap shard_store_cap;
  shard_store_cap.reserve(cap.data.size());
  base::ranges::copy(cap.data, std::back_inserter(shard_store_cap));
  return shard_store_cap;
}

::brave_wallet::OrchardShard From(const ShardTreeShard& tree) {
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

  return ::brave_wallet::OrchardShard(From(tree.address), shard_root_hash,
                                      std::move(data));
}

ShardTreeShard From(const ::brave_wallet::OrchardShard& tree) {
  ::rust::Vec<uint8_t> data;
  data.reserve(tree.shard_data.size());
  base::ranges::copy(tree.shard_data, std::back_inserter(data));

  ::rust::Vec<uint8_t> hash;
  if (tree.root_hash) {
    base::ranges::copy(tree.root_hash.value(), std::back_inserter(hash));
  }
  return ShardTreeShard{From(tree.address), std::move(hash), std::move(data)};
}

ShardTreeCheckpoint From(const ::brave_wallet::OrchardCheckpoint& checkpoint) {
  ::rust::Vec<uint32_t> marks_removed;
  base::ranges::copy(checkpoint.marks_removed,
                     std::back_inserter(marks_removed));
  return ShardTreeCheckpoint{!checkpoint.tree_state_position.has_value(),
                             checkpoint.tree_state_position.value_or(0),
                             marks_removed};
}

ShardTreeCheckpointBundle From(
    const ::brave_wallet::OrchardCheckpointBundle& checkpoint_bundle) {
  return ShardTreeCheckpointBundle(checkpoint_bundle.checkpoint_id,
                                   From(checkpoint_bundle.checkpoint));
}

::brave_wallet::OrchardCheckpoint From(const ShardTreeCheckpoint& checkpoint) {
  CheckpointTreeState checkpoint_tree_state = std::nullopt;
  if (!checkpoint.empty) {
    checkpoint_tree_state = checkpoint.position;
  }
  return ::brave_wallet::OrchardCheckpoint{
      checkpoint_tree_state,
      std::vector<uint32_t>(checkpoint.mark_removed.begin(),
                            checkpoint.mark_removed.end())};
}

ShardStoreStatusCode shard_store_get_shard(const ShardTreeDelegate& delegate,
                                           const ShardTreeAddress& addr,
                                           ShardTreeShard& input) {
  auto shard = delegate.GetShard(From(addr));
  if (!shard.has_value()) {
    return ShardStoreStatusCode::Error;
  } else if (!shard.value()) {
    return ShardStoreStatusCode::None;
  }
  input = From(**shard);
  return ShardStoreStatusCode::Ok;
}

ShardStoreStatusCode shard_store_last_shard(const ShardTreeDelegate& delegate,
                                            ShardTreeShard& input,
                                            uint8_t shard_level) {
  auto shard = delegate.LastShard(shard_level);
  if (!shard.has_value()) {
    return ShardStoreStatusCode::Error;
  } else if (!shard.value()) {
    return ShardStoreStatusCode::None;
  }
  input = From(**shard);
  return ShardStoreStatusCode::Ok;
}

ShardStoreStatusCode shard_store_put_shard(ShardTreeDelegate& delegate,
                                           const ShardTreeShard& tree) {
  auto result = delegate.PutShard(From(tree));
  if (!result.has_value()) {
    return ShardStoreStatusCode::Error;
  } else if (!result.value()) {
    return ShardStoreStatusCode::None;
  }
  return ShardStoreStatusCode::Ok;
}

ShardStoreStatusCode shard_store_get_shard_roots(
    const ShardTreeDelegate& delegate,
    ::rust::Vec<ShardTreeAddress>& input,
    uint8_t shard_level) {
  auto shard = delegate.GetShardRoots(shard_level);
  if (!shard.has_value()) {
    return ShardStoreStatusCode::Error;
  }
  for (const auto& root : *shard) {
    input.push_back(From(root));
  }
  return ShardStoreStatusCode::Ok;
}

ShardStoreStatusCode shard_store_truncate(ShardTreeDelegate& delegate,
                                          const ShardTreeAddress& address) {
  auto result = delegate.Truncate(address.index);
  if (!result.has_value()) {
    return ShardStoreStatusCode::Error;
  } else if (!result.value()) {
    return ShardStoreStatusCode::None;
  }
  return ShardStoreStatusCode::Ok;
}

ShardStoreStatusCode shard_store_get_cap(const ShardTreeDelegate& delegate,
                                         ShardTreeCap& input) {
  auto result = delegate.GetCap();
  if (!result.has_value()) {
    return ShardStoreStatusCode::Error;
  } else if (!result.value()) {
    return ShardStoreStatusCode::None;
  }
  input = From(**result);
  return ShardStoreStatusCode::Ok;
}

ShardStoreStatusCode shard_store_put_cap(ShardTreeDelegate& delegate,
                                         const ShardTreeCap& tree) {
  auto result = delegate.PutCap(From(tree));
  if (!result.has_value()) {
    return ShardStoreStatusCode::Error;
  }
  return ShardStoreStatusCode::Ok;
}

ShardStoreStatusCode shard_store_min_checkpoint_id(
    const ShardTreeDelegate& delegate,
    uint32_t& input) {
  auto result = delegate.MinCheckpointId();
  if (!result.has_value()) {
    return ShardStoreStatusCode::Error;
  } else if (!result.value()) {
    return ShardStoreStatusCode::None;
  }
  input = **result;
  return ShardStoreStatusCode::Ok;
}

ShardStoreStatusCode shard_store_max_checkpoint_id(
    const ShardTreeDelegate& delegate,
    uint32_t& input) {
  auto result = delegate.MaxCheckpointId();
  if (!result.has_value()) {
    return ShardStoreStatusCode::Error;
  } else if (!result.value()) {
    return ShardStoreStatusCode::None;
  }
  input = **result;
  return ShardStoreStatusCode::Ok;
}

ShardStoreStatusCode shard_store_add_checkpoint(
    ShardTreeDelegate& delegate,
    uint32_t checkpoint_id,
    const ShardTreeCheckpoint& checkpoint) {
  auto result = delegate.AddCheckpoint(checkpoint_id, From(checkpoint));
  if (!result.has_value()) {
    return ShardStoreStatusCode::Error;
  }
  return ShardStoreStatusCode::Ok;
}

ShardStoreStatusCode shard_store_checkpoint_count(
    const ShardTreeDelegate& delegate,
    size_t& into) {
  auto result = delegate.CheckpointCount();
  if (!result.has_value()) {
    return ShardStoreStatusCode::Error;
  }
  into = *result;
  return ShardStoreStatusCode::Ok;
}

ShardStoreStatusCode shard_store_get_checkpoint_at_depth(
    const ShardTreeDelegate& delegate,
    size_t depth,
    uint32_t& into_checkpoint_id,
    ShardTreeCheckpoint& into_checkpoint) {
  auto checkpoint_id = delegate.GetCheckpointAtDepth(depth);
  if (!checkpoint_id.has_value()) {
    return ShardStoreStatusCode::Error;
  } else if (!checkpoint_id.value()) {
    return ShardStoreStatusCode::None;
  }
  into_checkpoint_id = **checkpoint_id;

  auto checkpoint = delegate.GetCheckpoint(into_checkpoint_id);
  if (!checkpoint.has_value()) {
    return ShardStoreStatusCode::Error;
  } else if (!checkpoint.value()) {
    return ShardStoreStatusCode::None;
  }
  into_checkpoint = From((**checkpoint).checkpoint);
  return ShardStoreStatusCode::Ok;
}

ShardStoreStatusCode shard_store_get_checkpoint(
    const ShardTreeDelegate& delegate,
    uint32_t checkpoint_id,
    ShardTreeCheckpoint& input) {
  auto checkpoint = delegate.GetCheckpoint(checkpoint_id);
  if (!checkpoint.has_value()) {
    return ShardStoreStatusCode::Error;
  } else if (!checkpoint.value()) {
    return ShardStoreStatusCode::None;
  }
  input = From((**checkpoint).checkpoint);
  return ShardStoreStatusCode::Ok;
}

ShardStoreStatusCode shard_store_update_checkpoint(
    ShardTreeDelegate& delegate,
    uint32_t checkpoint_id,
    const ShardTreeCheckpoint& checkpoint) {
  auto result = delegate.UpdateCheckpoint(checkpoint_id, From(checkpoint));
  if (!result.has_value()) {
    return ShardStoreStatusCode::Error;
  } else if (!result.value()) {
    return ShardStoreStatusCode::None;
  }
  return ShardStoreStatusCode::Ok;
}

ShardStoreStatusCode shard_store_remove_checkpoint(ShardTreeDelegate& delegate,
                                                   uint32_t checkpoint_id) {
  auto result = delegate.RemoveCheckpoint(checkpoint_id);
  if (!result.has_value()) {
    return ShardStoreStatusCode::Error;
  } else if (!result.value()) {
    return ShardStoreStatusCode::None;
  }
  return ShardStoreStatusCode::Ok;
}

ShardStoreStatusCode shard_store_truncate_checkpoint(
    ShardTreeDelegate& delegate,
    uint32_t checkpoint_id) {
  auto result = delegate.TruncateCheckpoints(checkpoint_id);
  if (!result.has_value()) {
    return ShardStoreStatusCode::Error;
  } else if (!result.value()) {
    return ShardStoreStatusCode::None;
  }
  return ShardStoreStatusCode::Ok;
}

ShardStoreStatusCode shard_store_get_checkpoints(
    const ShardTreeDelegate& delegate,
    size_t limit,
    ::rust::Vec<ShardTreeCheckpointBundle>& into) {
  auto checkpoints = delegate.GetCheckpoints(limit);
  if (!checkpoints.has_value()) {
    return ShardStoreStatusCode::Error;
  }
  if (checkpoints->empty()) {
    return ShardStoreStatusCode::None;
  }
  for (const auto& checkpoint : checkpoints.value()) {
    into.push_back(From(checkpoint));
  }
  return ShardStoreStatusCode::Ok;
}

bool OrchardShardTreeImpl::ApplyScanResults(
    std::unique_ptr<OrchardDecodedBlocksBundle> commitments) {
  auto* bundle_impl =
      static_cast<OrchardDecodedBlocksBundleImpl*>(commitments.get());
  return orcard_shard_tree_->insert_commitments(bundle_impl->GetDecodeBundle());
}

base::expected<OrchardNoteWitness, std::string>
OrchardShardTreeImpl::CalculateWitness(uint32_t note_commitment_tree_position,
                                       uint32_t checkpoint) {
  auto result = orcard_shard_tree_->calculate_witness(
      note_commitment_tree_position, checkpoint);
  if (!result->is_ok()) {
    return base::unexpected(result->error_message().c_str());
  }

  auto value = result->unwrap();

  OrchardNoteWitness witness;
  witness.position = note_commitment_tree_position;
  for (size_t i = 0; i < value->size(); i++) {
    witness.merkle_path.push_back(value->item(i));
  }

  return witness;
}

bool OrchardShardTreeImpl::TruncateToCheckpoint(uint32_t checkpoint_id) {
  return orcard_shard_tree_->truncate(checkpoint_id);
}

OrchardShardTreeImpl::OrchardShardTreeImpl(
    rust::Box<OrchardShardTreeBundle> orcard_shard_tree)
    : orcard_shard_tree_(std::move(orcard_shard_tree)) {}

OrchardShardTreeImpl::~OrchardShardTreeImpl() {}

// static
std::unique_ptr<OrchardShardTree> OrchardShardTree::Create(
    std::unique_ptr<::brave_wallet::OrchardShardTreeDelegate> delegate) {
  auto shard_tree_result =
      ::brave_wallet::orchard::create_shard_tree(std::move(delegate));
  if (!shard_tree_result->is_ok()) {
    return nullptr;
  }
  return base::WrapUnique<OrchardShardTree>(
      new OrchardShardTreeImpl(shard_tree_result->unwrap()));
}

}  // namespace brave_wallet::orchard
