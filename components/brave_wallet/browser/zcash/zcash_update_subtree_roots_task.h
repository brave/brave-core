/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_UPDATE_SUBTREE_ROOTS_TASK_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_UPDATE_SUBTREE_ROOTS_TASK_H_

#include "brave/components/brave_wallet/browser/zcash/zcash_shield_sync_service.h"

namespace brave_wallet {

class ZCashUpdateSubtreeRootsTask {
 public:
  using ZCashUpdateSubtreeRootsTaskCallback = base::OnceCallback<void(bool)>;
  ZCashUpdateSubtreeRootsTask(ZCashShieldSyncService* sync_service,
                              ZCashUpdateSubtreeRootsTaskCallback callback);
  ~ZCashUpdateSubtreeRootsTask();

  void Start();

 private:
  void OnGetLatestShardIndex(base::expected<std::optional<uint32_t>,
                                            ZCashOrchardStorage::Error> result);
  void GetSubtreeRoots(uint32_t start_index);
  void OnGetSubtreeRoots(
      uint32_t start_index,
      base::expected<std::vector<zcash::mojom::SubtreeRootPtr>, std::string>
          result);
  void OnSubtreeRootsUpdated(
      std::optional<size_t> next_start_index,
      base::expected<bool, ZCashOrchardStorage::Error> result);

  raw_ptr<ZCashShieldSyncService> sync_service_;  // Owns this
  ZCashUpdateSubtreeRootsTaskCallback callback_;

  base::WeakPtrFactory<ZCashUpdateSubtreeRootsTask> weak_ptr_factory_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_UPDATE_SUBTREE_ROOTS_TASK_H_
