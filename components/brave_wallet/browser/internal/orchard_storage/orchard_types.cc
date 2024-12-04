/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/internal/orchard_storage/orchard_types.h"

#include "brave/components/brave_wallet/browser/internal/orchard_storage/orchard_storage.h"

namespace brave_wallet {

OrchardTreeState::OrchardTreeState() = default;
OrchardTreeState::~OrchardTreeState() = default;
OrchardTreeState::OrchardTreeState(OrchardTreeState&& other) = default;
OrchardTreeState& OrchardTreeState::operator=(OrchardTreeState&& other) =
    default;

OrchardCheckpoint::OrchardCheckpoint() = default;
OrchardCheckpoint::OrchardCheckpoint(CheckpointTreeState tree_state_position,
                                     std::vector<uint32_t> marks_removed)
    : tree_state_position(tree_state_position),
      marks_removed(std::move(marks_removed)) {}
OrchardCheckpoint::~OrchardCheckpoint() = default;
OrchardCheckpoint::OrchardCheckpoint(OrchardCheckpoint&& other) = default;
OrchardCheckpoint& OrchardCheckpoint::operator=(OrchardCheckpoint&& other) =
    default;
OrchardCheckpoint OrchardCheckpoint::Clone() {
  return OrchardCheckpoint(tree_state_position, marks_removed);
}

OrchardCheckpointBundle::OrchardCheckpointBundle(uint32_t checkpoint_id,
                                                 OrchardCheckpoint checkpoint)
    : checkpoint_id(checkpoint_id), checkpoint(std::move(checkpoint)) {}
OrchardCheckpointBundle::~OrchardCheckpointBundle() = default;
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
OrchardShard::OrchardShard(OrchardShard&& other) = default;
OrchardShard& OrchardShard::operator=(OrchardShard&& other) = default;

OrchardCommitment::OrchardCommitment(OrchardCommitmentValue cmu,
                                     bool is_marked,
                                     std::optional<uint32_t> checkpoint_id)
    : cmu(cmu), is_marked(is_marked), checkpoint_id(checkpoint_id) {}
OrchardCommitment::OrchardCommitment() = default;
OrchardCommitment::~OrchardCommitment() = default;
OrchardCommitment::OrchardCommitment(OrchardCommitment&& other) = default;
OrchardCommitment& OrchardCommitment::operator=(OrchardCommitment&& other) =
    default;

}  // namespace brave_wallet
