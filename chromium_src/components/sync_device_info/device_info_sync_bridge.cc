// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "components/sync_device_info/device_info_sync_bridge.h"

#include "brave/components/sync_device_info/brave_device_info.h"
#include "components/sync/base/deletion_origin.h"

#define BRAVE_MAKE_LOCAL_DEVICE_SPECIFICS \
  specifics->mutable_brave_fields()->set_is_self_delete_supported(true);

#define RefreshLocalDeviceInfoIfNeeded \
  RefreshLocalDeviceInfoIfNeeded_ChromiumImpl

// This macro disables Chromium's block which detects whether the local
// device record should be re-uploaded to server. We disable it because it
// breaks the ability to remove other device in sync chain for Brave
#define BRAVE_DEVICE_INFO_SYNC_BRIDGE_APPLY_SYNC_CHANGES_SKIP_NEXT_IF if (false)

#define BRAVE_SKIP_EXPIRE_OLD_ENTRIES return;

#define BRAVE_ON_READ_ALL_METADATA_CLEAR_PROGRESS_TOKEN         \
  if (!device_info_prefs_->IsResetDevicesProgressTokenDone()) { \
    metadata_batch->ClearProgressToken();                       \
    device_info_prefs_->SetResetDevicesProgressTokenDone();     \
  }

#include "src/components/sync_device_info/device_info_sync_bridge.cc"

#undef BRAVE_ON_READ_ALL_METADATA_CLEAR_PROGRESS_TOKEN
#undef BRAVE_SKIP_EXPIRE_OLD_ENTRIES
#undef BRAVE_DEVICE_INFO_SYNC_BRIDGE_APPLY_SYNC_CHANGES_SKIP_NEXT_IF
#undef RefreshLocalDeviceInfoIfNeeded
#undef BRAVE_MAKE_LOCAL_DEVICE_SPECIFICS

#include "base/containers/contains.h"
#include "base/task/sequenced_task_runner.h"

namespace syncer {

namespace {

const int kFailedAttemtpsToAckDeviceDelete = 5;

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
      specifics.device_type(),
      DeriveOsFromDeviceType(specifics.device_type(), specifics.manufacturer()),
      DeriveFormFactorFromDeviceType(specifics.device_type()),
      specifics.signin_scoped_device_id(), specifics.manufacturer(),
      specifics.model(), specifics.full_hardware_class(),
      ProtoTimeToTime(specifics.last_updated_timestamp()),
      GetPulseIntervalFromSpecifics(specifics),
      specifics.feature_fields().send_tab_to_self_receiving_enabled(),
      specifics.feature_fields().send_tab_to_self_receiving_type(),
      SpecificsToSharingInfo(specifics),
      SpecificsToPhoneAsASecurityKeyInfo(specifics),
      specifics.invalidation_fields().instance_id_token(), data_types,
      SpecificsToFloatingWorkspaceLastSigninTime(specifics),
      specifics.has_brave_fields() &&
          specifics.brave_fields().has_is_self_delete_supported() &&
          specifics.brave_fields().is_self_delete_supported());
}

}  // namespace

void DeviceInfoSyncBridge::DeleteDeviceInfo(const std::string& client_id,
                                            base::OnceClosure callback) {
  std::unique_ptr<WriteBatch> batch = store_->CreateWriteBatch();
  change_processor()->Delete(client_id, DeletionOrigin::Unspecified(),
                             batch->GetMetadataChangeList());
  DeleteSpecifics(client_id, batch.get());
  batch->GetMetadataChangeList()->ClearMetadata(client_id);
  CommitAndNotify(std::move(batch), /*should_notify=*/true);
  base::SequencedTaskRunner::GetCurrentDefault()->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(&DeviceInfoSyncBridge::OnDeviceInfoDeleted,
                     weak_ptr_factory_.GetWeakPtr(), client_id, 1,
                     std::move(callback)),
      base::Seconds(1));
}

void DeviceInfoSyncBridge::OnDeviceInfoDeleted(const std::string& client_id,
                                               const int attempt,
                                               base::OnceClosure callback) {
  // Make sure the deleted device info is sent
  if (change_processor()->IsEntityUnsynced(client_id) &&
      attempt < kFailedAttemtpsToAckDeviceDelete) {
    base::SequencedTaskRunner::GetCurrentDefault()->PostDelayedTask(
        FROM_HERE,
        base::BindOnce(&DeviceInfoSyncBridge::OnDeviceInfoDeleted,
                       weak_ptr_factory_.GetWeakPtr(), client_id, attempt + 1,
                       std::move(callback)),
        base::Seconds(1));
  } else {
    std::move(callback).Run();
  }
}

std::vector<std::unique_ptr<BraveDeviceInfo>>
DeviceInfoSyncBridge::GetAllBraveDeviceInfo() const {
  std::vector<std::unique_ptr<BraveDeviceInfo>> list;
  for (auto iter = all_data_.begin(); iter != all_data_.end(); ++iter) {
    list.push_back(BraveSpecificsToModel(iter->second.specifics()));
  }
  return list;
}

void DeviceInfoSyncBridge::RefreshLocalDeviceInfoIfNeeded() {
  const DeviceInfo* current_info =
      local_device_info_provider_->GetLocalDeviceInfo();
  if (!current_info) {
    return;
  }

  if (!base::Contains(all_data_, current_info->guid())) {
    // After initiating leave the sync chain `DeleteSpecifics` cleans
    // `all_data_` map.
    // It is possible that user close sync settings page or change the data type
    // before the confirmation `DeviceInfoSyncBridge::OnDeviceInfoDeleted()`
    // comes - this leaded to access to the invalid iterator and crash.
    return;
  }

  RefreshLocalDeviceInfoIfNeeded_ChromiumImpl();
}

}  // namespace syncer
