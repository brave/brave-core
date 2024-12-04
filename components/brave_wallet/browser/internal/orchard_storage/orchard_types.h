/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_INTERNAL_ORCHARD_STORAGE_ORCHARD_TYPES_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_INTERNAL_ORCHARD_STORAGE_ORCHARD_TYPES_H_

#include <utility>
#include <vector>

#include "base/memory/raw_ref.h"
#include "brave/components/brave_wallet/common/zcash_utils.h"

namespace brave_wallet {

// Leaf position of checkpoint.
using CheckpointTreeState = std::optional<uint32_t>;

// Checkpointed leafs are not pruned so they could be used
// as anchors for building shielded transactions.
// Last Orchard commitment in a block is used as a checkpoint.
struct OrchardCheckpoint {
  OrchardCheckpoint();
  OrchardCheckpoint(CheckpointTreeState, std::vector<uint32_t>);
  ~OrchardCheckpoint();
  OrchardCheckpoint(const OrchardCheckpoint& other) = delete;
  OrchardCheckpoint& operator=(const OrchardCheckpoint& other) = delete;
  OrchardCheckpoint(OrchardCheckpoint&& other);
  OrchardCheckpoint& operator=(OrchardCheckpoint&& other);

  OrchardCheckpoint Clone();
  bool operator==(const OrchardCheckpoint& other) const = default;

  // Leaf position of the checkpoint.
  CheckpointTreeState tree_state_position;
  // List of note positions that were spent at this checkpoint.
  std::vector<uint32_t> marks_removed;
};

struct OrchardCheckpointBundle {
  OrchardCheckpointBundle(uint32_t checkpoint_id, OrchardCheckpoint);
  ~OrchardCheckpointBundle();
  OrchardCheckpointBundle(const OrchardCheckpointBundle& other) = delete;
  OrchardCheckpointBundle& operator=(const OrchardCheckpointBundle& other) =
      delete;
  OrchardCheckpointBundle(OrchardCheckpointBundle&& other);
  OrchardCheckpointBundle& operator=(OrchardCheckpointBundle&& other);

  bool operator==(const OrchardCheckpointBundle& other) const = default;

  // The block height serves as the checkpoint identifier.
  uint32_t checkpoint_id = 0;
  OrchardCheckpoint checkpoint;
};

// Address of a subtree in the shard tree.
struct OrchardShardAddress {
  uint8_t level = 0;
  uint32_t index = 0;

  bool operator==(const OrchardShardAddress& other) const = default;
};

// Top part of the shard tree from the root to the shard roots level
// Used for optimization purposes in the shard tree crate.
using OrchardShardTreeCap = std::vector<uint8_t>;

// Subtree with root selected from the shard roots level.
struct OrchardShard {
  OrchardShard();
  OrchardShard(OrchardShardAddress shard_addr,
               std::optional<OrchardShardRootHash> shard_hash,
               std::vector<uint8_t> shard_data);
  ~OrchardShard();

  OrchardShard(const OrchardShard& other) = delete;
  OrchardShard& operator=(const OrchardShard& other) = delete;
  OrchardShard(OrchardShard&& other);
  OrchardShard& operator=(OrchardShard&& other);

  bool operator==(const OrchardShard& other) const = default;

  // Subtree root address.
  OrchardShardAddress address;
  // Root hash exists only on completed shards.
  std::optional<OrchardShardRootHash> root_hash;
  std::vector<uint8_t> shard_data;
  // Right-most position of the subtree leaf.
  uint32_t subtree_end_height = 0;
};

struct OrchardCommitment {
  OrchardCommitment(OrchardCommitmentValue cmu,
                    bool is_marked,
                    std::optional<uint32_t> checkpoint_id);
  OrchardCommitment();
  ~OrchardCommitment();

  OrchardCommitmentValue cmu;
  bool is_marked = false;
  std::optional<uint32_t> checkpoint_id;

  OrchardCommitment(const OrchardCommitment& other) = delete;
  OrchardCommitment& operator=(const OrchardCommitment& other) = delete;
  OrchardCommitment(OrchardCommitment&& other);
  OrchardCommitment& operator=(OrchardCommitment&& other);
};

// Compact representation of the Merkle tree on some point.
// Since batch inserting may contain gaps between scan ranges we insert
// frontier which allows to calculate node hashes and witnesses(merkle path from
// leaf to the tree root) even when previous scan ranges are not completed.
struct OrchardTreeState {
  OrchardTreeState();
  ~OrchardTreeState();
  OrchardTreeState(OrchardTreeState&& other);
  OrchardTreeState& operator=(OrchardTreeState&& other);
  OrchardTreeState(const OrchardTreeState&) = delete;
  OrchardTreeState& operator=(const OrchardTreeState&) = delete;

  // Tree state is linked to the end of some block.
  uint32_t block_height = 0u;
  // Number of leafs at the position.
  uint32_t tree_size = 0u;
  // https://docs.aztec.network/protocol-specs/l1-smart-contracts/frontier
  std::vector<uint8_t> frontier;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_INTERNAL_ORCHARD_STORAGE_ORCHARD_TYPES_H_
