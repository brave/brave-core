/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_SYNC_BRAVE_PROFILE_SYNC_SERVICE_DELEGATE_H_
#define BRAVE_BROWSER_SYNC_BRAVE_PROFILE_SYNC_SERVICE_DELEGATE_H_

#include "brave/components/sync/driver/profile_sync_service_delegate.h"

#include "base/memory/weak_ptr.h"
#include "base/scoped_observation.h"
#include "components/sync_device_info/device_info_tracker.h"

class Profile;

namespace syncer {

class DeviceInfoSyncService;
class DeviceInfoTracker;
class LocalDeviceInfoProvider;

// Helper class to prevent pass of profile pointer into BraveProfileSyncService
// and to keep DeviceInfoSyncService
class BraveProfileSyncServiceDelegate
    : public ProfileSyncServiceDelegate,
      public syncer::DeviceInfoTracker::Observer {
 public:
  explicit BraveProfileSyncServiceDelegate(
      DeviceInfoSyncService* device_info_sync_service);
  ~BraveProfileSyncServiceDelegate() override;

  void SuspendDeviceObserverForOwnReset() override;
  void ResumeDeviceObserver() override;

 private:
  // syncer::DeviceInfoTracker::Observer:
  void OnDeviceInfoChange() override;

  void OnSelfDeviceInfoDeleted(void);

  syncer::DeviceInfoTracker* device_info_tracker_;
  syncer::LocalDeviceInfoProvider* local_device_info_provider_;
  base::ScopedObservation<syncer::DeviceInfoTracker,
                          syncer::DeviceInfoTracker::Observer>
      device_info_observer_{this};

  DeviceInfoSyncService* device_info_sync_service_;

  base::WeakPtrFactory<BraveProfileSyncServiceDelegate> weak_ptr_factory_;

  BraveProfileSyncServiceDelegate(const BraveProfileSyncServiceDelegate&) =
      delete;
  BraveProfileSyncServiceDelegate& operator=(
      const BraveProfileSyncServiceDelegate&) = delete;
};

}  // namespace syncer

#endif  // BRAVE_BROWSER_SYNC_BRAVE_PROFILE_SYNC_SERVICE_DELEGATE_H_
