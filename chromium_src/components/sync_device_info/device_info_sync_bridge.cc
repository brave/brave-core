// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "components/sync_device_info/device_info_sync_bridge.h"

#include "components/sync/base/deletion_origin.h"
#include "components/sync/protocol/device_info_specifics.pb.h"

namespace syncer {
namespace {

// Our specifics support for SelfDeleteSupport.
SelfDeleteSupport SpecificsToSelfDeleteSupport(
    const sync_pb::DeviceInfoSpecifics& specifics) {
  if (specifics.has_brave_fields() &&
      specifics.brave_fields().has_is_self_delete_supported() &&
      specifics.brave_fields().is_self_delete_supported()) {
    return SelfDeleteSupport::kSupported;
  }
  return SelfDeleteSupport::kNotSupported;
}

// Forward declared so upstream's caller of `MakeLocalDeviceSpecifics` resolves
// to our wrapper rather than to the renamed `_ChromiumImpl`
std::unique_ptr<sync_pb::DeviceInfoSpecifics> MakeLocalDeviceSpecifics(
    const DeviceInfo& info);

}  // namespace
}  // namespace syncer

#include "base/task/sequenced_task_runner.h"

#include <components/sync_device_info/device_info_sync_bridge.cc>

namespace syncer {

namespace {

constexpr int kFailedAttemtpsToAckDeviceDelete = 5;

std::unique_ptr<DeviceInfoSpecifics> MakeLocalDeviceSpecifics(
    const DeviceInfo& info) {
  std::unique_ptr<DeviceInfoSpecifics> specifics =
      MakeLocalDeviceSpecifics_ChromiumImpl(info);
  specifics->mutable_brave_fields()->set_is_self_delete_supported(true);
  return specifics;
}

}  // namespace

void DeviceInfoSyncBridge::DeleteDeviceInfo(const std::string& client_id,
                                            base::OnceClosure callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  TRACE_EVENT0("sync", "DeviceInfoSyncBridge::DeleteDeviceInfo");
  CHECK(store_);
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

std::vector<std::unique_ptr<DeviceInfo>>
DeviceInfoSyncBridge::GetAllBraveDeviceInfo() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  TRACE_EVENT0("sync", "DeviceInfoSyncBridge::GetAllBraveDeviceInfo");
  std::vector<std::unique_ptr<DeviceInfo>> list;
  for (const auto& data : all_data_) {
    list.push_back(std::make_unique<DeviceInfo>(
        SpecificsToModel(data.second.specifics())));
  }
  return list;
}

// Tucking this function away here because `DeviceInfoTracker` has not
// translation unit, and the clang plugin wont allow the definition in the
// header. This function has to provide a dead definition, otherwise there are
// certain types of breakages that require patching upstream code.
std::vector<std::unique_ptr<DeviceInfo>>
DeviceInfoTracker::GetAllBraveDeviceInfo() const {
  NOTREACHED() << "This function must be overriden";
}

}  // namespace syncer
