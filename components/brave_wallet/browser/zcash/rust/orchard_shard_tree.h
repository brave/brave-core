// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <vector>

#include "brave/components/brave_wallet/common/zcash_utils.h"

namespace brave_wallet::orchard {

class OrchardShardTreeDelegate {
 public:
  virtual ~OrchardShardTreeDelegate() = 0;
  virtual std::optional<OrchardShard> GetCap() = 0;
  virtual bool PutCap(OrchardShard shard) = 0;
  virtual bool Truncate(OrchardShardAddress address) = 0;
  virtual std::optional<uint32_t> GetLatestShardIndex() = 0;
  virtual bool PutShard(OrchardShard shard) = 0;
  virtual std::optional<OrchardShard> GetShard(OrchardShardAddress address) = 0;
  virtual std::optional<OrchardShard> LastShard(uint8_t shard_height) = 0;
  virtual std::optional<uint32_t> CheckpointCount() = 0;
  virtual std::optional<uint32_t> MinCheckpointId() = 0;
  virtual std::optional<uint32_t> MaxCheckpointId() = 0;
  virtual std::optional<uint32_t> GetCheckpointAtDepth(uint32_t depth) = 0;
  virtual bool PutShardRoots(
      uint8_t shard_roots_height,
      uint32_t start_position,
      std::vector<OrchardShard> roots) = 0;
  virtual std::optional<std::vector<OrchardShardAddress>> GetShardRoots(uint8_t shard_level) = 0;
};

class OrchardShardTree {
 public:
  virtual ~OrchardShardTree() = 0;

  virtual bool InsertSubtreeRoots(std::vector<OrchardShard> subtrees) = 0;
  virtual bool InsertCommitments(std::vector<OrchardCommitment> commitments) = 0;

  static std::unique_ptr<OrchardShardTree> Create(std::unique_ptr<OrchardShardTreeDelegate> delegate);
};

}  // namespace brave_wallet::orchard
