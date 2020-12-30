/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SYNC_DRIVER_PROFILE_SYNC_SERVICE_DELEGATE_H_
#define BRAVE_COMPONENTS_SYNC_DRIVER_PROFILE_SYNC_SERVICE_DELEGATE_H_

namespace syncer {

class BraveProfileSyncService;

class ProfileSyncServiceDelegate {
 public:
  virtual ~ProfileSyncServiceDelegate() {}
  virtual void SuspendDeviceObserverForOwnReset() = 0;
  virtual void ResumeDeviceObserver() = 0;

  void set_profile_sync_service(BraveProfileSyncService* profile_sync_service) {
    profile_sync_service_ = profile_sync_service;
  }

 protected:
  BraveProfileSyncService* profile_sync_service_;
};

}  // namespace syncer

#endif  // BRAVE_COMPONENTS_SYNC_DRIVER_PROFILE_SYNC_SERVICE_DELEGATE_H_
