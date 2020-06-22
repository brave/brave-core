// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "../../../../components/sync_device_info/device_info_sync_bridge.cc"

#include "base/threading/sequenced_task_runner_handle.h"

namespace syncer {

void DeviceInfoSyncBridge::DeleteDeviceInfo(const std::string& client_id,
                                            base::OnceClosure callback) {
  std::unique_ptr<WriteBatch> batch = store_->CreateWriteBatch();
  change_processor()->Delete(client_id, batch->GetMetadataChangeList());
  DeleteSpecifics(client_id, batch.get());
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

}  // namespace syncer
