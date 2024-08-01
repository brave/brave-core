#include "brave/components/brave_wallet/browser/zcash/rust/cxx/src/shard_store.h"
#include "brave/components/brave_wallet/browser/zcash/rust/orchard_shard_tree_impl.h"

namespace brave_wallet::orchard {

class ShardStoreContextImpl : ShardStoreContext {
 public:
  ::rust::Box<ShardTree> GetShard(const ShardAddress& addr) override {
    delegate_->GetShard(ShardAddress(addr.level, addr.index));
  }
  ::rust::Box<ShardTree> LastShard() override {
    return  ::rust::Box<ShardTree>(ShardTree());
  }
  bool pust_shard(const ShardTree& tree) override {
    return false;
  }
  ::rust::Vec<ShardAddress> GetShardRoots() override {
    return ::rust::Vec<ShardAddress>();
  }
  bool Truncate(const ShardAddress& address) override {
    return false;
  }
  ::rust::Box<ShardTree> GetCap() override {
    return ::rust::Box<ShardTree>(ShardTree());
  }
  bool PutCap(const ShardTree& tree) override {
    return false;
  }
  ::rust::Box<CheckpointId> MinCheckpointId(ShardStoreContext& context) override {
    return ::rust::Box<CheckpointId>(CheckpointId());
  }
  ::rust::Box<CheckpointId> MaxCheckpointId(ShardStoreContext& context) override {
    return ::rust::Box<CheckpointId>(CheckpointId());
  }
  bool AddCheckpoint(const CheckpointId& checkpoint_id, const Checkpoint& checkpoint) override {
    return false;
  }
  uint32_t CheckpointCount() override {
    return 0;
  }
  ::rust::Box<Checkpoint> GetCheckpointAtDepth(uint32_t depth) override {
    return ::rust::Box<Checkpoint>(Checkpoint());
  }
  ::rust::Box<Checkpoint> GetCheckpoint(const CheckpointId& checkpoint_id) override {
    return ::rust::Box<Checkpoint>(Checkpoint());
  }
  ::rust::Vec<Checkpoint> GetCheckpoints() override {
    return ::rust::Vec<Checkpoint>();
  }
  bool UpdateCheckpointWith(const Checkpoint& checkpoint_id) override {
    return false;
  }
  bool RemoveCheckpoint(const CheckpointId& checkpoint_id) override {
    return false;
  }
  bool TruncateCheckpoint(const CheckpointId& checkpoint_id) override {
    return false;
  }
 private:
  std::unique_ptr<OrchardShardTreeDelegate> delegate_;
};

bool OrchardShardTreeImpl::InsertCommitments(std::vector<::brave_wallet::OrchardCommitment> commitments) {
  ::rust::Vec<OrchardCommitment> rust_commitments;
  for (const auto& item : commitments) {
    OrchardCommitment commitment;
    rust_commitments.emplace_back(std::move(orchard_compact_action));
  }
  return orcard_shard_tree_->insert_commitments(std::move(rust_commitments));
}

bool OrchardShardTreeImpl::InsertSubtreeRoots(std::vector<::brave_wallet::OrchardShard> subtrees) {
  ::rust::Vec<ShardTree> rust_subtrees;
  for (const auto& item : subtrees) {
    ShardTree rust_subtree;
    rust_subtrees.emplace_back(std::move(rust_subtree));
  }
  return orcard_shard_tree_->insert_subtree_roots(std::move(rust_subtrees));
}


OrchardShardTreeImpl::~OrchardShardTreeImpl() {
}

// static
std::unique_ptr<OrchardShardTree> OrchardShardTree::Create(
    std::unique_ptr<OrchardShardTreeDelegate> delegate) {
  auto shard_tree_result =
      ::brave_wallet::orchard::create_shard_tree(ShardStoreContextImpl(std::move(delegate)));
  if (!shard_tree_result.is_ok()) {
    return nullptr;
  }
  return base::WrapUnique<OrchardShardTree>(new OrchardShardTreeImpl(std::move(shard_tree_result.unwrap())));
}

}  // brave_wallet::orchard
