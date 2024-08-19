/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_SHIELD_SYNC_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_SHIELD_SYNC_SERVICE_H_

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "base/memory/scoped_refptr.h"
#include "base/threading/sequence_bound.h"
#include "base/types/expected.h"
#include "brave/components/brave_wallet/browser/internal/orchard_block_scanner.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_orchard_storage.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_rpc.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "mojo/public/cpp/bindings/remote.h"

namespace brave_wallet {

// ZCashScanService downloads and scans blockchain blocks to find
// spendable notes related to the account.
// Provided full view key allows to decode orchard compact actions
// related to the account.
class ZCashShieldSyncService {
 public:
  enum ErrorCode {
    kFailedToDownloadBlocks = 0,
    kFailedToUpdateDatabase = 1,
    kFailedToUpdateChainTip = 2,
    kFailedToRetrieveSpendableNotes = 3,
    kFailedToReceiveTreeState = 4,
    kFailedToInitAccount = 5,
    kFailedToRetrieveAccount = 6,
    kScannerError = 7,
  };

  struct Error {
    ErrorCode code;
    std::string message;
  };

  class OrchardBlockScannerProxy {
   public:
    OrchardBlockScannerProxy(
        std::array<uint8_t, kOrchardFullViewKeySize> full_view_key);
    virtual ~OrchardBlockScannerProxy();
    virtual void ScanBlocks(
        std::vector<OrchardNote> known_notes,
        std::vector<zcash::mojom::CompactBlockPtr> blocks,
        base::OnceCallback<void(base::expected<OrchardBlockScanner::Result,
                                               OrchardBlockScanner::ErrorCode>)>
            callback);

   private:
    base::SequenceBound<OrchardBlockScanner> background_block_scanner_;
  };

  ZCashShieldSyncService(
      ZCashRpc* zcash_rpc,
      const mojom::AccountIdPtr& account_id,
      const mojom::ZCashAccountShieldBirthdayPtr& account_birthday,
      const std::array<uint8_t, kOrchardFullViewKeySize>& fvk,
      base::FilePath db_dir_path);
  virtual ~ZCashShieldSyncService();

  bool IsStarted();

  void StartSyncing(mojo::PendingRemote<mojom::ZCashSyncObserver> observer);
  void PauseSyncing();

  mojom::ZCashShieldSyncStatusPtr GetSyncStatus();

 private:
  FRIEND_TEST_ALL_PREFIXES(ZCashShieldSyncServiceTest, ScanBlocks);

  void SetOrchardBlockScannerProxyForTesting(
      std::unique_ptr<OrchardBlockScannerProxy> block_scanner);

  void ScheduleWorkOnTask();
  void WorkOnTask();

  // Setup account info
  void GetAccountMeta();
  void OnGetAccountMeta(base::expected<ZCashOrchardStorage::AccountMeta,
                                       ZCashOrchardStorage::Error> result);
  void InitAccount();
  void OnAccountInit(std::optional<ZCashOrchardStorage::Error> error);

  // Get last known block in the blockchain
  void UpdateChainTip();
  void OnGetLatestBlock(
      base::expected<zcash::mojom::BlockIDPtr, std::string> result);

  // Chain reorg flow
  // Chain reorg happens when latest blocks are removed from the blockchain
  // We assume that there is a limit of reorg depth - kChainReorgBlockDelta

  // Verifies that last known scanned block hash is unchanged
  void VerifiyChainState(ZCashOrchardStorage::AccountMeta account_meta);
  void OnGetTreeStateForChainVerification(
      ZCashOrchardStorage::AccountMeta account_meta,
      base::expected<zcash::mojom::TreeStatePtr, std::string> tree_state);

  // Resolves block hash for the block we are going to fallback
  void GetTreeStateForChainReorg(uint32_t new_block_id);
  void OnGetTreeStateForChainReorg(
      uint32_t new_block_height,
      base::expected<zcash::mojom::TreeStatePtr, std::string> tree_state);
  void OnDatabaseUpdatedForChainReorg(
      uint32_t new_block_height,
      std::optional<ZCashOrchardStorage::Error> error);

  // Update spendable notes state
  void UpdateSpendableNotes();
  void OnGetSpendableNotes(base::expected<std::vector<OrchardNote>,
                                          ZCashOrchardStorage::Error> result);

  // Download, scan, update flow
  // Download next bunch of blocks
  void DownloadBlocks();
  void OnBlocksDownloaded(
      base::expected<std::vector<zcash::mojom::CompactBlockPtr>, std::string>
          result);
  // Process a bunch of downloaded blocks to resolve related notes and
  // nullifiers
  void ScanBlocks(std::vector<zcash::mojom::CompactBlockPtr> blocks);
  void OnBlocksScanned(uint32_t blocks_count,
                       std::string last_block_hash,
                       base::expected<OrchardBlockScanner::Result,
                                      OrchardBlockScanner::ErrorCode> result);
  void UpdateNotes(const std::vector<OrchardNote>& found_notes,
                   const std::vector<OrchardNullifier>& notes_to_delete,
                   uint32_t latest_scanned_block,
                   std::string latest_scanned_block_hash);
  void UpdateNotesComplete(uint32_t new_latest_scanned_block,
                           std::optional<ZCashOrchardStorage::Error> error);

  uint32_t GetSpendableBalance();
  std::optional<Error> error() { return error_; }

  // Params
  raw_ptr<ZCashRpc> zcash_rpc_ = nullptr;
  mojom::AccountIdPtr account_id_;
  // Birthday of the account will be used to resolve initial scan range.
  mojom::ZCashAccountShieldBirthdayPtr account_birthday_;
  base::FilePath db_dir_path_;
  std::string chain_id_;

  mojo::Remote<mojom::ZCashSyncObserver> observer_;

  base::SequenceBound<ZCashOrchardStorage> background_orchard_storage_;
  std::unique_ptr<OrchardBlockScannerProxy> block_scanner_;

  // Latest scanned block
  std::optional<size_t> latest_scanned_block_;
  // Latest block in the blockchain
  std::optional<size_t> chain_tip_block_;
  // Local cache of spendable notes to fast check on discovered nullifiers
  std::optional<std::vector<OrchardNote>> spendable_notes_;
  std::optional<Error> error_;
  bool stopped_ = false;

  mojom::ZCashShieldSyncStatusPtr current_sync_status_;

  base::WeakPtrFactory<ZCashShieldSyncService> weak_ptr_factory_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_SHIELD_SYNC_SERVICE_H_
