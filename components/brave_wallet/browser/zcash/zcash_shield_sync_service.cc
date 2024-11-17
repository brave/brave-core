/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/zcash/zcash_shield_sync_service.h"

#include <algorithm>
#include <utility>

#include "base/task/thread_pool.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_wallet_service.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "brave/components/brave_wallet/common/zcash_utils.h"

namespace brave_wallet {

namespace {

size_t GetCode(ZCashShieldSyncService::ErrorCode error) {
  switch (error) {
    case ZCashShieldSyncService::ErrorCode::kFailedToDownloadBlocks:
      return 0;
    case ZCashShieldSyncService::ErrorCode::kFailedToUpdateDatabase:
      return 1;
    case ZCashShieldSyncService::ErrorCode::kFailedToUpdateChainTip:
      return 2;
    case ZCashShieldSyncService::ErrorCode::kFailedToRetrieveSpendableNotes:
      return 3;
    case ZCashShieldSyncService::ErrorCode::kFailedToReceiveTreeState:
      return 4;
    case ZCashShieldSyncService::ErrorCode::kFailedToInitAccount:
      return 5;
    case ZCashShieldSyncService::ErrorCode::kFailedToRetrieveAccount:
      return 6;
    case ZCashShieldSyncService::ErrorCode::kScannerError:
      return 7;
  }
}

}  // namespace

ZCashShieldSyncService::OrchardBlockScannerProxy::OrchardBlockScannerProxy(
    OrchardFullViewKey full_view_key) {
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
    ZCashWalletService* zcash_wallet_service,
    const mojom::AccountIdPtr& account_id,
    const mojom::ZCashAccountShieldBirthdayPtr& account_birthday,
    const OrchardFullViewKey& fvk,
    base::WeakPtr<Observer> observer)
    : zcash_wallet_service_(zcash_wallet_service),
      account_id_(account_id.Clone()),
      account_birthday_(account_birthday.Clone()),
      observer_(std::move(observer)) {
  chain_id_ = GetNetworkForZCashKeyring(account_id->keyring_id);
  block_scanner_ = std::make_unique<OrchardBlockScannerProxy>(fvk);
}

ZCashShieldSyncService::~ZCashShieldSyncService() = default;

void ZCashShieldSyncService::SetOrchardBlockScannerProxyForTesting(
    std::unique_ptr<OrchardBlockScannerProxy> block_scanner) {
  block_scanner_ = std::move(block_scanner);
}

void ZCashShieldSyncService::StartSyncing() {
  ScheduleWorkOnTask();
  if (observer_) {
    observer_->OnSyncStart(account_id_);
  }
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
      observer_->OnSyncError(
          account_id_,
          base::NumberToString(GetCode(error_->code)) + ": " + error_->message);
    }
    return;
  }

  if (!chain_tip_block_) {
    UpdateChainTip();
    return;
  }

  if (!account_meta_) {
    GetOrCreateAccount();
    return;
  }

  if (!latest_scanned_block_) {
    VerifyChainState(*account_meta_);
    return;
  }

  if (!spendable_notes_) {
    UpdateSpendableNotes();
    return;
  }

  if (observer_) {
    observer_->OnSyncStatusUpdate(account_id_, current_sync_status_.Clone());
  }

  if (!downloaded_blocks_ && *latest_scanned_block_ < *chain_tip_block_) {
    DownloadBlocks();
    return;
  }

  if (downloaded_blocks_) {
    ScanBlocks();
    return;
  }

  if (observer_) {
    observer_->OnSyncStop(account_id_);
    stopped_ = true;
  }
}

void ZCashShieldSyncService::GetOrCreateAccount() {
  if (account_birthday_->value < kNu5BlockUpdate) {
    error_ =
        Error{ErrorCode::kFailedToInitAccount, "Wrong birthday block height"};
    ScheduleWorkOnTask();
    return;
  }
  orchard_storage()
      .AsyncCall(&ZCashOrchardStorage::GetAccountMeta)
      .WithArgs(account_id_.Clone())
      .Then(base::BindOnce(&ZCashShieldSyncService::OnGetAccountMeta,
                           weak_ptr_factory_.GetWeakPtr()));
}

void ZCashShieldSyncService::OnGetAccountMeta(
    base::expected<ZCashOrchardStorage::AccountMeta, ZCashOrchardStorage::Error>
        result) {
  if (result.has_value()) {
    account_meta_ = *result;
    if (account_meta_->latest_scanned_block_id.value() &&
        (account_meta_->latest_scanned_block_id.value() <
         account_meta_->account_birthday)) {
      error_ = Error{ErrorCode::kFailedToRetrieveAccount, ""};
    }
    ScheduleWorkOnTask();
    return;
  }

  if (result.error().error_code ==
      ZCashOrchardStorage::ErrorCode::kAccountNotFound) {
    InitAccount();
    return;
  } else {
    error_ = Error{ErrorCode::kFailedToRetrieveAccount, result.error().message};
  }
  ScheduleWorkOnTask();
}

