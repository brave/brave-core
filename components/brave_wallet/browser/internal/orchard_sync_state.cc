/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/internal/orchard_sync_state.h"

#include <algorithm>
#include <utility>

#include "base/check.h"
#include "base/check_is_test.h"
#include "base/containers/extend.h"
#include "base/types/expected_macros.h"
#include "brave/components/brave_wallet/browser/internal/orchard_storage/orchard_shard_tree_types.h"
#include "brave/components/brave_wallet/common/zcash_utils.h"

namespace brave_wallet {

OrchardSyncState::SpendableNotesBundle::SpendableNotesBundle() = default;
OrchardSyncState::SpendableNotesBundle::~SpendableNotesBundle() = default;
OrchardSyncState::SpendableNotesBundle::SpendableNotesBundle(
    OrchardSyncState::SpendableNotesBundle&&) = default;
OrchardSyncState::SpendableNotesBundle&
OrchardSyncState::SpendableNotesBundle::operator=(
    OrchardSyncState::SpendableNotesBundle&&) = default;

OrchardSyncState::OrchardSyncState(const base::FilePath& path_to_database)
    : storage_(OrchardStorage(path_to_database)) {}

OrchardSyncState::~OrchardSyncState() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
}

orchard::OrchardShardTree& OrchardSyncState::GetOrCreateShardTree(
    const mojom::AccountIdPtr& account_id) LIFETIME_BOUND {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (shard_trees_.find(account_id->unique_key) == shard_trees_.end()) {
    shard_trees_[account_id->unique_key] =
        orchard::OrchardShardTree::Create(storage_, account_id);
  }
  auto* manager = shard_trees_[account_id->unique_key].get();
  CHECK(manager);
  return *manager;
}

base::expected<OrchardStorage::Result, OrchardStorage::Error>
OrchardSyncState::RegisterAccount(const mojom::AccountIdPtr& account_id,
                                  uint64_t account_birthday_block) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  {
    auto tx = storage_.Transactionally();
    if (!tx.has_value()) {
      return base::unexpected(tx.error());
    }
    auto register_account_result =
        storage_.RegisterAccount(account_id, account_birthday_block);
    if (!register_account_result.has_value() ||
        register_account_result.value() != OrchardStorage::Result::kSuccess) {
      return register_account_result;
    }
    return tx->Commit();
  }
}

base::expected<std::optional<OrchardStorage::AccountMeta>,
               OrchardStorage::Error>
OrchardSyncState::GetAccountMeta(const mojom::AccountIdPtr& account_id) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return storage_.GetAccountMeta(account_id);
}

base::expected<OrchardStorage::Result, OrchardStorage::Error>
OrchardSyncState::Rewind(const mojom::AccountIdPtr& account_id,
                         uint32_t rewind_block_height,
                         const std::string& rewind_block_hash) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  {
    auto tx = storage_.Transactionally();
    if (!tx.has_value()) {
      return base::unexpected(tx.error());
    }
    if (!GetOrCreateShardTree(account_id)
             .TruncateToCheckpoint(rewind_block_height)) {
      return base::unexpected(
          OrchardStorage::Error{OrchardStorage::ErrorCode::kInternalError,
                                "Failed to truncate tree"});
    }
    auto chain_reorg_result = storage_.HandleChainReorg(
        account_id, rewind_block_height, rewind_block_hash);
    if (!chain_reorg_result.has_value() ||
        chain_reorg_result.value() != OrchardStorage::Result::kSuccess) {
      return chain_reorg_result;
    }
    return tx->Commit();
  }
}

base::expected<std::optional<OrchardSyncState::SpendableNotesBundle>,
               OrchardStorage::Error>
OrchardSyncState::GetSpendableNotes(const mojom::AccountIdPtr& account_id,
                                    const OrchardAddrRawPart& change_address) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  ASSIGN_OR_RETURN(auto account_meta, storage_.GetAccountMeta(account_id));
  if (!account_meta.has_value()) {
    return base::ok(std::nullopt);
  }
  auto latest_scanned_block_id = account_meta->latest_scanned_block_id;
  if (!latest_scanned_block_id) {
    return OrchardSyncState::SpendableNotesBundle();
  }
  ASSIGN_OR_RETURN(auto notes, storage_.GetSpendableNotes(account_id));
  ASSIGN_OR_RETURN(auto anchor, storage_.GetMaxCheckpointedHeight(
                                    account_id, latest_scanned_block_id.value(),
                                    kZCashInternalAddressMinConfirmations));

  SpendableNotesBundle result;
  result.anchor_block_id = anchor;
  result.all_notes = notes;

  // Anchor may not exist due chain reorg
  if (anchor) {
    result.spendable_notes = notes;
    auto to_remove = std::ranges::remove_if(
        result.spendable_notes,
        [&change_address, &anchor,
         &latest_scanned_block_id](const OrchardNote& note) {
          if (note.block_id > latest_scanned_block_id ||
              note.block_id > anchor.value()) {
            return true;
          }

          bool is_internal = note.addr == change_address;
          auto diff = latest_scanned_block_id.value() - note.block_id;
          return (is_internal &&
                  diff < kZCashInternalAddressMinConfirmations) ||
                 (!is_internal && diff < kZCashPublicAddressMinConfirmations);
        });
    result.spendable_notes.erase(to_remove.begin(), to_remove.end());
  }
  return std::move(result);
}

