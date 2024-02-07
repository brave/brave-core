/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SYNC_SERVICE_SYNC_SERVICE_IMPL_DELEGATE_H_
#define BRAVE_COMPONENTS_SYNC_SERVICE_SYNC_SERVICE_IMPL_DELEGATE_H_

#include <utility>

#include "base/functional/callback_forward.h"
#include "base/memory/raw_ptr.h"

namespace syncer {

class BraveSyncServiceImpl;

class SyncServiceImplDelegate {
 public:
  virtual ~SyncServiceImplDelegate() {}
  virtual void SuspendDeviceObserverForOwnReset() = 0;
  virtual void ResumeDeviceObserver() = 0;

  virtual void SetLocalDeviceAppearedCallback(
      base::OnceCallback<void()> local_device_appeared_callback) = 0;

  virtual void GetKnownToSyncHistoryCount(
      base::OnceCallback<void(std::pair<bool, int>)> callback) = 0;

  void set_profile_sync_service(BraveSyncServiceImpl* sync_service_impl) {
    sync_service_impl_ = sync_service_impl;
  }

 protected:
  raw_ptr<BraveSyncServiceImpl> sync_service_impl_ = nullptr;
};

}  // namespace syncer

#endif  // BRAVE_COMPONENTS_SYNC_SERVICE_SYNC_SERVICE_IMPL_DELEGATE_H_
