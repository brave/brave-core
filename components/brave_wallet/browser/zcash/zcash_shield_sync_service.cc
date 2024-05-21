/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/zcash/zcash_shield_sync_service.h"

#include "base/task/thread_pool.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "brave/components/brave_wallet/common/zcash_utils.h"

namespace brave_wallet {

ZCashShieldSyncService::OrchardBlockScannerProxy::OrchardBlockScannerProxy(
    std::array<uint8_t, kOrchardFullViewKeySize> full_view_key) {
  background_block_scanner_.emplace(
      base::ThreadPool::CreateSequencedTaskRunner({base::MayBlock()}),
      full_view_key);
}

ZCashShieldSyncService::OrchardBlockScannerProxy::~OrchardBlockScannerProxy() =
    default;

void ZCashShieldSyncService::OrchardBlockScannerProxy::ScanBlocks(
    std::vector<OrchardNote> known_notes,
    std::vector<zcash::mojom::CompactBlockPtr> blocks,
    base::OnceCallback<void(base::expected<OrchardBlockScanner::Result,
                                           OrchardBlockScanner::ErrorCode>)>
        callback) {
  background_block_scanner_.AsyncCall(&OrchardBlockScanner::ScanBlocks)
      .WithArgs(std::move(known_notes), std::move(blocks))
      .Then(std::move(callback));
}

ZCashShieldSyncService::ZCashShieldSyncService(
    ZCashRpc* zcash_rpc,
    const mojom::AccountIdPtr& account_id,
    const mojom::ZCashAccountShieldBirthdayPtr& account_birthday,
    const std::array<uint8_t, kOrchardFullViewKeySize>& fvk,
    base::FilePath db_dir_path)
    : zcash_rpc_(zcash_rpc),
      account_id_(account_id.Clone()),
      account_birthday_(account_birthday.Clone()),
      db_dir_path_(db_dir_path) {
  chain_id_ = GetNetworkForZCashKeyring(account_id->keyring_id);
  block_scanner_ = std::make_unique<OrchardBlockScannerProxy>(fvk);
  background_orchard_storage_.emplace(
      base::ThreadPool::CreateSequencedTaskRunner({base::MayBlock()}),
      db_dir_path_);
}

ZCashShieldSyncService::~ZCashShieldSyncService() = default;

void ZCashShieldSyncService::SetOrchardBlockScannerProxyForTesting(
    std::unique_ptr<OrchardBlockScannerProxy> block_scanner) {
  block_scanner_ = std::move(block_scanner);
}

void ZCashShieldSyncService::StartSyncing(
    mojo::PendingRemote<mojom::ZCashSyncObserver> observer) {
  if (observer_.is_bound()) {
    NOTREACHED_IN_MIGRATION();
    return;
  }
  observer_.Bind(std::move(observer));
  ScheduleWorkOnTask();
}

void ZCashShieldSyncService::PauseSyncing() {
  stopped_ = true;
  observer_->OnStop();
}

mojom::ZCashShieldSyncStatusPtr ZCashShieldSyncService::GetSyncStatus() {
  return current_sync_status_.Clone();
}

void ZCashShieldSyncService::ScheduleWorkOnTask() {
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(&ZCashShieldSyncService::WorkOnTask,
                                weak_ptr_factory_.GetWeakPtr()));
}

void ZCashShieldSyncService::WorkOnTask() {
  if (stopped_) {
    return;
  }

  if (error_) {
    if (observer_) {
      observer_->OnError(base::NumberToString(error_->code) + ": " +
                         error_->message);
    }
    return;
  }

  if (!chain_tip_block_) {
    UpdateChainTip();
    return;
  }

  if (!latest_scanned_block_) {
    GetAccountMeta();
    return;
  }

  if (!spendable_notes_) {
    UpdateSpendableNotes();
    return;
  }

  if (observer_) {
    observer_->OnUpdateSyncStatus(current_sync_status_.Clone());
  }

  if (*latest_scanned_block_ < *chain_tip_block_) {
    DownloadBlocks();
    return;
  }

  if (observer_) {
    observer_->OnStop();
    stopped_ = true;
  }
}

void ZCashShieldSyncService::GetAccountMeta() {
  background_orchard_storage_.AsyncCall(&ZCashOrchardStorage::GetAccountMeta)
      .WithArgs(account_id_.Clone())
      .Then(base::BindOnce(&ZCashShieldSyncService::OnGetAccountMeta,
                           weak_ptr_factory_.GetWeakPtr()));
}

void ZCashShieldSyncService::OnGetAccountMeta(
    base::expected<ZCashOrchardStorage::AccountMeta, ZCashOrchardStorage::Error>
        result) {
  if (!result.has_value()) {
    if (result.error().error_code ==
        ZCashOrchardStorage::ErrorCode::kAccountNotFound) {
      InitAccount();
    } else {
      error_ = Error{kFailedToRetrieveAccount, result.error().message};
      ScheduleWorkOnTask();
    }
    return;
  }

  VerifiyChainState(*result);
}

