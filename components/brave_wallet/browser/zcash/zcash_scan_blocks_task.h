// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_SCAN_BLOCKS_TASK_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_SCAN_BLOCKS_TASK_H_

#include "brave/components/brave_wallet/browser/zcash/zcash_blocks_batch_scan_task.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_shield_sync_service.h"

namespace brave_wallet {

class ZCashScanBlocksTask {
 public:
  using ZCashScanBlocksTaskObserver = base::RepeatingCallback<void(
      base::expected<ZCashShieldSyncService::ScanRangeResult,
                     ZCashShieldSyncService::Error>)>;

  ZCashScanBlocksTask(ZCashShieldSyncService* sync_service,
                      ZCashScanBlocksTaskObserver observer);
  ~ZCashScanBlocksTask();

  void Start();

 private:
  struct ScanRange {
    uint32_t from;
    uint32_t to;
  };
  void ScheduleWorkOnTask();
  void WorkOnTask();

  void GetLatestScannedBlock();
  void OnGetAccountMeta(base::expected<ZCashOrchardStorage::AccountMeta,
                                       ZCashOrchardStorage::Error> result);

  void GetChainTip();
  void OnGetChainTip(
      base::expected<zcash::mojom::BlockIDPtr, std::string> result);

  void PrepareScanRanges();

  void ScanRanges();
  void OnScanningRangeComplete(
      base::expected<bool, ZCashShieldSyncService::Error> result);

  raw_ptr<ZCashShieldSyncService> sync_service_;
  ZCashScanBlocksTaskObserver observer_;

  std::optional<ZCashShieldSyncService::Error> error_;
  std::optional<uint32_t> latest_scanned_block_;
  std::optional<uint32_t> chain_tip_block_;
  std::optional<std::deque<ScanRange>> scan_ranges_;
  std::optional<size_t> initial_ranges_count_;
  std::unique_ptr<ZCashBlocksBatchScanTask> current_block_range_;

  base::WeakPtrFactory<ZCashScanBlocksTask> weak_ptr_factory_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_SCAN_BLOCKS_TASK_H_
