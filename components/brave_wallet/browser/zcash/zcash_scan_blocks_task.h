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

class ZCashScanBlocksTask {
 public:
  using ZCashScanBlocksTaskObserver = base::RepeatingCallback<void(
      base::expected<ZCashShieldSyncService::ScanRangeResult,
                     ZCashShieldSyncService::Error>)>;

  ZCashScanBlocksTask(ZCashShieldSyncService::Context& context,
                      ZCashShieldSyncService::OrchardBlockScannerProxy& scanner,
                      ZCashScanBlocksTaskObserver observer,
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

  raw_ref<ZCashShieldSyncService::Context> context_;
  raw_ref<ZCashShieldSyncService::OrchardBlockScannerProxy> scanner_;

  ZCashScanBlocksTaskObserver observer_;

  std::optional<uint32_t> to_;
  std::optional<uint32_t> start_block_;
  std::optional<uint32_t> end_block_;

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