base::expected<std::vector<OrchardNoteSpend>, OrchardStorage::Error>
OrchardSyncState::GetNullifiers(const mojom::AccountIdPtr& account_id) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return storage_.GetNullifiers(account_id);
}

base::expected<OrchardStorage::Result, OrchardStorage::Error>
OrchardSyncState::ApplyScanResults(
    const mojom::AccountIdPtr& account_id,
    OrchardBlockScanner::Result block_scanner_results) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  auto existing_notes = storage_.GetSpendableNotes(account_id);
  RETURN_IF_ERROR(existing_notes);

  std::vector<OrchardNote> notes_to_add =
      block_scanner_results.discovered_notes;
  base::Extend(existing_notes.value(), notes_to_add);

  std::vector<OrchardNoteSpend> nf_to_add;

  for (const auto& nf : block_scanner_results.found_spends) {
    if (std::ranges::find_if(existing_notes.value(), [&nf](const auto& v) {
          return v.nullifier == nf.nullifier;
        }) != existing_notes.value().end()) {
      nf_to_add.push_back(nf);
    }
  }

  {
    auto tx = storage_.Transactionally();
    if (!tx.has_value()) {
      return base::unexpected(tx.error());
    }

    if (!GetOrCreateShardTree(account_id)
             .ApplyScanResults(
                 std::move(block_scanner_results.scanned_blocks))) {
      return base::unexpected(
          OrchardStorage::Error{OrchardStorage::ErrorCode::kInternalError,
                                "Failed to insert commitments"});
    }

    auto update_notes_result =
        storage_.UpdateNotes(account_id, notes_to_add, std::move(nf_to_add),
                             block_scanner_results.latest_scanned_block_id,
                             block_scanner_results.latest_scanned_block_hash);

    if (!update_notes_result.has_value() ||
        update_notes_result.value() != OrchardStorage::Result::kSuccess) {
      return update_notes_result;
    }

    return tx->Commit();
  }
}

base::expected<OrchardStorage::Result, OrchardStorage::Error>
OrchardSyncState::ResetAccountSyncState(const mojom::AccountIdPtr& account_id) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  {
    auto tx = storage_.Transactionally();
    if (!tx.has_value()) {
      return base::unexpected(tx.error());
    }
    auto reset_account_sync_state_result =
        storage_.ResetAccountSyncState(account_id);
    if (!reset_account_sync_state_result.has_value() ||
        reset_account_sync_state_result.value() !=
            OrchardStorage::Result::kSuccess) {
      return base::unexpected(reset_account_sync_state_result.error());
    }
    return tx->Commit();
  }
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
      return base::unexpected(
          OrchardStorage::Error{OrchardStorage::ErrorCode::kInternalError,
                                "Failed to calculate wittness"});
    }
    result.push_back(input);
    result.back().witness = std::move(witness.value());
  }
  return base::ok(std::move(result));
}

base::expected<std::optional<uint32_t>, OrchardStorage::Error>
OrchardSyncState::GetLatestShardIndex(const mojom::AccountIdPtr& account_id) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return storage_.GetLatestShardIndex(account_id);
}

base::expected<std::optional<uint32_t>, OrchardStorage::Error>
OrchardSyncState::GetMinCheckpointId(const mojom::AccountIdPtr& account_id) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return storage_.MinCheckpointId(account_id);
}

void OrchardSyncState::ResetDatabase() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  storage_.ResetDatabase();
}

void OrchardSyncState::OverrideShardTreeForTesting(  // IN_TEST
    const mojom::AccountIdPtr& account_id,
    std::unique_ptr<orchard::OrchardShardTree> shard_tree) {
  CHECK_IS_TEST();
  shard_trees_[account_id->unique_key] = std::move(shard_tree);
}

OrchardStorage& OrchardSyncState::orchard_storage() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return storage_;
}

}  // namespace brave_wallet
