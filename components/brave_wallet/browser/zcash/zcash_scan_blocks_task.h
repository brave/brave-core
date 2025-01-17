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

// ZCashScanBlocksTask scans blocks from the last scanned block to the provided
// right border. Splits this range to subranges and uses a bunch of smaller
// tasks to process. Current implementation uses sequential scanning. Parallel
// implementation TBD. Notifies client with the progress. See also
// ZCashBlocksBatchScanTask.
class ZCashScanBlocksTask {
 public:
  using ZCashScanBlocksTaskObserver = base::RepeatingCallback<void(
      base::expected<ZCashShieldSyncService::ScanRangeResult,
                     ZCashShieldSyncService::Error>)>;

  ZCashScanBlocksTask(ZCashActionContext& context,
                      ZCashShieldSyncService::OrchardBlockScannerProxy& scanner,
                      ZCashScanBlocksTaskObserver observer,
                      // Right border for scanning, chain tip is used
                      // if std::nullopt provided
                      std::optional<uint32_t> to);
  ~ZCashScanBlocksTask();

  void Start();

 private:
  struct ScanRange {
    uint32_t from;
    uint32_t to;
  };
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

  void ScanRanges();
  void OnScanningRangeComplete(
      base::expected<bool, ZCashShieldSyncService::Error> result);

  raw_ref<ZCashActionContext> context_;
  raw_ref<ZCashShieldSyncService::OrchardBlockScannerProxy> scanner_;

  ZCashScanBlocksTaskObserver observer_;

  std::optional<uint32_t> to_;
  std::optional<uint32_t> start_block_;
  std::optional<uint32_t> end_block_;

  bool started_ = false;

  std::optional<ZCashShieldSyncService::Error> error_;
  std::optional<OrchardStorage::AccountMeta> account_meta_;
  std::optional<uint32_t> chain_tip_block_;
  std::optional<std::deque<ScanRange>> scan_ranges_;
  std::optional<size_t> initial_ranges_count_;
  std::unique_ptr<ZCashBlocksBatchScanTask> current_block_range_;

  base::WeakPtrFactory<ZCashScanBlocksTask> weak_ptr_factory_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_SCAN_BLOCKS_TASK_H_
