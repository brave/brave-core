/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_SYNC_BRAVE_SYNC_SERVICE_IMPL_DELEGATE_H_
#define BRAVE_BROWSER_SYNC_BRAVE_SYNC_SERVICE_IMPL_DELEGATE_H_

#include "brave/components/sync/driver/sync_service_impl_delegate.h"

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/scoped_observation.h"
#include "components/sync_device_info/device_info_tracker.h"

class Profile;

namespace syncer {

class DeviceInfoSyncService;
class DeviceInfoTracker;
class LocalDeviceInfoProvider;

// Helper class to prevent pass of profile pointer into BraveSyncServiceImpl
// and to keep DeviceInfoSyncService
class BraveSyncServiceImplDelegate
    : public SyncServiceImplDelegate,
      public syncer::DeviceInfoTracker::Observer {
 public:
  explicit BraveSyncServiceImplDelegate(
      DeviceInfoSyncService* device_info_sync_service);
  ~BraveSyncServiceImplDelegate() override;

  void SuspendDeviceObserverForOwnReset() override;
  void ResumeDeviceObserver() override;

 private:
  // syncer::DeviceInfoTracker::Observer:
  void OnDeviceInfoChange() override;

  void OnSelfDeviceInfoDeleted(void);

  void RecordP3ASyncStatus();

  raw_ptr<syncer::DeviceInfoTracker> device_info_tracker_ = nullptr;
  raw_ptr<syncer::LocalDeviceInfoProvider> local_device_info_provider_ =
      nullptr;
  base::ScopedObservation<syncer::DeviceInfoTracker,
                          syncer::DeviceInfoTracker::Observer>
      device_info_observer_{this};

  raw_ptr<DeviceInfoSyncService> device_info_sync_service_ = nullptr;

  base::WeakPtrFactory<BraveSyncServiceImplDelegate> weak_ptr_factory_;

  BraveSyncServiceImplDelegate(const BraveSyncServiceImplDelegate&) = delete;
  BraveSyncServiceImplDelegate& operator=(const BraveSyncServiceImplDelegate&) =
      delete;
};

}  // namespace syncer

#endif  // BRAVE_BROWSER_SYNC_BRAVE_SYNC_SERVICE_IMPL_DELEGATE_H_
