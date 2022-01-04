/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_REWARDS_VG_SYNC_SERVICE_H_
#define BRAVE_BROWSER_BRAVE_REWARDS_VG_SYNC_SERVICE_H_

#include <memory>
#include <string>
#include <vector>

#include "brave/browser/brave_rewards/vg_body_sync_bridge.h"
#include "brave/browser/brave_rewards/vg_spend_status_sync_bridge.h"
#include "brave/components/services/bat_ledger/public/interfaces/bat_ledger.mojom.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/sync/model/data_batch.h"
#include "components/sync/model/model_type_controller_delegate.h"

class VgSyncService : public KeyedService {
 public:
  // using GetPairsCallback =
  //     base::OnceCallback<void(std::vector<bat_ledger::mojom::PairPtr>)>;

  explicit VgSyncService(
      std::unique_ptr<VgBodySyncBridge> vg_body_sync_bridge,
      std::unique_ptr<VgSpendStatusSyncBridge> vg_spend_status_sync_bridge);

  VgSyncService(const VgSyncService&) = delete;
  VgSyncService& operator=(const VgSyncService&) = delete;

  ~VgSyncService() override;

  base::WeakPtr<syncer::ModelTypeControllerDelegate>
  GetControllerDelegateForVgBodies();

  base::WeakPtr<syncer::ModelTypeControllerDelegate>
  GetControllerDelegateForVgSpendStatuses();

  void Shutdown() override;

  // void AddPair(std::int64_t key, const std::string& value);

  // void GetPairs(GetPairsCallback callback);

 private:
  // void OnGetPairs(GetPairsCallback callback,
  //                 std::unique_ptr<syncer::DataBatch> data_batch);

  std::unique_ptr<VgBodySyncBridge> vg_body_sync_bridge_;
  std::unique_ptr<VgSpendStatusSyncBridge> vg_spend_status_sync_bridge_;
};

#endif  // BRAVE_BROWSER_BRAVE_REWARDS_VG_SYNC_SERVICE_H_
