/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_SYNC_BRAVE_SYNC_WORKER_H_
#define BRAVE_IOS_BROWSER_API_SYNC_BRAVE_SYNC_WORKER_H_

#include "base/scoped_observer.h"
#include "components/sync/driver/sync_service.h"
#include "components/sync/driver/sync_service_observer.h"

#include <string>
#include <vector>

class ChromeBrowserState;

namespace syncer {
class BraveProfileSyncService;
class DeviceInfo;
}  // namespace syncer

class BraveSyncWorker : public syncer::SyncServiceObserver {
 public:
  BraveSyncWorker(ChromeBrowserState* browser_state_);
  ~BraveSyncWorker() override;

  bool SetSyncEnabled(bool enabled);
  std::string GetOrCreateSyncCode();
  bool SetSyncCode(const std::string& sync_code);
  const syncer::DeviceInfo* GetLocalDeviceInfo();
  std::vector<std::unique_ptr<syncer::DeviceInfo>> GetDeviceList();
  bool IsSyncEnabled();
  bool IsSyncFeatureActive();
  bool ResetSync();

 private:
  // syncer::SyncServiceObserver implementation.
    
  syncer::BraveProfileSyncService* GetSyncService() const;
  void OnStateChanged(syncer::SyncService* service) override;
  void OnSyncShutdown(syncer::SyncService* service) override;

  void OnLocalDeviceInfoDeleted();

  ChromeBrowserState* browser_state_;  // NOT OWNED
  ScopedObserver<syncer::SyncService, syncer::SyncServiceObserver>
      sync_service_observer_{this};
  base::WeakPtrFactory<BraveSyncWorker> weak_ptr_factory_{this};

  DISALLOW_COPY_AND_ASSIGN(BraveSyncWorker);
};

#endif /* BRAVE_IOS_BROWSER_API_SYNC_BRAVE_SYNC_WORKER_H_ */
