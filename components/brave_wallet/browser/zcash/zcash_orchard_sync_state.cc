/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/zcash/zcash_orchard_sync_state.h"

#include <utility>

#include "base/check_is_test.h"
#include "base/containers/extend.h"
#include "brave/components/brave_wallet/browser/internal/orchard_storage/orchard_shard_tree_delegate.h"
#include "brave/components/brave_wallet/common/zcash_utils.h"

namespace brave_wallet {

ZCashOrchardSyncState::ZCashOrchardSyncState(base::FilePath path_to_database) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  storage_ = std::make_unique<ZCashOrchardStorage>(path_to_database);
}

ZCashOrchardSyncState::~ZCashOrchardSyncState() = default;

OrchardShardTreeManager& ZCashOrchardSyncState::GetOrCreateShardTreeManager(
    const mojom::AccountIdPtr& account_id) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (shard_tree_managers_.find(account_id) == shard_tree_managers_.end()) {
    shard_tree_managers_[account_id.Clone()] = OrchardShardTreeManager::Create(
        base::WrapUnique(new OrchardShardTreeDelegate(account_id, *storage_)));
  }
  auto* manager = shard_tree_managers_[account_id.Clone()].get();
  CHECK(manager);
  return *manager;
}

base::expected<ZCashOrchardStorage::AccountMeta, ZCashOrchardStorage::Error>
ZCashOrchardSyncState::RegisterAccount(const mojom::AccountIdPtr& account_id,
                                       uint64_t account_birthday_block) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return storage_->RegisterAccount(std::move(account_id),
                                   account_birthday_block);
}

base::expected<ZCashOrchardStorage::AccountMeta, ZCashOrchardStorage::Error>
ZCashOrchardSyncState::GetAccountMeta(const mojom::AccountIdPtr& account_id) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return storage_->GetAccountMeta(std::move(account_id));
}

std::optional<ZCashOrchardStorage::Error>
ZCashOrchardSyncState::HandleChainReorg(const mojom::AccountIdPtr& account_id,
                                        uint32_t reorg_block_id,
                                        const std::string& reorg_block_hash) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return storage_->HandleChainReorg(std::move(account_id), reorg_block_id,
                                    reorg_block_hash);
}

base::expected<std::vector<OrchardNote>, ZCashOrchardStorage::Error>
ZCashOrchardSyncState::GetSpendableNotes(
    const mojom::AccountIdPtr& account_id) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return storage_->GetSpendableNotes(std::move(account_id));
}

base::expected<std::vector<OrchardNoteSpend>, ZCashOrchardStorage::Error>
ZCashOrchardSyncState::GetNullifiers(const mojom::AccountIdPtr& account_id) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return storage_->GetNullifiers(std::move(account_id));
}

std::optional<ZCashOrchardStorage::Error> ZCashOrchardSyncState::UpdateNotes(
    const mojom::AccountIdPtr& account_id,
    OrchardBlockScanner::Result block_scanner_results,
    const uint32_t latest_scanned_block,
    const std::string& latest_scanned_block_hash) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  auto existing_notes = storage_->GetSpendableNotes(account_id);
  if (!existing_notes.has_value()) {
    return existing_notes.error();
  }

  std::vector<OrchardNote> notes_to_add =
      block_scanner_results.discovered_notes;
  base::Extend(existing_notes.value(), notes_to_add);

  std::vector<OrchardNoteSpend> nf_to_add;

  for (const auto& nf : block_scanner_results.found_spends) {
    if (std::find_if(existing_notes.value().begin(),
                     existing_notes.value().end(), [&nf](const auto& v) {
                       return v.nullifier == nf.nullifier;
                     }) != existing_notes.value().end()) {
      nf_to_add.push_back(nf);
    }
  }

  if (!GetOrCreateShardTreeManager(account_id)
           .InsertCommitments(std::move(block_scanner_results))) {
    return ZCashOrchardStorage::Error{
        ZCashOrchardStorage::ErrorCode::kInternalError,
        "Failed to insert commitments"};
  }

  return storage_->UpdateNotes(std::move(account_id), notes_to_add,
                               std::move(nf_to_add), latest_scanned_block,
                               latest_scanned_block_hash);
}

base::expected<bool, ZCashOrchardStorage::Error>
ZCashOrchardSyncState::ResetAccountSyncState(
    const mojom::AccountIdPtr& account_id) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return storage_->ResetAccountSyncState(std::move(account_id));
}

base::expected<std::vector<OrchardInput>, ZCashOrchardStorage::Error>
ZCashOrchardSyncState::CalculateWitnessForCheckpoint(
    const mojom::AccountIdPtr& account_id,
    const std::vector<OrchardInput>& notes,
    uint32_t checkpoint_position) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  auto result = GetOrCreateShardTreeManager(account_id)
                    .CalculateWitness(notes, checkpoint_position);
  if (!result.has_value()) {
    return base::unexpected(ZCashOrchardStorage::Error{
        ZCashOrchardStorage::ErrorCode::kConsistencyError, result.error()});
  }
  return base::ok(std::move(result.value()));
}

void ZCashOrchardSyncState::ResetDatabase() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  storage_->ResetDatabase();
}

}  // namespace brave_wallet
