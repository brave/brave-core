// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_BLOCKS_BATCH_SCAN_TASK_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_BLOCKS_BATCH_SCAN_TASK_H_

#include <string>
#include <vector>

#include "brave/components/brave_wallet/browser/zcash/zcash_shield_sync_service.h"
#include "brave/components/brave_wallet/common/zcash_utils.h"

namespace brave_wallet {

// Scans single scan range.
class ZCashBlocksBatchScanTask {
 public:
  using ZCashBlocksBatchScanTaskCallback = base::OnceCallback<void(
      base::expected<bool, ZCashShieldSyncService::Error>)>;
  ZCashBlocksBatchScanTask(
      ZCashActionContext& context,
      ZCashShieldSyncService::OrchardBlockScannerProxy& scanner,
      uint32_t from,
      uint32_t to,
      ZCashBlocksBatchScanTaskCallback callback);
  ~ZCashBlocksBatchScanTask();

  uint32_t from() const { return from_; }

  uint32_t to() const { return to_; }

  void Start();

 private:
  void WorkOnTask();
  void ScheduleWorkOnTask();

  void GetFrontierTreeState();
  void OnGetFrontierTreeState(
      base::expected<zcash::mojom::TreeStatePtr, std::string>);
  void GetFrontierBlock();
  void OnGetFrontierBlock(
      base::expected<std::vector<zcash::mojom::CompactBlockPtr>, std::string>
          result);

  void DownloadBlocks();
  void OnBlocksDownloaded(
      size_t expected_size,
      base::expected<std::vector<zcash::mojom::CompactBlockPtr>, std::string>
          result);

  void ScanBlocks();
  void OnBlocksScanned(base::expected<OrchardBlockScanner::Result,
                                      OrchardBlockScanner::ErrorCode> result);

  void UpdateDatabase();
  void OnDatabaseUpdated(
      base::expected<OrchardStorage::Result, OrchardStorage::Error> result);

  raw_ref<ZCashActionContext> context_;
  raw_ref<ZCashShieldSyncService::OrchardBlockScannerProxy> scanner_;
  uint32_t from_ = 0;
  uint32_t to_ = 0;
  ZCashBlocksBatchScanTaskCallback callback_;

  uint32_t frontier_block_height_ = 0;

  std::optional<ZCashShieldSyncService::Error> error_;
  std::optional<zcash::mojom::TreeStatePtr> frontier_tree_state_;
  std::optional<zcash::mojom::CompactBlockPtr> frontier_block_;
  std::optional<std::vector<zcash::mojom::CompactBlockPtr>> downloaded_blocks_;
  std::optional<OrchardBlockScanner::Result> scan_result_;
  std::optional<zcash::mojom::CompactBlockPtr> latest_scanned_block_;

  bool started_ = false;
  bool database_updated_ = false;

  base::WeakPtrFactory<ZCashBlocksBatchScanTask> weak_ptr_factory_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_BLOCKS_BATCH_SCAN_TASK_H_
