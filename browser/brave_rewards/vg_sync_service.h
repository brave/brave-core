/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_REWARDS_VG_SYNC_SERVICE_H_
#define BRAVE_BROWSER_BRAVE_REWARDS_VG_SYNC_SERVICE_H_

#include <memory>
#include <string>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "brave/browser/brave_rewards/vg_body_sync_bridge.h"
#include "brave/browser/brave_rewards/vg_spend_status_sync_bridge.h"
#include "brave/components/sync/protocol/vg_specifics.pb.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/sync/model/data_batch.h"
#include "components/sync/model/model_type_controller_delegate.h"

namespace brave_rewards {
class VgSyncService : public KeyedService {
 public:
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

  void BackUpVgBodies(std::vector<sync_pb::VgBodySpecifics> vg_bodies);

  void BackUpVgSpendStatuses(
      std::vector<sync_pb::VgSpendStatusSpecifics> vg_spend_statuses);

  void WhenVgsReady(
      base::RepeatingCallback<
          void(std::vector<sync_pb::VgBodySpecifics>,
               std::vector<sync_pb::VgSpendStatusSpecifics>)> when_vgs_ready);

 private:
  std::unique_ptr<VgBodySyncBridge> vg_body_sync_bridge_;
  std::unique_ptr<VgSpendStatusSyncBridge> vg_spend_status_sync_bridge_;
  base::WeakPtrFactory<VgSyncService> weak_ptr_factory_{this};
};
}  // namespace brave_rewards

#endif  // BRAVE_BROWSER_BRAVE_REWARDS_VG_SYNC_SERVICE_H_
