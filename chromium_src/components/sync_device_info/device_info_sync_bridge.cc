// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#define BRAVE_DEVICE_INFO_SYNC_BRIDGE \
  InitSyncTabsPrefChangeRegistrar()

#include "../../../../components/sync_device_info/device_info_sync_bridge.cc"

#include "base/threading/sequenced_task_runner_handle.h"
#include "components/sync/base/pref_names.h"

namespace syncer {

void DeviceInfoSyncBridge::DeleteDeviceInfo(const std::string& client_id,
                                            base::OnceClosure callback) {
  std::unique_ptr<WriteBatch> batch = store_->CreateWriteBatch();
  change_processor()->Delete(client_id, batch->GetMetadataChangeList());
  DeleteSpecifics(client_id, batch.get());
  batch->GetMetadataChangeList()->ClearMetadata(client_id);
  CommitAndNotify(std::move(batch), /*should_notify=*/true);
  base::SequencedTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(&DeviceInfoSyncBridge::OnDeviceInfoDeleted,
                     weak_ptr_factory_.GetWeakPtr(), client_id,
                     std::move(callback)),
      base::TimeDelta::FromSeconds(1));
}

void DeviceInfoSyncBridge::OnDeviceInfoDeleted(const std::string& client_id,
                                               base::OnceClosure callback) {
  // Make sure the deleted device info is sent
  if (change_processor()->IsEntityUnsynced(client_id)) {
    base::SequencedTaskRunnerHandle::Get()->PostDelayedTask(
        FROM_HERE,
        base::BindOnce(&DeviceInfoSyncBridge::OnDeviceInfoDeleted,
                       weak_ptr_factory_.GetWeakPtr(), client_id,
                       std::move(callback)),
        base::TimeDelta::FromSeconds(1));
  } else {
    std::move(callback).Run();
  }
}

void DeviceInfoSyncBridge::InitSyncTabsPrefChangeRegistrar() {
  // Monitor Open Tabs sync pref change
  brave_pref_change_registrar_.Init(device_info_prefs_->GetPrefService());

  brave_pref_change_registrar_.Add(
      prefs::kSyncTabs,
      base::Bind(&DeviceInfoSyncBridge::OnSyncTabsPrefsChanged,
                 weak_ptr_factory_.GetWeakPtr()));
}

void DeviceInfoSyncBridge::OnSyncTabsPrefsChanged(const std::string& pref) {
  DCHECK_EQ(pref, prefs::kSyncTabs);
  if (!local_device_info_provider_ ||
      !local_device_info_provider_->GetLocalDeviceInfo()) {
    return;
  }
  RefreshLocalDeviceInfo();
}

}  // namespace syncer
