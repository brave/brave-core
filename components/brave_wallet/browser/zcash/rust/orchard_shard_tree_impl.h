#include "brave/components/brave_wallet/browser/zcash/rust/orchard_shard_tree.h"
#include "brave/components/brave_wallet/browser/zcash/rust/lib.rs.h"

namespace brave_wallet::orchard {

class OrchardShardTreeImpl : public OrchardShardTree {
 public:
  OrchardShardTreeImpl(rust::Box<OrchardShardTree> orcard_shard_tree);
  ~OrchardShardTreeImpl();
  bool InsertCommitments(std::vector<OrchardCommitment> commitments) override;
  bool InsertSubtreeRoots(std::vector<OrchardShard> subtrees) override;

  ~OrchardShardTreeImpl() override;

 private:
  static std::unique_ptr<OrchardShardTreeImpl> Create(
      std::unique_ptr<::brave_wallet::orchard::ShardStoreContext> context);

  ::rust::Box<OrchardShardTree> orcard_shard_tree_;
};

}  // brave_wallet::orchard
