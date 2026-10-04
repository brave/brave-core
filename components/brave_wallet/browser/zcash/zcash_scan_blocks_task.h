/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_SCAN_BLOCKS_TASK_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_SCAN_BLOCKS_TASK_H_

#include <deque>
#include <memory>
#include <string>

#include "base/threading/sequence_bound.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_blocks_batch_scan_task.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_shield_sync_service.h"

namespace brave_wallet {

inline constexpr uint32_t kZCashMaxTasksInProgress = 4u;

// ZCashScanBlocksTask scans blocks from the last scanned block to the provided
// right border. Splits this range to subranges and uses a bunch of smaller
// tasks to process.
// Posts a number of ZCashBlocksBatchScanTask to work in parallel.
// After a ZCashBlocksBatchScanTask is ready applies the result to the
// OrchardSyncState.
class ZCashScanBlocksTask {
 public:
  using ZCashScanBlocksTaskObserver = base::RepeatingCallback<void(
      base::expected<ZCashShieldSyncService::ScanRangeResult,
                     ZCashShieldSyncService::Error>)>;
  using ScanRange = ZCashBlocksBatchScanTask::ScanRange;

  ZCashScanBlocksTask(ZCashActionContext& context,
                      ZCashShieldSyncService::OrchardBlockScannerProxy& scanner,
                      ZCashScanBlocksTaskObserver observer,
                      // Right border for scanning, chain tip is used
                      // if std::nullopt provided
                      std::optional<uint32_t> to);
  ~ZCashScanBlocksTask();

  void Start();

  void set_max_tasks_in_progress(uint32_t tasks) {
    max_tasks_in_progress_ = tasks;
  }

 private:
  void ScheduleWorkOnTask();
  void WorkOnTask();

  void GetAccountMeta();
  void OnGetAccountMeta(
      base::expected<std::optional<OrchardStorage::AccountMeta>,
                     OrchardStorage::Error> result);

  void GetChainTip();
  void OnGetChainTip(
      base::expected<zcash::mojom::BlockIDPtr, std::string> result);

  void PrepareScanRanges();

  void MaybeScanRanges();
  void OnScanningRangeComplete(
      base::expected<void, ZCashShieldSyncService::Error> result);

  void MaybeInsertResult();
  void OnResultInserted(
      ScanRange scan_range,
      base::expected<OrchardStorage::Result, OrchardStorage::Error> result);

  void NotifyObserver();

  size_t ReadyScanTasks();

  raw_ref<ZCashActionContext> context_;
  raw_ref<ZCashShieldSyncService::OrchardBlockScannerProxy> scanner_;

  ZCashScanBlocksTaskObserver observer_;

  std::optional<uint32_t> to_;
  std::optional<uint32_t> start_block_;
  std::optional<uint32_t> end_block_;

  bool started_ = false;
  bool finished_ = false;
  uint32_t max_tasks_in_progress_ = kZCashMaxTasksInProgress;

  std::optional<ZCashShieldSyncService::Error> error_;
  std::optional<OrchardStorage::AccountMeta> account_meta_;
  std::optional<uint32_t> chain_tip_block_;
  std::optional<uint32_t> initial_ranges_count_;
  std::optional<std::deque<ScanRange>> pending_scan_ranges_;

  std::deque<ZCashBlocksBatchScanTask> scan_tasks_in_progress_;
  bool inserting_in_progress_ = false;

  base::WeakPtrFactory<ZCashScanBlocksTask> weak_ptr_factory_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_SCAN_BLOCKS_TASK_H_