void ZCashShieldSyncService::InitAccount() {
  background_orchard_storage_.AsyncCall(&ZCashOrchardStorage::RegisterAccount)
      .WithArgs(account_id_.Clone(), account_birthday_->value,
                account_birthday_->hash)
      .Then(base::BindOnce(&ZCashShieldSyncService::OnAccountInit,
                           weak_ptr_factory_.GetWeakPtr()));
}

void ZCashShieldSyncService::OnAccountInit(
    std::optional<ZCashOrchardStorage::Error> error) {
  if (error) {
    error_ = Error{kFailedToInitAccount, error->message};
    ScheduleWorkOnTask();
  } else {
    GetAccountMeta();
  }
}

void ZCashShieldSyncService::VerifiyChainState(
    ZCashOrchardStorage::AccountMeta account_meta) {
  // If block chain has removed blocks we already scanned then we need to handle
  // chain reorg.
  if (*chain_tip_block_ < account_meta.latest_scanned_block_id) {
    // Assume that chain reorg can't affect more than kChainReorgBlockDelta
    // blocks So we can just fallback on this number from the chain tip block.
    GetTreeStateForChainReorg(*chain_tip_block_ - kChainReorgBlockDelta);
    return;
  }
  // Retrieve block info for last scanned block id to check whether block hash
  // is the same
  auto block_id = zcash::mojom::BlockID::New(
      account_meta.latest_scanned_block_id, std::vector<uint8_t>());
  zcash_rpc_->GetTreeState(
      chain_id_, std::move(block_id),
      base::BindOnce(
          &ZCashShieldSyncService::OnGetTreeStateForChainVerification,
          weak_ptr_factory_.GetWeakPtr(), std::move(account_meta)));
}

void ZCashShieldSyncService::OnGetTreeStateForChainVerification(
    ZCashOrchardStorage::AccountMeta account_meta,
    base::expected<zcash::mojom::TreeStatePtr, std::string> tree_state) {
  if (!tree_state.has_value() || !tree_state.value()) {
    error_ = Error{kFailedToReceiveTreeState, tree_state.error()};
    ScheduleWorkOnTask();
  } else {
    if (tree_state.value()->hash != account_meta.latest_scanned_block_hash) {
      // Assume that chain reorg can't affect more than kChainReorgBlockDelta
      // blocks So we can just fallback on this number.
      GetTreeStateForChainReorg(account_meta.latest_scanned_block_id -
                                kChainReorgBlockDelta);
      return;
    } else {
      // Restore latest scanned block from the database so we can continue
      // scanning from previous point.
      latest_scanned_block_ = account_meta.latest_scanned_block_id;
      ScheduleWorkOnTask();
    }
  }
}

void ZCashShieldSyncService::GetTreeStateForChainReorg(
    uint32_t new_block_height) {
  // Query block info by block height
  auto block_id =
      zcash::mojom::BlockID::New(new_block_height, std::vector<uint8_t>());
  zcash_rpc_->GetTreeState(
      chain_id_, std::move(block_id),
      base::BindOnce(&ZCashShieldSyncService::OnGetTreeStateForChainReorg,
                     weak_ptr_factory_.GetWeakPtr(), new_block_height));
}

void ZCashShieldSyncService::OnGetTreeStateForChainReorg(
    uint32_t new_block_height,
    base::expected<zcash::mojom::TreeStatePtr, std::string> tree_state) {
  if (!tree_state.has_value() || !tree_state.value() ||
      new_block_height != (*tree_state)->height) {
    error_ = Error{kFailedToReceiveTreeState, tree_state.error()};
    ScheduleWorkOnTask();
    return;
  } else {
    // Reorg database so records related to removed blocks are wiped out
    background_orchard_storage_
        .AsyncCall(&ZCashOrchardStorage::HandleChainReorg)
        .WithArgs(account_id_.Clone(), (*tree_state)->height,
                  (*tree_state)->hash)
        .Then(base::BindOnce(
            &ZCashShieldSyncService::OnDatabaseUpdatedForChainReorg,
            weak_ptr_factory_.GetWeakPtr(), (*tree_state)->height));
  }
}

void ZCashShieldSyncService::OnDatabaseUpdatedForChainReorg(
    uint32_t new_block_height,
    std::optional<ZCashOrchardStorage::Error> error) {
  if (error) {
    error_ = Error{kFailedToUpdateDatabase, error->message};
    ScheduleWorkOnTask();
  } else {
    latest_scanned_block_ = new_block_height;
    // Reload list of discovered spendable notes and update account info
    UpdateSpendableNotes();
  }
}

