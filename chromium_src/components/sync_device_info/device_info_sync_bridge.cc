// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/components/sync_device_info/brave_device_info.h"

#define BRAVE_MAKE_LOCAL_DEVICE_SPECIFICS \
  specifics->mutable_brave_fields()->set_is_self_delete_supported(true);

#include "../../../../components/sync_device_info/device_info_sync_bridge.cc"

#undef BRAVE_MAKE_LOCAL_DEVICE_SPECIFICS

#include "base/threading/sequenced_task_runner_handle.h"

namespace syncer {

namespace {

std::unique_ptr<BraveDeviceInfo> BraveSpecificsToModel(
    const DeviceInfoSpecifics& specifics) {
  ModelTypeSet data_types;
  for (const int field_number :
       specifics.invalidation_fields().interested_data_type_ids()) {
    ModelType data_type = GetModelTypeFromSpecificsFieldNumber(field_number);
    if (!IsRealDataType(data_type)) {
      DLOG(WARNING) << "Unknown field number " << field_number;
      continue;
    }
    data_types.Put(data_type);
  }
  // The code is duplicated from SpecificsToModel by intent to avoid use of
  // extra patch
  return std::make_unique<BraveDeviceInfo>(
      specifics.cache_guid(), specifics.client_name(),
      specifics.chrome_version(), specifics.sync_user_agent(),
      specifics.device_type(), specifics.signin_scoped_device_id(),
      specifics.manufacturer(), specifics.model(),
      ProtoTimeToTime(specifics.last_updated_timestamp()),
      GetPulseIntervalFromSpecifics(specifics),
      specifics.feature_fields().send_tab_to_self_receiving_enabled(),
      SpecificsToSharingInfo(specifics),
      SpecificsToPhoneAsASecurityKeyInfo(specifics),
      specifics.invalidation_fields().instance_id_token(), data_types,
      specifics.has_brave_fields() &&
          specifics.brave_fields().has_is_self_delete_supported() &&
          specifics.brave_fields().is_self_delete_supported());
}

}  // namespace

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

std::vector<std::unique_ptr<BraveDeviceInfo>>
DeviceInfoSyncBridge::GetAllBraveDeviceInfo() const {
  std::vector<std::unique_ptr<BraveDeviceInfo>> list;
  for (auto iter = all_data_.begin(); iter != all_data_.end(); ++iter) {
    list.push_back(BraveSpecificsToModel(*iter->second));
  }
  return list;
}

}  // namespace syncer