void ZCashShieldSyncService::InitAccount() {
  orchard_storage()
      .AsyncCall(&ZCashOrchardStorage::RegisterAccount)
      .WithArgs(account_id_.Clone(), account_birthday_->value)
      .Then(base::BindOnce(&ZCashShieldSyncService::OnAccountInit,
                           weak_ptr_factory_.GetWeakPtr()));
}

void ZCashShieldSyncService::OnAccountInit(
    base::expected<ZCashOrchardStorage::AccountMeta, ZCashOrchardStorage::Error>
        result) {
  if (!result.has_value()) {
    error_ = Error{ErrorCode::kFailedToInitAccount, result.error().message};
  } else {
    account_meta_ = *result;
  }
  ScheduleWorkOnTask();
}

void ZCashShieldSyncService::VerifyChainState(
    ZCashOrchardStorage::AccountMeta account_meta) {
  if (account_meta.account_birthday < kNu5BlockUpdate) {
    error_ = Error{ErrorCode::kFailedToRetrieveAccount,
                   "Wrong birthday block height"};
    ScheduleWorkOnTask();
    return;
  }
  if (!account_meta.latest_scanned_block_id) {
    latest_scanned_block_ = account_meta.account_birthday - 1;
    ScheduleWorkOnTask();
    return;
  }
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
      account_meta.latest_scanned_block_id.value(), std::vector<uint8_t>());
  zcash_rpc().GetTreeState(
      chain_id_, std::move(block_id),
      base::BindOnce(
          &ZCashShieldSyncService::OnGetTreeStateForChainVerification,
          weak_ptr_factory_.GetWeakPtr(), std::move(account_meta)));
}

void ZCashShieldSyncService::OnGetTreeStateForChainVerification(
    ZCashOrchardStorage::AccountMeta account_meta,
    base::expected<zcash::mojom::TreeStatePtr, std::string> tree_state) {
  if (!tree_state.has_value() || !tree_state.value()) {
    error_ = Error{ErrorCode::kFailedToReceiveTreeState, tree_state.error()};
    ScheduleWorkOnTask();
    return;
  }
  auto backend_block_hash = RevertHex(tree_state.value()->hash);
  if (backend_block_hash != account_meta.latest_scanned_block_hash.value()) {
    // Assume that chain reorg can't affect more than kChainReorgBlockDelta
    // blocks So we can just fallback on this number.
    uint32_t new_block_id =
        account_meta.latest_scanned_block_id.value() > kChainReorgBlockDelta
            ? account_meta.latest_scanned_block_id.value() -
                  kChainReorgBlockDelta
            : 0;
    GetTreeStateForChainReorg(new_block_id);
    return;
  }

  // Restore latest scanned block from the database so we can continue
  // scanning from previous point.
  latest_scanned_block_ = account_meta.latest_scanned_block_id;
  ScheduleWorkOnTask();
}

void ZCashShieldSyncService::GetTreeStateForChainReorg(
    uint32_t new_block_height) {
  // Query block info by block height
  auto block_id =
      zcash::mojom::BlockID::New(new_block_height, std::vector<uint8_t>());
  zcash_rpc().GetTreeState(
      chain_id_, std::move(block_id),
      base::BindOnce(&ZCashShieldSyncService::OnGetTreeStateForChainReorg,
                     weak_ptr_factory_.GetWeakPtr(), new_block_height));
}

void ZCashShieldSyncService::OnGetTreeStateForChainReorg(
    uint32_t new_block_height,
    base::expected<zcash::mojom::TreeStatePtr, std::string> tree_state) {
  if (!tree_state.has_value() || !tree_state.value() ||
      new_block_height != (*tree_state)->height) {
    error_ = Error{ErrorCode::kFailedToReceiveTreeState, tree_state.error()};
    ScheduleWorkOnTask();
    return;
  } else {
    // Reorg database so records related to removed blocks are wiped out
    orchard_storage()
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
    error_ = Error{ErrorCode::kFailedToUpdateDatabase, error->message};
    ScheduleWorkOnTask();
    return;
  }

  latest_scanned_block_ = new_block_height;
  ScheduleWorkOnTask();
}

void ZCashShieldSyncService::UpdateSpendableNotes() {
  orchard_storage()
      .AsyncCall(&ZCashOrchardStorage::GetSpendableNotes)
      .WithArgs(account_id_.Clone())
      .Then(base::BindOnce(&ZCashShieldSyncService::OnGetSpendableNotes,
                           weak_ptr_factory_.GetWeakPtr()));
}

