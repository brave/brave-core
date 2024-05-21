/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_SCANNING_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_SCANNING_SERVICE_H_

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "base/memory/scoped_refptr.h"
#include "base/threading/sequence_bound.h"
#include "base/types/expected.h"
#include "brave/components/brave_wallet/browser/zcash/orchard_storage.h"
#include "brave/components/brave_wallet/browser/zcash/rust/lib.rs.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_rpc.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_transaction.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "components/keyed_service/core/keyed_service.h"
#include "content/public/browser/browser_context.h"

namespace brave_wallet {

constexpr size_t kSyncThreshold = 100;
constexpr size_t kScanBatchSize = 10;

struct ZCashBlockScannerResult {
  ZCashBlockScannerResult(std::vector<OrchardNote> discovered_notes,
                          std::vector<OrchardNullifier> spent_notes);
  ZCashBlockScannerResult(const ZCashBlockScannerResult&);
  ZCashBlockScannerResult& operator=(const ZCashBlockScannerResult&);
  ~ZCashBlockScannerResult();

  std::vector<OrchardNote> discovered_notes;
  std::vector<OrchardNullifier> spent_notes;
};

class ZCashBlockScanner {
 public:
  explicit ZCashBlockScanner(
      const std::array<uint8_t, kOrchardFullViewKeySize>& full_view_key);
  ~ZCashBlockScanner();

  base::expected<ZCashBlockScannerResult, std::string> ParseBlocks(
      std::vector<OrchardNote> known_nullifiers,
      std::vector<mojom::CompactBlockPtr> blocks);

 private:
  std::array<uint8_t, kOrchardFullViewKeySize> full_view_key_;
};

class ZCashScanService : public KeyedService {
 public:
  ZCashScanService(ZCashRpc* zcash_rpc,
                   const mojom::AccountIdPtr& account_id,
                   const mojom::ZCashAccountBirthdayPtr& account_birthday,
                   const std::array<uint8_t, kOrchardFullViewKeySize>& fvk,
                   base::FilePath db_dir_path);
  ~ZCashScanService() override;

  void UpdateAccountMeta();

  bool IsStarted();
  void StartSyncing(mojo::PendingRemote<mojom::ZCashSyncObserver> observer);
  void PauseSyncing();

  mojom::ZCashSyncStatusPtr GetSyncStatus();

 private:
  void Iterate();

  // Setup account info
  void GetAccountMeta();
  void OnGetAccountMeta(
      base::expected<AccountMeta, OrchardStorage::OrchardStorageError> result);
  void InitAccount();
  void OnAccountInit(std::optional<OrchardStorage::OrchardStorageError> error);

  void UpdateChainTip();
  void OnGetLatestBlock(base::expected<mojom::BlockIDPtr, std::string> result);

  // Chain reorg
  void VerifiyChainState(AccountMeta account_meta);
  void OnGetTreeStateForChainVerification(
      AccountMeta account_meta,
      base::expected<mojom::TreeStatePtr, std::string> tree_state);
  void GetTreeStateForChainReorg(uint64_t new_block_id);
  void OnGetTreeStateForChainReorg(
      base::expected<mojom::TreeStatePtr, std::string> tree_state);
  void OnDatabaseUpdatedForChainReorg(
      std::optional<OrchardStorage::OrchardStorageError> error);

  // Update spendable notes state
  void UpdateSpendableNotes();
  void OnGetSpendableNotes(
      base::expected<std::vector<OrchardNote>,
                     OrchardStorage::OrchardStorageError> result);

  // Download, scan, update flow
  void DownloadBlocks();
  void OnBlocksDownloaded(
      base::expected<std::vector<mojom::CompactBlockPtr>, std::string> result);
  void ScanBlocks(std::vector<mojom::CompactBlockPtr> blocks);
  void OnBlocksScanned(
      uint64_t blocks_count,
      std::string last_block_hash,
      base::expected<ZCashBlockScannerResult, std::string> result);
  void UpdateNotes(const std::vector<OrchardNote>& found_notes,
                   const std::vector<OrchardNullifier>& notes_to_delete,
                   uint64_t latest_scanned_block,
                   std::string latest_scanned_block_hash);
  void UpdateNotesComplete(
      uint64_t new_next_block_to_scan,
      std::optional<OrchardStorage::OrchardStorageError> error);

  uint64_t GetSpendableBalance();

  raw_ptr<ZCashRpc> zcash_rpc_ = nullptr;
  mojo::Remote<mojom::ZCashSyncObserver> observer_;

  std::string chain_id_;
  mojom::AccountIdPtr account_id_;
  mojom::ZCashAccountBirthdayPtr account_birthday_;
  std::array<uint8_t, kOrchardFullViewKeySize> full_view_key_;
  base::FilePath db_dir_path_;

  std::optional<const base::SequenceBound<OrchardStorage>>
      background_orchard_storage_;
  std::optional<const base::SequenceBound<ZCashBlockScanner>>
      background_block_scanner_;

  std::optional<size_t> next_block_to_scan_;
  std::optional<size_t> latest_block_;
  std::optional<std::vector<OrchardNote>> spendable_notes_;
  std::optional<std::string> error_;
  bool stopped_ = false;

  mojom::ZCashSyncStatusPtr current_sync_status_;

  base::WeakPtrFactory<ZCashScanService> weak_ptr_factory_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_SCANNING_SERVICE_H_
