// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_RUST_CXX_ORCHARD_SHARD_TREE_DELEGATE_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_RUST_CXX_ORCHARD_SHARD_TREE_DELEGATE_H_

#include "base/memory/raw_ref.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "third_party/rust/cxx/v1/cxx.h"

namespace brave_wallet {

class OrchardStorage;

namespace orchard {

struct CxxOrchardShard;
struct CxxOrchardShardTreeCap;
struct CxxOrchardShardAddress;
struct CxxOrchardCheckpoint;

struct BoolResultWrapper;
struct CheckpointBundleResultWrapper;
struct CheckpointCountResultWrapper;
struct CheckpointIdResultWrapper;
struct CheckpointsResultWrapper;
struct OrchardShardResultWrapper;
struct OrchardShardTreeCapResultWrapper;
struct ShardRootsResultWrapper;

class CxxOrchardShardTreeDelegate {
 public:
  explicit CxxOrchardShardTreeDelegate(OrchardStorage& storage,
                                       const mojom::AccountIdPtr& account_id);
  ~CxxOrchardShardTreeDelegate();

  ::rust::Box<OrchardShardResultWrapper> LastShard(uint8_t shard_level) const;
  ::rust::Box<BoolResultWrapper> PutShard(const CxxOrchardShard& tree) const;
  ::rust::Box<OrchardShardResultWrapper> GetShard(
      const CxxOrchardShardAddress& addr) const;
  ::rust::Box<ShardRootsResultWrapper> GetShardRoots(uint8_t shard_level) const;
  ::rust::Box<BoolResultWrapper> Truncate(
      const CxxOrchardShardAddress& address) const;
  ::rust::Box<OrchardShardTreeCapResultWrapper> GetCap() const;
  ::rust::Box<BoolResultWrapper> PutCap(
      const CxxOrchardShardTreeCap& tree) const;
  ::rust::Box<CheckpointIdResultWrapper> MinCheckpointId() const;
  ::rust::Box<CheckpointIdResultWrapper> MaxCheckpointId() const;
  ::rust::Box<BoolResultWrapper> AddCheckpoint(
      uint32_t checkpoint_id,
      const CxxOrchardCheckpoint& checkpoint) const;
  ::rust::Box<CheckpointCountResultWrapper> CheckpointCount() const;
  ::rust::Box<CheckpointBundleResultWrapper> CheckpointAtDepth(
      size_t depth) const;
  ::rust::Box<CheckpointBundleResultWrapper> GetCheckpoint(
      uint32_t checkpoint_id) const;
  ::rust::Box<BoolResultWrapper> UpdateCheckpoint(
      uint32_t checkpoint_id,
      const CxxOrchardCheckpoint& checkpoint) const;
  ::rust::Box<BoolResultWrapper> RemoveCheckpoint(uint32_t checkpoint_id) const;
  ::rust::Box<BoolResultWrapper> TruncateCheckpoint(
      uint32_t checkpoint_id) const;
  ::rust::Box<CheckpointsResultWrapper> GetCheckpoints(size_t limit) const;

 private:
  raw_ref<OrchardStorage> storage_;
  ::brave_wallet::mojom::AccountIdPtr account_id_;
};

}  // namespace orchard
}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_RUST_CXX_ORCHARD_SHARD_TREE_DELEGATE_H_