void ZCashShieldSyncService::OnGetSpendableNotes(
    base::expected<std::vector<OrchardNote>, ZCashOrchardStorage::Error>
        result) {
  if (!result.has_value()) {
    error_ = Error{ErrorCode::kFailedToRetrieveSpendableNotes,
                   result.error().message};
  } else {
    spendable_notes_ = result.value();
    current_sync_status_ = mojom::ZCashShieldSyncStatus::New(
        latest_scanned_block_.value(), chain_tip_block_.value(),
        spendable_notes_->size(), GetSpendableBalance());
  }
  ScheduleWorkOnTask();
}

void ZCashShieldSyncService::UpdateChainTip() {
  zcash_rpc().GetLatestBlock(
      chain_id_, base::BindOnce(&ZCashShieldSyncService::OnGetLatestBlock,
                                weak_ptr_factory_.GetWeakPtr()));
}

void ZCashShieldSyncService::OnGetLatestBlock(
    base::expected<zcash::mojom::BlockIDPtr, std::string> result) {
  if (!result.has_value()) {
    error_ = Error{ErrorCode::kFailedToUpdateChainTip, result.error()};
  } else {
    chain_tip_block_ = (*result)->height;
  }
  ScheduleWorkOnTask();
}

void ZCashShieldSyncService::DownloadBlocks() {
  zcash_rpc().GetCompactBlocks(
      chain_id_, *latest_scanned_block_ + 1,
      std::min(*chain_tip_block_, *latest_scanned_block_ + kScanBatchSize),
      base::BindOnce(&ZCashShieldSyncService::OnBlocksDownloaded,
                     weak_ptr_factory_.GetWeakPtr()));
}

void ZCashShieldSyncService::OnBlocksDownloaded(
    base::expected<std::vector<zcash::mojom::CompactBlockPtr>, std::string>
        result) {
  if (!result.has_value()) {
    error_ = Error{ErrorCode::kFailedToDownloadBlocks, result.error()};
  } else {
    downloaded_blocks_ = std::move(result.value());
  }
  ScheduleWorkOnTask();
}

void ZCashShieldSyncService::ScanBlocks() {
  if (!downloaded_blocks_ || downloaded_blocks_->empty()) {
    error_ = Error{ErrorCode::kScannerError, ""};
    ScheduleWorkOnTask();
    return;
  }

  auto last_block_hash = ToHex(downloaded_blocks_->back()->hash);
  auto last_block_height = downloaded_blocks_->back()->height;

  block_scanner_->ScanBlocks(
      *spendable_notes_, std::move(downloaded_blocks_.value()),
      base::BindOnce(&ZCashShieldSyncService::OnBlocksScanned,
                     weak_ptr_factory_.GetWeakPtr(), last_block_height,
                     last_block_hash));
}

void ZCashShieldSyncService::OnBlocksScanned(
    uint32_t last_block_height,
    std::string last_block_hash,
    base::expected<OrchardBlockScanner::Result, OrchardBlockScanner::ErrorCode>
        result) {
  downloaded_blocks_ = std::nullopt;
  if (!result.has_value()) {
    error_ = Error{ErrorCode::kScannerError, ""};
    ScheduleWorkOnTask();
  } else {
    UpdateNotes(result->discovered_notes, result->spent_notes,
                last_block_height, last_block_hash);
  }
}

void ZCashShieldSyncService::UpdateNotes(
    const std::vector<OrchardNote>& found_notes,
    const std::vector<OrchardNoteSpend>& notes_to_delete,
    uint32_t latest_scanned_block,
    std::string latest_scanned_block_hash) {
  orchard_storage()
      .AsyncCall(&ZCashOrchardStorage::UpdateNotes)
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
    error_ = Error{ErrorCode::kFailedToUpdateDatabase, error->message};
  } else {
    latest_scanned_block_ = new_latest_scanned_block;
    spendable_notes_ = std::nullopt;
  }
  ScheduleWorkOnTask();
}

uint32_t ZCashShieldSyncService::GetSpendableBalance() {
  CHECK(spendable_notes_.has_value());
  uint32_t balance = 0;
  for (const auto& note : spendable_notes_.value()) {
    balance += note.amount;
  }
  return balance;
}

ZCashRpc& ZCashShieldSyncService::zcash_rpc() {
  return zcash_wallet_service_->zcash_rpc();
}

base::SequenceBound<ZCashOrchardStorage>&
ZCashShieldSyncService::orchard_storage() {
  return zcash_wallet_service_->orchard_storage();
}

}  // namespace brave_wallet
