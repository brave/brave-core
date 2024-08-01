#include "brave/components/brave_wallet/browser/zcash/zcash_orchard_storage.h"
#include "brave/components/brave_wallet/browser/internal/orchard_shard_tree_manager.h"
#include "brave/components/brave_wallet/common/zcash_utils.h"

namespace brave_wallet {

class ZCashOrchardSyncState {
 public:
  ZCashOrchardSyncState(base::FilePath path_to_database);
  ~ZCashOrchardSyncState();

  std::optional<ZCashOrchardStorage::Error> RegisterAccount(
      mojom::AccountIdPtr account_id,
      uint64_t account_birthday_block,
      const std::string& account_bithday_block_hash);

  base::expected<ZCashOrchardStorage::AccountMeta, ZCashOrchardStorage::Error> GetAccountMeta(
      mojom::AccountIdPtr account_id);

  std::optional<ZCashOrchardStorage::Error> HandleChainReorg(mojom::AccountIdPtr account_id,
                                        uint32_t reorg_block_id,
                                        const std::string& reorg_block_hash);

  base::expected<std::vector<OrchardNote>, ZCashOrchardStorage::Error>
  GetSpendableNotes(mojom::AccountIdPtr account_id);

  base::expected<std::vector<OrchardNullifier>, ZCashOrchardStorage::Error> GetNullifiers(
      mojom::AccountIdPtr account_id);

  std::optional<ZCashOrchardStorage::Error> UpdateNotes(
      mojom::AccountIdPtr account_id,
      const std::vector<OrchardNote>& notes_to_add,
      const std::vector<OrchardNullifier>& notes_to_delete,
      const uint32_t latest_scanned_block,
      const std::string& latest_scanned_block_hash);

  void ResetDatabase();

  void UpdateSubtreeRoots(mojom::AccountIdPtr account_id, std::vector<zcash::mojom::SubtreeRootPtr> roots);
  void AppendLeafs(mojom::AccountIdPtr account_id, std::vector<OrchardCommitment> commitments);

 private:

  base::RefCounted<ZCashOrchardStorage> storage_;
  std::unique_ptr<OrchardShardTreeManager> shard_tree_manager_;

};

}  // brave_wallet
