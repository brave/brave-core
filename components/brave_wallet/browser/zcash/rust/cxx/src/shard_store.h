// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_RUST_CXX_SRC_SHARD_STORE_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_RUST_CXX_SRC_SHARD_STORE_H_

#include "third_party/rust/cxx/v1/cxx.h"

namespace brave_wallet {
class OrchardShardTreeDelegate;
}  // namespace brave_wallet

namespace brave_wallet::orchard {

enum class ShardStoreStatusCode : uint32_t;
struct ShardTreeShard;
struct ShardTreeAddress;
struct ShardTreeCheckpoint;
struct ShardTreeCap;
struct ShardTreeCheckpointBundle;

class ShardTreeDelegate {
 public:
  explicit ShardTreeDelegate(
      std::unique_ptr<OrchardShardTreeDelegate> delegate);
  ~ShardTreeDelegate();

  ShardStoreStatusCode LastShard(ShardTreeShard& into,
                                 uint8_t shard_level) const;
  ShardStoreStatusCode PutShard(const ShardTreeShard& tree) const;
  ShardStoreStatusCode GetShard(const ShardTreeAddress& addr,
                                ShardTreeShard& tree) const;
  ShardStoreStatusCode GetShardRoots(

      ::rust::Vec<ShardTreeAddress>& into,
      uint8_t shard_level) const;
  ShardStoreStatusCode Truncate(const ShardTreeAddress& address) const;
  ShardStoreStatusCode GetCap(ShardTreeCap& into) const;
  ShardStoreStatusCode PutCap(const ShardTreeCap& tree) const;
  ShardStoreStatusCode MinCheckpointId(uint32_t& into) const;
  ShardStoreStatusCode MaxCheckpointId(uint32_t& into) const;
  ShardStoreStatusCode AddCheckpoint(
      uint32_t checkpoint_id,
      const ShardTreeCheckpoint& checkpoint) const;
  ShardStoreStatusCode CheckpointCount(size_t& into) const;
  ShardStoreStatusCode CheckpointAtDepth(
      size_t depth,
      uint32_t& into_checkpoint_id,
      ShardTreeCheckpoint& into_checpoint) const;
  ShardStoreStatusCode GetCheckpoint(uint32_t checkpoint_id,
                                     ShardTreeCheckpoint& into) const;
  ShardStoreStatusCode UpdateCheckpoint(
      uint32_t checkpoint_id,
      const ShardTreeCheckpoint& checkpoint) const;
  ShardStoreStatusCode RemoveCheckpoint(uint32_t checkpoint_id) const;
  ShardStoreStatusCode TruncateCheckpoint(uint32_t checkpoint_id) const;
  ShardStoreStatusCode GetCheckpoints(
      size_t limit,
      ::rust::Vec<ShardTreeCheckpointBundle>& into) const;

 private:
  std::unique_ptr<OrchardShardTreeDelegate> delegate_;
};

}  // namespace brave_wallet::orchard

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_RUST_CXX_SRC_SHARD_STORE_H_