void ZCashShieldSyncService::UpdateSpendableNotes() {
  spendable_notes_ = std::nullopt;
  background_orchard_storage_.AsyncCall(&ZCashOrchardStorage::GetSpendableNotes)
      .WithArgs(account_id_.Clone())
      .Then(base::BindOnce(&ZCashShieldSyncService::OnGetSpendableNotes,
                           weak_ptr_factory_.GetWeakPtr()));
}

void ZCashShieldSyncService::OnGetSpendableNotes(
    base::expected<std::vector<OrchardNote>, ZCashOrchardStorage::Error>
        result) {
  if (!result.has_value()) {
    error_ = Error{kFailedToRetrieveSpendableNotes, result.error().message};
  } else {
    spendable_notes_ = result.value();
    current_sync_status_ = mojom::ZCashShieldSyncStatus::New(
        latest_scanned_block_.value(), chain_tip_block_.value(),
        spendable_notes_->size(), GetSpendableBalance());
  }

  ScheduleWorkOnTask();
}

void ZCashShieldSyncService::UpdateChainTip() {
  zcash_rpc_->GetLatestBlock(
      chain_id_, base::BindOnce(&ZCashShieldSyncService::OnGetLatestBlock,
                                weak_ptr_factory_.GetWeakPtr()));
}

void ZCashShieldSyncService::OnGetLatestBlock(
    base::expected<zcash::mojom::BlockIDPtr, std::string> result) {
  if (!result.has_value()) {
    error_ = Error{kFailedToUpdateChainTip, result.error()};
  } else {
    chain_tip_block_ = (*result)->height;
  }

  ScheduleWorkOnTask();
}

void ZCashShieldSyncService::DownloadBlocks() {
  zcash_rpc_->GetCompactBlocks(
      chain_id_, *latest_scanned_block_ + 1,
      std::min(*latest_scanned_block_ + 1,
               *latest_scanned_block_ + kScanBatchSize),
      base::BindOnce(&ZCashShieldSyncService::OnBlocksDownloaded,
                     weak_ptr_factory_.GetWeakPtr()));
}

void ZCashShieldSyncService::OnBlocksDownloaded(
    base::expected<std::vector<zcash::mojom::CompactBlockPtr>, std::string>
        result) {
  if (!result.has_value()) {
    error_ = Error{ErrorCode::kFailedToDownloadBlocks, result.error()};
    ScheduleWorkOnTask();
  } else {
    ScanBlocks(std::move(result.value()));
  }
}

void ZCashShieldSyncService::ScanBlocks(
    std::vector<zcash::mojom::CompactBlockPtr> blocks) {
  if (blocks.empty()) {
    error_ = Error{kScannerError, ""};
    ScheduleWorkOnTask();
    return;
  }

  auto blocks_count = blocks.size();
  auto last_block_hash = ToHex(blocks[blocks.size() - 1]->hash);

  block_scanner_->ScanBlocks(
      *spendable_notes_, std::move(blocks),
      base::BindOnce(&ZCashShieldSyncService::OnBlocksScanned,
                     weak_ptr_factory_.GetWeakPtr(), blocks_count,
                     last_block_hash));
}

void ZCashShieldSyncService::OnBlocksScanned(
    uint32_t blocks_count,
    std::string last_block_hash,
    base::expected<OrchardBlockScanner::Result, OrchardBlockScanner::ErrorCode>
        result) {
  if (!result.has_value()) {
    error_ =
        Error{ErrorCode::kScannerError, base::NumberToString(result.error())};
    ScheduleWorkOnTask();
  } else {
    UpdateNotes(result->discovered_notes, result->spent_notes,
                latest_scanned_block_.value() + blocks_count, last_block_hash);
  }
}

void ZCashShieldSyncService::UpdateNotes(
    const std::vector<OrchardNote>& found_notes,
    const std::vector<OrchardNullifier>& notes_to_delete,
    uint32_t latest_scanned_block,
    std::string latest_scanned_block_hash) {
  background_orchard_storage_.AsyncCall(&ZCashOrchardStorage::UpdateNotes)
      .WithArgs(account_id_.Clone(), found_notes, notes_to_delete,
                latest_scanned_block, latest_scanned_block_hash)
      .Then(base::BindOnce(&ZCashShieldSyncService::UpdateNotesComplete,
                           weak_ptr_factory_.GetWeakPtr(),
                           latest_scanned_block));
}

void ZCashShieldSyncService::UpdateNotesComplete(
    uint32_t new_latest_scanned_block,
    std::optional<ZCashOrchardStorage::Error> error) {
  if (error) {
    error_ = Error{kFailedToUpdateDatabase, error->message};
    ScheduleWorkOnTask();
  } else {
    latest_scanned_block_ = new_latest_scanned_block;
    UpdateSpendableNotes();
  }
}

uint32_t ZCashShieldSyncService::GetSpendableBalance() {
  CHECK(spendable_notes_.has_value());
  uint32_t balance = 0;
  for (const auto& note : spendable_notes_.value()) {
    balance += note.amount;
  }
  return balance;
}

}  // namespace brave_wallet
