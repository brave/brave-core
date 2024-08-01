/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/internal/orchard_shard_tree_manager.h"
#include "brave/components/brave_wallet/browser/zcash/rust/cxx/src/shard_store.h"
namespace brave_wallet {

class OrchardShardTreeContexImpl : public orchard::ShardStoreContext {
 public:
  OrchardShardTreeContexImpl(std::unique_ptr<OrchardShardTreeDelegate> delegate);
  ~OrchardShardTreeContexImpl();

  ::rust::Box<ShardTree> GetShard(const ShardAddress& addr) override;
  ::rust::Box<ShardTree> LastShard() override;
  bool pust_shard(const ShardTree& tree) override;
  ::rust::Vec<ShardAddress> GetShardRoots() override;
  bool Truncate(const ShardAddress& address) override;
  ::rust::Box<ShardTree> GetCap() override;
  bool PutCap(const ShardTree& tree) override;
  ::rust::Box<CheckpointId> MinCheckpointId(ShardStoreContext& context) override;
  ::rust::Box<CheckpointId> MaxCheckpointId(ShardStoreContext& context) override;
  bool AddCheckpoint(const CheckpointId& checkpoint_id, const Checkpoint& checkpoint) override;
  uint32_t CheckpointCount() override;
  ::rust::Box<Checkpoint> GetCheckpointAtDepth(uint32_t depth) override;
  ::rust::Box<Checkpoint> GetCheckpoint(const CheckpointId& checkpoint_id) override;
  ::rust::Vec<Checkpoint> GetCheckpoints() override;
  bool UpdateCheckpointWith(const Checkpoint& checkpoint_id) override;
  bool RemoveCheckpoint(const CheckpointId& checkpoint_id) override;
  bool TruncateCheckpoint(const CheckpointId& checkpoint_id) override;
};

// static
std::unique_ptr<OrchardShardTreeManager> OrchardShardTreeManager::Create(
    std::unique_ptr<orchard::OrchardShardTreeDelegate> delegate) {
  auto shard_tree = orchard::ShardTree::Create(std::move(delegate));
  if (!shard_tree) {
    // NOTREACHED
    return nullptr;
  }
  return std::make_unique<OrchardShardTreeManager>(std::move(shard_tree));
}

OrchardShardTreeManager::OrchardShardTreeManager(
    std::unique_ptr<::brave_wallet::orchard::OrchardShardTree> shard_tree) {
  orchard_shard_tree_ = std::move(shard_tree);
}

OrchardShardTreeManager::~OrchardShardTreeManager() {
}

bool OrchardShardTreeManager::InsertCommitments(std::vector<OrchardCommitment> commitments) {
  return orchard_shard_tree_->InsertCommitments(std::move(commitments));
}

bool OrchardShardTreeManager::InsertSubtreeRoots(std::vector<OrchardShard> roots) {
  return orchard_shard_tree_->InsertSubtreeRoots(std::move(roots));
}

}  // namespace brave_wallet
