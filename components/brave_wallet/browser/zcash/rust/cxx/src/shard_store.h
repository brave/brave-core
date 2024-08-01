// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_RUST_CXX_SRC_SHARD_STORE_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_RUST_CXX_SRC_SHARD_STORE_H_

#include "third_party/rust/cxx/v1/cxx.h"

namespace brave_wallet::orchard {

struct ShardTree;
struct ShardAddress;
struct CheckpointId;
struct Checkpoint;

class ShardStoreContext {
 public:
  virtual ~ShardStoreContext() = default;

  virtual ::rust::Box<ShardTree> GetShard(const ShardAddress& addr) = 0;
  virtual ::rust::Box<ShardTree> LastShard() = 0;
  virtual bool pust_shard(const ShardTree& tree) = 0;
  virtual ::rust::Vec<ShardAddress> GetShardRoots() = 0;
  virtual bool Truncate(const ShardAddress& address) = 0;
  virtual ::rust::Box<ShardTree> GetCap() = 0;
  virtual bool PutCap(const ShardTree& tree) = 0;
  virtual ::rust::Box<CheckpointId> MinCheckpointId(ShardStoreContext& context) = 0;
  virtual ::rust::Box<CheckpointId> MaxCheckpointId(ShardStoreContext& context) = 0;
  virtual bool AddCheckpoint(const CheckpointId& checkpoint_id, const Checkpoint& checkpoint) = 0;
  virtual uint32_t CheckpointCount() = 0;
  virtual ::rust::Box<Checkpoint> GetCheckpointAtDepth(uint32_t depth);
  virtual ::rust::Box<Checkpoint> GetCheckpoint(const CheckpointId& checkpoint_id);
  virtual ::rust::Vec<Checkpoint> GetCheckpoints() = 0;
  virtual bool UpdateCheckpointWith(const Checkpoint& checkpoint_id) = 0;
  virtual bool RemoveCheckpoint(const CheckpointId& checkpoint_id) = 0;
  virtual bool TruncateCheckpoint(const CheckpointId& checkpoint_id) = 0;
};

// ::rust::Box<ShardTree> get_shard(ShardStoreContext& context, const ShardAddress& address);
// ::rust::Box<ShardTree> last_shard(ShardStoreContext& context);
// bool pust_shard(ShardStoreContext& context, const ShardTree& tree);
// ::rust::Vec<ShardAddress> get_shard_roots(ShardStoreContext& context);
// bool truncate(ShardStoreContext& context, const ShardAddress& address);
// ::rust::Box<ShardTree> get_cap(ShardStoreContext& context);
// bool put_chap(ShardStoreContext& context, const ShardTree& tree);
// ::rust::Box<CheckpointId> min_checkpoint_id(ShardStoreContext& context);
// ::rust::Box<CheckpointId> max_checkpoint_id(ShardStoreContext& context);
// bool add_checkpoint(ShardStoreContext& context, const CheckpointId& checkpoint_id,
//                     const Checkpoint& checkpoint);
// uint32_t checkpoint_count(ShardStoreContext& context);
// ::rust::Box<Checkpoint> get_checkpoint_at_depth(ShardStoreContext& context, uint32_t depth);
// ::rust::Box<Checkpoint> get_checkpoint(ShardStoreContext& context, const CheckpointId& checkpoint_id);
// ::rust::Vec<Checkpoint> get_checkpoints(ShardStoreContext& context);
// bool update_checkpoint_with(ShardStoreContext& context, const Checkpoint& checkpoint_id);
// bool remove_checkpoint(ShardStoreContext& context, const CheckpointId& checkpoint_id);
// bool truncate_checkpoints(ShardStoreContext& context, const CheckpointId& checkpoint_id);

}  // namespace brave_wallet::orchard

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_RUST_CXX_SRC_SHARD_STORE_H_

