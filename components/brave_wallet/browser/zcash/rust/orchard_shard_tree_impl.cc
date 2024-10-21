// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/components/brave_wallet/browser/zcash/rust/orchard_shard_tree_impl.h"

#include "base/memory/ptr_util.h"
#include "base/ranges/algorithm.h"
#include "brave/components/brave_wallet/browser/zcash/rust/cxx/src/shard_store.h"
#include "brave/components/brave_wallet/browser/zcash/rust/orchard_decoded_blocks_bunde_impl.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "brave/components/brave_wallet/common/zcash_utils.h"

namespace brave_wallet::orchard {

::brave_wallet::OrchardShardAddress From(const FfiShardAddress& addr) {
  return ::brave_wallet::OrchardShardAddress{addr.level, addr.index};
}

FfiShardAddress From(const ::brave_wallet::OrchardShardAddress& addr) {
  return FfiShardAddress{addr.level, addr.index};
}

FfiCap From(::brave_wallet::OrchardCap& orchard_cap) {
  ::rust::Vec<uint8_t> data;
  data.reserve(orchard_cap.data.size());
  base::ranges::copy(orchard_cap.data, std::back_inserter(data));
  return FfiCap{std::move(data)};
}

::brave_wallet::OrchardCap From(const FfiCap& cap) {
  ::brave_wallet::OrchardCap orchard_cap;
  orchard_cap.data.reserve(cap.data.size());
  base::ranges::copy(cap.data, std::back_inserter(orchard_cap.data));
  return orchard_cap;
}

::brave_wallet::OrchardShard From(const FfiShardTree& tree) {
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

  LOG(ERROR) << "XXXZZZ Put: Shard data " << ToHex(data);
  LOG(ERROR) << "XXXZZZ Put: Shard end height ";
  // LOG(ERROR) << "XXXZZZ Put: Contains marked " << tree.contains_marked;
  LOG(ERROR) << "XXXZZZ Put: Root " << ToHex(tree.hash);

  return ::brave_wallet::OrchardShard(From(tree.address), shard_root_hash,
                                      std::move(data));
}

FfiShardTree From(const ::brave_wallet::OrchardShard& tree) {
  ::rust::Vec<uint8_t> data;
  data.reserve(tree.shard_data.size());
  base::ranges::copy(tree.shard_data, std::back_inserter(data));

  ::rust::Vec<uint8_t> hash;
  if (tree.root_hash) {
    base::ranges::copy(tree.root_hash.value(), std::back_inserter(hash));
  }
  LOG(ERROR) << "XXXZZZ Get: Shard data " << ToHex(tree.shard_data);
  LOG(ERROR) << "XXXZZZ Get: Shard end height " << tree.subtree_end_height;
  // LOG(ERROR) << "XXXZZZ Get: Contains marked " << tree.contains_marked;
  // LOG(ERROR) << "XXXZZZ Get: Root " << ToHex(tree.root_hash);

  return FfiShardTree{From(tree.address), std::move(hash), std::move(data)};
}

FfiCheckpoint From(const ::brave_wallet::OrchardCheckpoint& checkpoint) {
  ::rust::Vec<uint32_t> marks_removed;
  base::ranges::copy(checkpoint.marks_removed,
                     std::back_inserter(marks_removed));
  return FfiCheckpoint{!checkpoint.tree_state_position.has_value(),
                       checkpoint.tree_state_position.value_or(0),
                       marks_removed};
}

FfiCheckpointBundle From(
    const ::brave_wallet::OrchardCheckpointBundle& checkpoint_bundle) {
  return FfiCheckpointBundle(checkpoint_bundle.checkpoint_id,
                             From(checkpoint_bundle.checkpoint));
}

::brave_wallet::OrchardCheckpoint From(const FfiCheckpoint& checkpoint) {
  CheckpointTreeState checkpoint_tree_state = std::nullopt;
  if (!checkpoint.empty) {
    checkpoint_tree_state = checkpoint.position;
  }
  return ::brave_wallet::OrchardCheckpoint{
      checkpoint_tree_state,
      std::vector<uint32_t>(checkpoint.mark_removed.begin(),
                            checkpoint.mark_removed.end())};
}

ShardStoreStatusCode orchard_get_shard(const ShardStoreContext& ctx,
                                       const FfiShardAddress& addr,
                                       FfiShardTree& input) {
  LOG(ERROR) << "XXXZZZ orchard_get_shard " << addr.level << " " << addr.index;

  auto shard = ctx.GetShard(From(addr));
  if (!shard.has_value()) {
    LOG(ERROR) << "XXXZZZ error";
    return ShardStoreStatusCode::Error;
  } else if (!shard.value()) {
    return ShardStoreStatusCode::None;
  }
  input = From(**shard);
  return ShardStoreStatusCode::Ok;
}

ShardStoreStatusCode orchard_last_shard(const ShardStoreContext& ctx,
                                        FfiShardTree& input) {
  LOG(ERROR) << "XXXZZZ orchard_last_shard";

  auto shard = ctx.LastShard(4);
  if (!shard.has_value()) {
    return ShardStoreStatusCode::Error;
  } else if (!shard.value()) {
    return ShardStoreStatusCode::None;
  }
  input = From(**shard);
  return ShardStoreStatusCode::Ok;
}

ShardStoreStatusCode orchard_put_shard(ShardStoreContext& ctx,
                                       const FfiShardTree& tree) {
  LOG(ERROR) << "XXXZZZ orchard_put_shard";

  auto result = ctx.PutShard(From(tree));
  if (!result.has_value()) {
    return ShardStoreStatusCode::Error;
  } else if (!result.value()) {
    return ShardStoreStatusCode::None;
  }
  return ShardStoreStatusCode::Ok;
}

ShardStoreStatusCode orchard_get_shard_roots(
    const ShardStoreContext& ctx,
    ::rust::Vec<FfiShardAddress>& input) {
  LOG(ERROR) << "XXXZZZ orchard_get_shard_roots";

  auto shard = ctx.GetShardRoots(4);
  if (!shard.has_value()) {
    return ShardStoreStatusCode::Error;
  }
  for (const auto& root : *shard) {
    input.push_back(From(root));
  }
  return ShardStoreStatusCode::Ok;
}

ShardStoreStatusCode orchard_truncate(ShardStoreContext& ctx,
                                      const FfiShardAddress& address) {
  LOG(ERROR) << "XXXZZZ orchard_truncate";

  return ShardStoreStatusCode::Ok;
}

ShardStoreStatusCode orchard_get_cap(const ShardStoreContext& ctx,
                                     FfiCap& input) {
  LOG(ERROR) << "XXXZZZ orchard_get_cap";
  auto result = ctx.GetCap();
  if (!result.has_value()) {
    return ShardStoreStatusCode::Error;
  } else if (!result.value()) {
    return ShardStoreStatusCode::None;
  }
  input = From(**result);
  return ShardStoreStatusCode::Ok;
}

ShardStoreStatusCode orchard_put_cap(ShardStoreContext& ctx,
                                     const FfiCap& tree) {
  LOG(ERROR) << "XXXZZZ orchard_put_cap";

  auto result = ctx.PutCap(From(tree));
  if (!result.has_value()) {
    return ShardStoreStatusCode::Error;
  }
  return ShardStoreStatusCode::Ok;
}

ShardStoreStatusCode orchard_min_checkpoint_id(const ShardStoreContext& ctx,
                                               uint32_t& input) {
  LOG(ERROR) << "XXXZZZ orchard_min_checkpoint_id";

  auto result = ctx.MinCheckpointId();
  if (!result.has_value()) {
    return ShardStoreStatusCode::Error;
  } else if (!result.value()) {
    return ShardStoreStatusCode::None;
  }
  input = **result;
  return ShardStoreStatusCode::Ok;
}

ShardStoreStatusCode orchard_max_checkpoint_id(const ShardStoreContext& ctx,
                                               uint32_t& input) {
  LOG(ERROR) << "XXXZZZ orchard_max_checkpoint_id";

  auto result = ctx.MaxCheckpointId();
  if (!result.has_value()) {
    return ShardStoreStatusCode::Error;
  } else if (!result.value()) {
    return ShardStoreStatusCode::None;
  }
  input = **result;
  return ShardStoreStatusCode::Ok;
}

ShardStoreStatusCode orchard_add_checkpoint(ShardStoreContext& ctx,
                                            uint32_t checkpoint_id,
                                            const FfiCheckpoint& checkpoint) {
  LOG(ERROR) << "XXXZZZ orchard_add_checkpoint";

  auto result = ctx.AddCheckpoint(checkpoint_id, From(checkpoint));
  if (!result.has_value()) {
    return ShardStoreStatusCode::Error;
  }
  return ShardStoreStatusCode::Ok;
}

ShardStoreStatusCode orchard_checkpoint_count(const ShardStoreContext& ctx,
                                              size_t& into) {
  LOG(ERROR) << "XXXZZZ orchard_checkpoint_count";

  auto result = ctx.CheckpointCount();
  if (!result.has_value()) {
    return ShardStoreStatusCode::Error;
  }
  LOG(ERROR) << "XXXZZZ checkpoint count " << *result;
  into = *result;
  return ShardStoreStatusCode::Ok;
}

ShardStoreStatusCode orchard_get_checkpoint_at_depth(
    const ShardStoreContext& ctx,
    size_t depth,
    uint32_t& into_checkpoint_id,
    FfiCheckpoint& into_checkpoint) {
  LOG(ERROR) << "XXXZZZ orchard_get_checkpoint_at_depth";

  auto checkpoint_id = ctx.GetCheckpointAtDepth(depth);
  if (!checkpoint_id.has_value()) {
    return ShardStoreStatusCode::Error;
  } else if (!checkpoint_id.value()) {
    return ShardStoreStatusCode::None;
  }
  into_checkpoint_id = **checkpoint_id;

  auto checkpoint = ctx.GetCheckpoint(into_checkpoint_id);
  if (!checkpoint.has_value()) {
    return ShardStoreStatusCode::Error;
  } else if (!checkpoint.value()) {
    return ShardStoreStatusCode::None;
  }
  into_checkpoint = From((**checkpoint).checkpoint);
  return ShardStoreStatusCode::Ok;
}

ShardStoreStatusCode orchard_get_checkpoint(const ShardStoreContext& ctx,
                                            uint32_t checkpoint_id,
                                            FfiCheckpoint& input) {
  LOG(ERROR) << "XXXZZZ get checkpoint";

  auto checkpoint = ctx.GetCheckpoint(checkpoint_id);
  if (!checkpoint.has_value()) {
    return ShardStoreStatusCode::Error;
  } else if (!checkpoint.value()) {
    return ShardStoreStatusCode::None;
  }
  input = From((**checkpoint).checkpoint);
  return ShardStoreStatusCode::Ok;
}

ShardStoreStatusCode orchard_update_checkpoint(
    ShardStoreContext& ctx,
    uint32_t checkpoint_id,
    const FfiCheckpoint& checkpoint) {
  LOG(ERROR) << "XXXZZZ update checkpoint";

  auto result = ctx.UpdateCheckpoint(checkpoint_id, From(checkpoint));
  if (!result.has_value()) {
    return ShardStoreStatusCode::Error;
  } else if (!result.value()) {
    return ShardStoreStatusCode::None;
  }
  return ShardStoreStatusCode::Ok;
}

ShardStoreStatusCode orchard_remove_checkpoint(ShardStoreContext& ctx,
                                               uint32_t checkpoint_id) {
  LOG(ERROR) << "XXXZZZ remove checkpoint";

  auto result = ctx.RemoveCheckpoint(checkpoint_id);
  if (!result.has_value()) {
    return ShardStoreStatusCode::Error;
  } else if (!result.value()) {
    return ShardStoreStatusCode::None;
  }
  return ShardStoreStatusCode::Ok;
}

ShardStoreStatusCode orchard_truncate_checkpoint(ShardStoreContext& ctx,
                                                 uint32_t checkpoint_id) {
  LOG(ERROR) << "XXXZZZ truncate checkpoints";

  auto result = ctx.TruncateCheckpoints(checkpoint_id);
  if (!result.has_value()) {
    return ShardStoreStatusCode::Error;
  } else if (!result.value()) {
    return ShardStoreStatusCode::None;
  }
  return ShardStoreStatusCode::Ok;
}

ShardStoreStatusCode orchard_with_checkpoints(
    const ShardStoreContext& ctx,
    size_t limit,
    rust::cxxbridge1::Fn<
        ShardStoreStatusCode(uint32_t, const FfiCheckpoint& checkpoint)> fn) {
  LOG(ERROR) << "XXXZZZ with checkpoints";
  auto checkpoints = ctx.GetCheckpoints(limit);
  if (!checkpoints.has_value()) {
    return ShardStoreStatusCode::Error;
  } else if (checkpoints->empty()) {
    return ShardStoreStatusCode::None;
  }

  // TODO(cypt4): Make via a call to the DB
  for (const auto& checkpoint : checkpoints.value()) {
    LOG(ERROR) << "XXXZZZ " << checkpoint.checkpoint_id;
    auto r = fn(checkpoint.checkpoint_id, From(checkpoint.checkpoint));
    if (r != ShardStoreStatusCode::Ok) {
      return r;
    }
  }
  return ShardStoreStatusCode::Ok;
}

ShardStoreStatusCode orchard_get_checkpoints(
    const ShardStoreContext& ctx,
    size_t limit,
    ::rust::Vec<FfiCheckpointBundle>& into) {
  LOG(ERROR) << "XXXZZZ get checkpoints " << limit;

  auto checkpoints = ctx.GetCheckpoints(limit);
  if (!checkpoints.has_value()) {
    return ShardStoreStatusCode::Error;
  }
  if (checkpoints->empty()) {
    return ShardStoreStatusCode::None;
  }
  LOG(ERROR) << "XXXZZZ get checkpoints  size" << checkpoints->size();
  for (const auto& checkpoint : checkpoints.value()) {
    into.push_back(From(checkpoint));
  }
  return ShardStoreStatusCode::Ok;
}

bool OrchardShardTreeImpl::ApplyScanResults(
    std::unique_ptr<OrchardDecodedBlocksBundle> commitments) {
  auto* bundle_impl =
      static_cast<OrchardDecodedBlocksBundleImpl*>(commitments.get());
  LOG(ERROR) << "XXXZZZ apply scan result " << bundle_impl;

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
