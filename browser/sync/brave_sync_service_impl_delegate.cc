/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/sync/brave_sync_service_impl_delegate.h"

#include <algorithm>
#include <utility>

#include "base/functional/callback_helpers.h"
#include "base/metrics/histogram_functions.h"
#include "base/task/single_thread_task_runner.h"
#include "brave/components/sync/service/brave_sync_service_impl.h"
#include "components/history/core/browser/history_service.h"
#include "components/sync_device_info/device_info_sync_service.h"
#include "components/sync_device_info/device_info_tracker.h"
#include "components/sync_device_info/local_device_info_provider.h"

namespace syncer {

BraveSyncServiceImplDelegate::BraveSyncServiceImplDelegate(
    DeviceInfoSyncService* device_info_sync_service,
    history::HistoryService* history_service)
    : device_info_sync_service_(device_info_sync_service),
      history_service_(history_service),
      weak_ptr_factory_(this) {
  DCHECK(device_info_sync_service_);

  local_device_info_provider_ =
      device_info_sync_service_->GetLocalDeviceInfoProvider();

  device_info_tracker_ = device_info_sync_service_->GetDeviceInfoTracker();
  DCHECK(device_info_tracker_);

  device_info_observer_.Observe(device_info_tracker_);
}

BraveSyncServiceImplDelegate::~BraveSyncServiceImplDelegate() = default;

void BraveSyncServiceImplDelegate::OnDeviceInfoChange() {
  DCHECK(sync_service_impl_);

  RecordP3ASyncStatus();

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

  if (found_local_device && local_device_appeared_callback_) {
    std::move(local_device_appeared_callback_).Run();
  }

  // When our device was removed from the sync chain by some other device,
  // we don't seee it in devices list, we must reset sync in a proper way
  if (!found_local_device) {
    // We can't call OnSelfDeviceInfoDeleted directly because we are on
    // remove device execution path, so posting task
    base::SingleThreadTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE,
        base::BindOnce(&BraveSyncServiceImplDelegate::OnSelfDeviceInfoDeleted,
                       weak_ptr_factory_.GetWeakPtr()));
  }
}

void BraveSyncServiceImplDelegate::OnSelfDeviceInfoDeleted() {
  sync_service_impl_->OnSelfDeviceInfoDeleted(base::DoNothing());
}

void BraveSyncServiceImplDelegate::SuspendDeviceObserverForOwnReset() {
  device_info_observer_.Reset();
}

void BraveSyncServiceImplDelegate::ResumeDeviceObserver() {
  if (!device_info_observer_.IsObserving()) {
    device_info_observer_.Observe(device_info_tracker_);
  }
}

void BraveSyncServiceImplDelegate::RecordP3ASyncStatus() {
  int num_devices = device_info_tracker_->GetAllDeviceInfo().size();

  // 0 - sync is disabled
  // 1 - one device in chain
  // 2 - two devices in chain
  // 3 - three or more devices in chain
  int p3a_value = std::min(num_devices, 3);

  base::UmaHistogramExactLinear("Brave.Sync.Status.2", p3a_value, 3);
}

void BraveSyncServiceImplDelegate::SetLocalDeviceAppearedCallback(
    base::OnceCallback<void()> local_device_appeared_callback) {
  local_device_appeared_callback_ = std::move(local_device_appeared_callback);
}

void BraveSyncServiceImplDelegate::GetKnownToSyncHistoryCount(
    base::OnceCallback<void(std::pair<bool, int>)> callback) {
  history_service_->GetKnownToSyncCount(base::BindOnce(
      [](base::OnceCallback<void(std::pair<bool, int>)> callback,
         history::HistoryCountResult known_to_sync) {
        std::move(callback).Run(
            std::pair(known_to_sync.success, known_to_sync.count));
      },
      std::move(callback)));
}

}  // namespace syncer
