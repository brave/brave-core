// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_RUST_CXX_SRC_SHARD_TREE_DELEGATE_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_RUST_CXX_SRC_SHARD_TREE_DELEGATE_H_

#include <vector>

#include "third_party/rust/cxx/v1/cxx.h"

namespace brave_wallet {
class OrchardShardTreeDelegate;
}  // namespace brave_wallet

namespace brave_wallet::orchard {

struct ShardTreeShard;
struct ShardTreeCap;
struct ShardTreeAddress;
struct ShardTreeCheckpoint;

struct BoolResultWrapper;
struct CheckpointBundleResultWrapper;
struct CheckpointCountResultWrapper;
struct CheckpointIdResultWrapper;
struct CheckpointsResultWrapper;
struct ShardRootsResultWrapper;
struct ShardTreeCapResultWrapper;
struct ShardTreeShardResultWrapper;

class ShardTreeDelegate {
 public:
  explicit ShardTreeDelegate(
      std::unique_ptr<OrchardShardTreeDelegate> delegate);
  ~ShardTreeDelegate();

  ::rust::Box<ShardTreeShardResultWrapper> LastShard(uint8_t shard_level) const;
  ::rust::Box<BoolResultWrapper> PutShard(const ShardTreeShard& tree) const;
  ::rust::Box<ShardTreeShardResultWrapper> GetShard(
      const ShardTreeAddress& addr) const;
  ::rust::Box<ShardRootsResultWrapper> GetShardRoots(uint8_t shard_level) const;
  ::rust::Box<BoolResultWrapper> Truncate(
      const ShardTreeAddress& address) const;
  ::rust::Box<ShardTreeCapResultWrapper> GetCap() const;
  ::rust::Box<BoolResultWrapper> PutCap(const ShardTreeCap& tree) const;
  ::rust::Box<CheckpointIdResultWrapper> MinCheckpointId() const;
  ::rust::Box<CheckpointIdResultWrapper> MaxCheckpointId() const;
  ::rust::Box<BoolResultWrapper> AddCheckpoint(
      uint32_t checkpoint_id,
      const ShardTreeCheckpoint& checkpoint) const;
  ::rust::Box<CheckpointCountResultWrapper> CheckpointCount() const;
  ::rust::Box<CheckpointBundleResultWrapper> CheckpointAtDepth(
      size_t depth) const;
  ::rust::Box<CheckpointBundleResultWrapper> GetCheckpoint(
      uint32_t checkpoint_id) const;
  ::rust::Box<BoolResultWrapper> UpdateCheckpoint(
      uint32_t checkpoint_id,
      const ShardTreeCheckpoint& checkpoint) const;
  ::rust::Box<BoolResultWrapper> RemoveCheckpoint(uint32_t checkpoint_id) const;
  ::rust::Box<BoolResultWrapper> TruncateCheckpoint(
      uint32_t checkpoint_id) const;
  ::rust::Box<CheckpointsResultWrapper> GetCheckpoints(size_t limit) const;

 private:
  std::unique_ptr<OrchardShardTreeDelegate> delegate_;
};

}  // namespace brave_wallet::orchard

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_RUST_CXX_SRC_SHARD_TREE_DELEGATE_H_
