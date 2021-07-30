/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/sync/brave_profile_sync_service_delegate.h"

#include <utility>

#include "base/task/post_task.h"
#include "base/threading/thread_task_runner_handle.h"
#include "brave/components/sync/driver/brave_sync_profile_sync_service.h"
#include "components/sync_device_info/device_info_sync_service.h"
#include "components/sync_device_info/device_info_tracker.h"
#include "components/sync_device_info/local_device_info_provider.h"

namespace syncer {

BraveProfileSyncServiceDelegate::BraveProfileSyncServiceDelegate(
    DeviceInfoSyncService* device_info_sync_service)
    : device_info_sync_service_(device_info_sync_service),
      weak_ptr_factory_(this) {
  DCHECK(device_info_sync_service_);

  local_device_info_provider_ =
      device_info_sync_service_->GetLocalDeviceInfoProvider();

  device_info_tracker_ = device_info_sync_service_->GetDeviceInfoTracker();
  DCHECK(device_info_tracker_);

  device_info_observer_.Observe(device_info_tracker_);
}

BraveProfileSyncServiceDelegate::~BraveProfileSyncServiceDelegate() {}

void BraveProfileSyncServiceDelegate::OnDeviceInfoChange() {
  DCHECK(profile_sync_service_);

  const syncer::DeviceInfo* local_device_info =
      local_device_info_provider_->GetLocalDeviceInfo();

  bool found_local_device = false;
  const auto all_devices = device_info_tracker_->GetAllDeviceInfo();
  for (const auto& device : all_devices) {
    if (local_device_info->guid() == device->guid()) {
      found_local_device = true;
      break;
    }
  }

  // When our device was removed from the sync chain by some other device,
  // we don't seee it in devices list, we must reset sync in a proper way
  if (!found_local_device) {
    // We can't call OnSelfDeviceInfoDeleted directly because we are on
    // remove device execution path, so posting task
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE,
        base::BindOnce(
            &BraveProfileSyncServiceDelegate::OnSelfDeviceInfoDeleted,
            weak_ptr_factory_.GetWeakPtr()));
  }
}

void BraveProfileSyncServiceDelegate::OnSelfDeviceInfoDeleted() {
  profile_sync_service_->OnSelfDeviceInfoDeleted(base::DoNothing::Once());
}

void BraveProfileSyncServiceDelegate::SuspendDeviceObserverForOwnReset() {
  device_info_observer_.Reset();
}

void BraveProfileSyncServiceDelegate::ResumeDeviceObserver() {
  if (!device_info_observer_.IsObserving()) {
    device_info_observer_.Observe(device_info_tracker_);
  }
}

}  // namespace syncer
