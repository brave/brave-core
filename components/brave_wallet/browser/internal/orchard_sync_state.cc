/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/internal/orchard_sync_state.h"

#include <utility>

#include "base/check_is_test.h"
#include "base/containers/extend.h"
#include "brave/components/brave_wallet/browser/internal/orchard_storage/orchard_types.h"
#include "brave/components/brave_wallet/browser/zcash/rust/orchard_shard_tree.h"
#include "brave/components/brave_wallet/common/zcash_utils.h"

namespace brave_wallet {

OrchardSyncState::OrchardSyncState(const base::FilePath& path_to_database) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  storage_ = std::make_unique<OrchardStorage>(path_to_database);
}

OrchardSyncState::~OrchardSyncState() = default;

orchard::OrchardShardTree& OrchardSyncState::GetOrCreateShardTree(
    const mojom::AccountIdPtr& account_id) LIFETIME_BOUND {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  std::unique_ptr<orchard::OrchardShardTree>& manager =
      shard_trees_[account_id->unique_key];
  if (!manager) {
    manager = orchard::OrchardShardTree::Create(*storage_, account_id);
  }
  return *manager;
}

base::expected<OrchardStorage::AccountMeta, OrchardStorage::Error>
OrchardSyncState::RegisterAccount(const mojom::AccountIdPtr& account_id,
                                  uint64_t account_birthday_block) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return storage_->RegisterAccount(account_id, account_birthday_block);
}

base::expected<OrchardStorage::AccountMeta, OrchardStorage::Error>
OrchardSyncState::GetAccountMeta(const mojom::AccountIdPtr& account_id) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return storage_->GetAccountMeta(account_id);
}

std::optional<OrchardStorage::Error> OrchardSyncState::HandleChainReorg(
    const mojom::AccountIdPtr& account_id,
    uint32_t reorg_block_id,
    const std::string& reorg_block_hash) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return storage_->HandleChainReorg(account_id, reorg_block_id,
                                    reorg_block_hash);
}

base::expected<std::vector<OrchardNote>, OrchardStorage::Error>
OrchardSyncState::GetSpendableNotes(const mojom::AccountIdPtr& account_id) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return storage_->GetSpendableNotes(account_id);
}

base::expected<std::vector<OrchardNoteSpend>, OrchardStorage::Error>
OrchardSyncState::GetNullifiers(const mojom::AccountIdPtr& account_id) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return storage_->GetNullifiers(account_id);
}

std::optional<OrchardStorage::Error> OrchardSyncState::ApplyScanResults(
    const mojom::AccountIdPtr& account_id,
    OrchardBlockScanner::Result block_scanner_results,
    const uint32_t latest_scanned_block,
    const std::string& latest_scanned_block_hash) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  auto existing_notes = storage_->GetSpendableNotes(account_id);
  if (!existing_notes.has_value()) {
    return existing_notes.error();
  }

  size_t offset = existing_notes.value().size();
  base::Extend(existing_notes.value(),
               std::move(block_scanner_results.discovered_notes));

  base::span<const OrchardNote> notes_to_add =
      base::span(existing_notes.value()).subspan(offset);
  std::vector<OrchardNoteSpend> nf_to_add;

  for (const auto& nf : block_scanner_results.found_spends) {
    if (base::ranges::find_if(*existing_notes, [&nf](const auto& v) {
          return v.nullifier == nf.nullifier;
        }) != existing_notes.value().end()) {
      nf_to_add.push_back(nf);
    }
  }

  if (!GetOrCreateShardTree(account_id)
           .ApplyScanResults(std::move(block_scanner_results.scanned_blocks))) {
    return OrchardStorage::Error{OrchardStorage::ErrorCode::kInternalError,
                                 "Failed to insert commitments"};
  }

  return storage_->UpdateNotes(account_id, std::move(notes_to_add),
                               std::move(nf_to_add), latest_scanned_block,
                               latest_scanned_block_hash);
}

base::expected<OrchardStorage::Result, OrchardStorage::Error>
OrchardSyncState::ResetAccountSyncState(const mojom::AccountIdPtr& account_id) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return storage_->ResetAccountSyncState(account_id);
}

base::expected<std::vector<OrchardInput>, OrchardStorage::Error>
OrchardSyncState::CalculateWitnessForCheckpoint(
    const mojom::AccountIdPtr& account_id,
    const std::vector<OrchardInput>& notes,
    uint32_t checkpoint_position) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  auto& shard_tree = GetOrCreateShardTree(account_id);

  std::vector<OrchardInput> result;
  result.reserve(notes.size());
  for (auto& input : notes) {
    auto witness = shard_tree.CalculateWitness(
        input.note.orchard_commitment_tree_position, checkpoint_position);
    if (!witness.has_value()) {
      return base::unexpected(OrchardStorage::Error{
          OrchardStorage::ErrorCode::kInternalError, witness.error()});
    }
    result.push_back(input);
    result.back().witness = std::move(witness.value());
  }
  return base::ok(std::move(result));
}

void OrchardSyncState::ResetDatabase() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  storage_->ResetDatabase();
}

base::expected<OrchardStorage::Result, OrchardStorage::Error>
OrchardSyncState::Truncate(const mojom::AccountIdPtr& account_id,
                           uint32_t checkpoint_id) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return GetOrCreateShardTree(account_id).TruncateToCheckpoint(checkpoint_id)
             ? OrchardStorage::Result::kSuccess
             : OrchardStorage::Result::kFailure;
}

// Testing
void OrchardSyncState::OverrideShardTreeForTesting(
    const mojom::AccountIdPtr& account_id) {
  shard_trees_[account_id->unique_key] =
      orchard::OrchardShardTree::CreateForTesting(*storage_, account_id);
}

OrchardStorage* OrchardSyncState::orchard_storage() {
  return storage_.get();
}

}  // namespace brave_wallet
