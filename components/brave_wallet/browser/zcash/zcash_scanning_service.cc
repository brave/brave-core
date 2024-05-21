/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/zcash/zcash_scanning_service.h"

#include "base/task/thread_pool.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "brave/components/brave_wallet/common/hex_utils.h"

namespace brave_wallet {

namespace {
constexpr size_t kNullifierSize = 32;
constexpr size_t kChainReorgBlockDelta = 150;
}  // namespace

ZCashBlockScannerResult::ZCashBlockScannerResult(
    std::vector<OrchardNote> discovered_notes,
    std::vector<OrchardNullifier> spent_notes)
    : discovered_notes(std::move(discovered_notes)),
      spent_notes(std::move(spent_notes)) {}

ZCashBlockScannerResult::ZCashBlockScannerResult(
    const ZCashBlockScannerResult&) = default;

ZCashBlockScannerResult& ZCashBlockScannerResult::operator=(
    const ZCashBlockScannerResult&) = default;

ZCashBlockScannerResult::~ZCashBlockScannerResult() = default;

ZCashBlockScanner::ZCashBlockScanner(
    const std::array<uint8_t, kOrchardFullViewKeySize>& full_view_key)
    : full_view_key_(full_view_key) {}

ZCashBlockScanner::~ZCashBlockScanner() = default;

base::expected<ZCashBlockScannerResult, std::string>
ZCashBlockScanner::ParseBlocks(std::vector<OrchardNote> known_notes,
                               std::vector<mojom::CompactBlockPtr> blocks) {
  std::vector<OrchardNullifier> found_nullifiers;
  std::vector<OrchardNote> found_notes;

  for (const auto& block : blocks) {
    for (const auto& tx : block->vtx) {
      ::rust::Vec<orchard::OrchardCompactAction> orchard_actions;
      for (const auto& orchard_action : tx->orchard_actions) {
        // If orchard actions spends known note then remember note to delete.
        std::array<uint8_t, kNullifierSize> action_nullifier;
        std::copy(orchard_action->nullifier.begin(),
                  orchard_action->nullifier.end(), action_nullifier.begin());
        if (std::find_if(known_notes.begin(), known_notes.end(),
                         [&action_nullifier](const auto& v) {
                           return v.nullifier == action_nullifier;
                         }) != known_notes.end()) {
          OrchardNullifier nullifier;
          nullifier.block_id = block->height;
          nullifier.nullifier = action_nullifier;
          found_nullifiers.push_back(std::move(nullifier));
        }
        orchard::OrchardCompactAction orchard_compact_action;
        std::copy(orchard_action->nullifier.begin(),
                  orchard_action->nullifier.end(),
                  orchard_compact_action.nullifier.begin());
        std::copy(orchard_action->cmx.begin(), orchard_action->cmx.end(),
                  orchard_compact_action.cmx.begin());
        std::copy(orchard_action->ephemeral_key.begin(),
                  orchard_action->ephemeral_key.end(),
                  orchard_compact_action.ephemeral_key.begin());
        std::copy(orchard_action->ciphertext.begin(),
                  orchard_action->ciphertext.end(),
                  orchard_compact_action.enc_cipher_text.begin());
        orchard_actions.push_back(std::move(orchard_compact_action));
      }
      ::rust::Box<::brave_wallet::orchard::BatchOrchardDecodeBundleResult>
          result = ::brave_wallet::orchard::batch_decode(
              full_view_key_, std::move(orchard_actions));
      if (result->is_ok()) {
        ::rust::Box<::brave_wallet::orchard::BatchOrchardDecodeBundle>
            result_bundle = result->unwrap();
        for (size_t i = 0; i < result_bundle->size(); i++) {
          found_notes.push_back(OrchardNote(
              {block->height, result_bundle->note_nullifier(full_view_key_, i),
               result_bundle->note_value(i)}));
        }
      }
    }
  }
  return ZCashBlockScannerResult(std::move(found_notes),
                                 std::move(found_nullifiers));
}

ZCashScanService::ZCashScanService(
    ZCashRpc* zcash_rpc,
    const mojom::AccountIdPtr& account_id,
    const mojom::ZCashAccountBirthdayPtr& account_birthday,
    const std::array<uint8_t, kOrchardFullViewKeySize>& fvk,
    base::FilePath db_dir_path)
    : zcash_rpc_(zcash_rpc),
      account_id_(account_id.Clone()),
      account_birthday_(account_birthday.Clone()),
      full_view_key_(fvk),
      db_dir_path_(db_dir_path) {
  chain_id_ = GetNetworkForZCashKeyring(account_id->keyring_id);
}

ZCashScanService::~ZCashScanService() = default;

void ZCashScanService::UpdateAccountMeta() {
  if (!background_orchard_storage_) {
    background_orchard_storage_.emplace(
        base::ThreadPool::CreateSequencedTaskRunner({base::MayBlock()}),
        db_dir_path_);
  }
  InitAccount();
}

void ZCashScanService::StartSyncing(
    mojo::PendingRemote<mojom::ZCashSyncObserver> observer) {
  observer_.Bind(std::move(observer));
  Iterate();
}

void ZCashScanService::PauseSyncing() {
  stopped_ = true;
  observer_.reset();
}

mojom::ZCashSyncStatusPtr ZCashScanService::GetSyncStatus() {
  return current_sync_status_.Clone();
}

void ZCashScanService::Iterate() {
  if (stopped_) {
    return;
  }

  if (error_) {
    observer_->OnError(error_.value());
    return;
  }

  if (!background_block_scanner_) {
    background_block_scanner_.emplace(
        base::ThreadPool::CreateSequencedTaskRunner({base::MayBlock()}),
        full_view_key_);
  }

  if (!background_orchard_storage_) {
    background_orchard_storage_.emplace(
        base::ThreadPool::CreateSequencedTaskRunner({base::MayBlock()}),
        db_dir_path_);
  }

  if (!next_block_to_scan_) {
    GetAccountMeta();
    return;
  }

  if (!latest_block_) {
    UpdateChainTip();
    return;
  }

  if (!spendable_notes_) {
    UpdateSpendableNotes();
    return;
  }

  if (observer_) {
    observer_->OnUpdateSyncStatus(current_sync_status_.Clone());
  }

  if (next_block_to_scan_ < latest_block_) {
    DownloadBlocks();
    return;
  }

  if (observer_) {
    observer_->OnStop();
  }
}

void ZCashScanService::GetAccountMeta() {
  background_orchard_storage_->AsyncCall(&OrchardStorage::GetAccountMeta)
      .WithArgs(account_id_.Clone())
      .Then(base::BindOnce(&ZCashScanService::OnGetAccountMeta,
                           weak_ptr_factory_.GetWeakPtr()));
}

void ZCashScanService::OnGetAccountMeta(
    base::expected<AccountMeta, OrchardStorage::OrchardStorageError> result) {
  if (!result.has_value()) {
    if (result.error().error_code ==
        OrchardStorage::OrchardStorageErrorCode::kAccountNotFound) {
      InitAccount();
    } else {
      error_ = result.error().message;
      Iterate();
    }
    return;
  }
  next_block_to_scan_ = result->latest_scanned_block_id;
  Iterate();
}

void ZCashScanService::InitAccount() {
  background_orchard_storage_->AsyncCall(&OrchardStorage::RegisterAccount)
      .WithArgs(account_id_.Clone(), account_birthday_->value,
                account_birthday_->hash)
      .Then(base::BindOnce(&ZCashScanService::OnAccountInit,
                           weak_ptr_factory_.GetWeakPtr()));
}

void ZCashScanService::OnAccountInit(
    std::optional<OrchardStorage::OrchardStorageError> error) {
  if (error) {
    error_ = error->message;
    Iterate();
  } else {
    GetAccountMeta();
  }
}

void ZCashScanService::VerifiyChainState(AccountMeta account_meta) {
  auto block_id = mojom::BlockID::New(account_meta.latest_scanned_block_id,
                                      std::vector<uint8_t>());
  zcash_rpc_->GetTreeState(
      chain_id_, std::move(block_id),
      base::BindOnce(&ZCashScanService::OnGetTreeStateForChainVerification,
                     weak_ptr_factory_.GetWeakPtr(), std::move(account_meta)));
}

void ZCashScanService::OnGetTreeStateForChainVerification(
    AccountMeta account_meta,
    base::expected<mojom::TreeStatePtr, std::string> tree_state) {
  if (!tree_state.has_value()) {
    error_ = "Failed to receive tree state " + tree_state.error();
    Iterate();
  }

  if (ToHex(tree_state.value()->hash) !=
      account_meta.latest_scanned_block_hash) {
    latest_block_.reset();
    spendable_notes_.reset();
    GetTreeStateForChainReorg(account_meta.latest_scanned_block_id -
                              kChainReorgBlockDelta);
  }
}

void ZCashScanService::GetTreeStateForChainReorg(uint64_t new_block_id) {
  auto block_id = mojom::BlockID::New(new_block_id, std::vector<uint8_t>());
  zcash_rpc_->GetTreeState(
      chain_id_, std::move(block_id),
      base::BindOnce(&ZCashScanService::OnGetTreeStateForChainReorg,
                     weak_ptr_factory_.GetWeakPtr()));
}

void ZCashScanService::OnGetTreeStateForChainReorg(
    base::expected<mojom::TreeStatePtr, std::string> tree_state) {
  if (!tree_state.has_value() || !tree_state.value()) {
    error_ = "Failed to receive tree state " + tree_state.error();
    Iterate();
  }
  background_orchard_storage_->AsyncCall(&OrchardStorage::HandleChainReorg)
      .WithArgs(account_id_.Clone(), (*tree_state)->height, (*tree_state)->hash)
      .Then(base::BindOnce(&ZCashScanService::OnDatabaseUpdatedForChainReorg,
                           weak_ptr_factory_.GetWeakPtr()));
}

void ZCashScanService::OnDatabaseUpdatedForChainReorg(
    std::optional<OrchardStorage::OrchardStorageError> error) {
  if (error) {
    error_ = "Failed to update database " + error->message;
    Iterate();
  } else {
    UpdateSpendableNotes();
  }
}

void ZCashScanService::UpdateSpendableNotes() {
  spendable_notes_ = std::nullopt;
  background_orchard_storage_->AsyncCall(&OrchardStorage::GetSpendableNotes)
      .WithArgs(account_id_.Clone())
      .Then(base::BindOnce(&ZCashScanService::OnGetSpendableNotes,
                           weak_ptr_factory_.GetWeakPtr()));
}

void ZCashScanService::OnGetSpendableNotes(
    base::expected<std::vector<OrchardNote>,
                   OrchardStorage::OrchardStorageError> result) {
  if (!result.has_value()) {
    error_ = "Cannot fetch spendable notes: " + result.error().message;
  } else {
    spendable_notes_ = result.value();
    current_sync_status_ = mojom::ZCashSyncStatus::New(
        next_block_to_scan_.value(), latest_block_.value(),
        spendable_notes_->size(), GetSpendableBalance());
  }

  Iterate();
}

void ZCashScanService::UpdateChainTip() {
  zcash_rpc_->GetLatestBlock(chain_id_,
                             base::BindOnce(&ZCashScanService::OnGetLatestBlock,
                                            weak_ptr_factory_.GetWeakPtr()));
}

void ZCashScanService::OnGetLatestBlock(
    base::expected<mojom::BlockIDPtr, std::string> result) {
  if (!result.has_value()) {
    error_ = "Failed to get latest block";
  } else {
    latest_block_ = (*result)->height;
  }

  Iterate();
}

void ZCashScanService::DownloadBlocks() {
  zcash_rpc_->GetCompactBlocks(
      chain_id_, *next_block_to_scan_,
      std::min(*latest_block_, *next_block_to_scan_ + kScanBatchSize),
      base::BindOnce(&ZCashScanService::OnBlocksDownloaded,
                     weak_ptr_factory_.GetWeakPtr()));
}

void ZCashScanService::OnBlocksDownloaded(
    base::expected<std::vector<mojom::CompactBlockPtr>, std::string> result) {
  if (!result.has_value()) {
    error_ = "Failed to download blocks " + result.error();
    Iterate();
  } else {
    ScanBlocks(std::move(result.value()));
    next_block_to_scan_ = *next_block_to_scan_ + result->size();
  }
}

void ZCashScanService::ScanBlocks(std::vector<mojom::CompactBlockPtr> blocks) {
  if (blocks.empty()) {
    error_ = "No blocks to scan";
    Iterate();
    return;
  }

  auto blocks_count = blocks.size();
  auto last_block_hash = ToHex(blocks[blocks.size() - 1]->hash);

  background_block_scanner_->AsyncCall(&ZCashBlockScanner::ParseBlocks)
      .WithArgs(*spendable_notes_, std::move(blocks))
      .Then(base::BindOnce(&ZCashScanService::OnBlocksScanned,
                           weak_ptr_factory_.GetWeakPtr(), blocks_count,
                           last_block_hash));
}

void ZCashScanService::OnBlocksScanned(
    uint64_t blocks_count,
    std::string last_block_hash,
    base::expected<ZCashBlockScannerResult, std::string> result) {
  if (!result.has_value()) {
    error_ = result.error();
    Iterate();
  } else {
    UpdateNotes(result->discovered_notes, result->spent_notes,
                next_block_to_scan_.value() + blocks_count, last_block_hash);
  }
}

void ZCashScanService::UpdateNotes(
    const std::vector<OrchardNote>& found_notes,
    const std::vector<OrchardNullifier>& notes_to_delete,
    uint64_t latest_scanned_block,
    std::string latest_scanned_block_hash) {
  background_orchard_storage_->AsyncCall(&OrchardStorage::UpdateNotes)
      .WithArgs(account_id_.Clone(), found_notes, notes_to_delete,
                latest_scanned_block, latest_scanned_block_hash)
      .Then(base::BindOnce(&ZCashScanService::UpdateNotesComplete,
                           weak_ptr_factory_.GetWeakPtr(),
                           latest_scanned_block));
}

void ZCashScanService::UpdateNotesComplete(
    uint64_t new_next_block_to_scan,
    std::optional<OrchardStorage::OrchardStorageError> error) {
  if (error) {
    error_ = "Can't update notes DB";
    Iterate();
  } else {
    next_block_to_scan_ = new_next_block_to_scan;
    UpdateSpendableNotes();
  }
}

uint64_t ZCashScanService::GetSpendableBalance() {
  CHECK(spendable_notes_.has_value());
  uint64_t balance = 0;
  for (const auto& note : spendable_notes_.value()) {
    balance += note.amount;
  }
  return balance;
}

}  // namespace brave_wallet
