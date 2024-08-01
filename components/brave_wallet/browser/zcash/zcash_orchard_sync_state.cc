#include "brave/components/brave_wallet/browser/zcash/zcash_orchard_sync_state.h"

namespace brave_wallet {

ZCashOrchardSyncState::ZCashOrchardSyncState(base::FilePath path_to_database) {
  storage_ = std::make_unique<ZCashOrchardStorage>(path_to_database);
  shard_tree_manager_ = std::make_unique<OrchardShardTreeManager>(storage_.get());
}

std::optional<ZCashOrchardStorage::Error> ZCashOrchardSyncState::RegisterAccount(
    mojom::AccountIdPtr account_id,
    uint64_t account_birthday_block,
    const std::string& account_bithday_block_hash) {
  return storage_->RegisterAccount(std::move(account_id, account_birthday_block, account_bithday_block_hash ));
}

base::expected<ZCashOrchardStorage::AccountMeta, ZCashOrchardStorage::Error> ZCashOrchardSyncState::GetAccountMeta(
    mojom::AccountIdPtr account_id) {
  return storage_->GetAccountMeta(std::move(account_id));
}

std::optional<ZCashOrchardStorage::Error> ZCashOrchardSyncState::HandleChainReorg(mojom::AccountIdPtr account_id,
                                                           uint32_t reorg_block_id,
                                                                                  const std::string& reorg_block_hash) {
  return storage_->HandleChainReorg(std::move(account_id), reorg_block_id, reorg_block_hash);

}

base::expected<std::vector<OrchardNote>, ZCashOrchardStorage::Error>
ZCashOrchardSyncState::GetSpendableNotes(mojom::AccountIdPtr account_id) {
  return storage_->GetSpendableNotes(std::move(account_id));
}

base::expected<std::vector<OrchardNullifier>, ZCashOrchardStorage::Error> ZCashOrchardSyncState::GetNullifiers(
    mojom::AccountIdPtr account_id) {
  return storage_->GetNullifiers(std::move(account_id));
}

std::optional<ZCashOrchardStorage::Error> ZCashOrchardSyncState::UpdateNotes(
    mojom::AccountIdPtr account_id,
    const std::vector<OrchardNote>& notes_to_add,
    const std::vector<OrchardNullifier>& notes_to_delete,
    const uint32_t latest_scanned_block,
    const std::string& latest_scanned_block_hash) {
  return storage_->UpdateNotes(std::move(account_id), notes_to_add, notes_to_delete,
                               latest_scanned_block, latest_scanned_block_hash);

}

void ZCashOrchardSyncState::ResetDatabase() {
  storage_->ResetDatabase();
}

}  // namespace brave_wallet
