/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_rewards/vg_body_sync_bridge.h"

#include <utility>

#include "base/bind.h"
#include "base/callback_helpers.h"
#include "base/check_op.h"
#include "base/location.h"
#include "base/logging.h"
#include "components/sync/model/entity_change.h"
#include "components/sync/model/metadata_batch.h"
#include "components/sync/model/mutable_data_batch.h"

namespace {
std::string GetStorageKeyFromSpecifics(
    const sync_pb::VgBodySpecifics& vg_body) {
  return vg_body.creds_id();
}

std::unique_ptr<syncer::EntityData> ToEntityData(
    sync_pb::VgBodySpecifics&& vg_body) {
  auto entity_data = std::make_unique<syncer::EntityData>();
  entity_data->id = vg_body.creds_id();
  entity_data->name = vg_body.creds_id();
  entity_data->specifics.mutable_vg_body()->Swap(&vg_body);

  return entity_data;
}
}  // namespace

VgBodySyncBridge::VgBodySyncBridge(
    std::unique_ptr<syncer::ModelTypeChangeProcessor> change_processor,
    syncer::OnceModelTypeStoreFactory store_factory)
    : syncer::ModelTypeSyncBridge(std::move(change_processor)) {
  std::move(store_factory)
      .Run(syncer::VG_BODIES, base::BindOnce(&VgBodySyncBridge::OnStoreCreated,
                                             weak_ptr_factory_.GetWeakPtr()));
}

VgBodySyncBridge::~VgBodySyncBridge() = default;

base::WeakPtr<syncer::ModelTypeControllerDelegate>
VgBodySyncBridge::GetControllerDelegate() {
  return change_processor()->GetControllerDelegate();
}

void VgBodySyncBridge::AddVgBody(sync_pb::VgBodySpecifics vg_body) {
  if (!store_) {
    return;
  }

  if (!change_processor()->IsTrackingMetadata()) {
    return;
  }

  // LOG(INFO) << "Adding pair { " << pair.key() << ", " << pair.value()
  //           << " } ...";

  const std::string storage_key = GetStorageKeyFromSpecifics(vg_body);

  auto write_batch = store_->CreateWriteBatch();
  write_batch->WriteData(storage_key, vg_body.SerializeAsString());
  change_processor()->Put(storage_key, ToEntityData(std::move(vg_body)),
                          write_batch->GetMetadataChangeList());

  store_->CommitWriteBatch(std::move(write_batch),
                           base::BindOnce(&VgBodySyncBridge::OnCommitWriteBatch,
                                          weak_ptr_factory_.GetWeakPtr()));
}

void VgBodySyncBridge::GetVgBodies(DataCallback callback) {
  GetAllDataForDebugging(std::move(callback));
}

std::unique_ptr<syncer::MetadataChangeList>
VgBodySyncBridge::CreateMetadataChangeList() {
  return syncer::ModelTypeStore::WriteBatch::CreateMetadataChangeList();
}

absl::optional<syncer::ModelError> VgBodySyncBridge::MergeSyncData(
    std::unique_ptr<syncer::MetadataChangeList> metadata_change_list,
    syncer::EntityChangeList entity_data) {
  DCHECK(change_processor()->IsTrackingMetadata());
  return ApplySyncChanges(std::move(metadata_change_list),
                          std::move(entity_data));
}

absl::optional<syncer::ModelError> VgBodySyncBridge::ApplySyncChanges(
    std::unique_ptr<syncer::MetadataChangeList> metadata_change_list,
    syncer::EntityChangeList entity_changes) {
  auto write_batch = store_->CreateWriteBatch();

  for (const auto& change : entity_changes) {
    if (change->type() == syncer::EntityChange::ACTION_DELETE) {
      write_batch->DeleteData(change->storage_key());
    } else {
      write_batch->WriteData(
          change->storage_key(),
          change->data().specifics.vg_body().SerializeAsString());
    }
  }

  write_batch->TakeMetadataChangesFrom(std::move(metadata_change_list));

  store_->CommitWriteBatch(std::move(write_batch),
                           base::BindOnce(&VgBodySyncBridge::OnCommitWriteBatch,
                                          weak_ptr_factory_.GetWeakPtr()));

  return {};
}

void VgBodySyncBridge::GetData(StorageKeyList storage_keys,
                               DataCallback callback) {
  store_->ReadData(storage_keys, base::BindOnce(&VgBodySyncBridge::OnReadData,
                                                weak_ptr_factory_.GetWeakPtr(),
                                                std::move(callback)));
}

void VgBodySyncBridge::GetAllDataForDebugging(DataCallback callback) {
  store_->ReadAllData(base::BindOnce(&VgBodySyncBridge::OnReadAllData,
                                     weak_ptr_factory_.GetWeakPtr(),
                                     std::move(callback)));
}

std::string VgBodySyncBridge::GetClientTag(
    const syncer::EntityData& entity_data) {
  return GetStorageKey(entity_data);
}

std::string VgBodySyncBridge::GetStorageKey(
    const syncer::EntityData& entity_data) {
  return GetStorageKeyFromSpecifics(entity_data.specifics.vg_body());
}

void VgBodySyncBridge::ApplyStopSyncChanges(
    std::unique_ptr<syncer::MetadataChangeList> delete_metadata_change_list) {
  if (delete_metadata_change_list) {
    store_->DeleteAllDataAndMetadata(
        base::BindOnce(&VgBodySyncBridge::OnDeleteAllDataAndMetadata,
                       weak_ptr_factory_.GetWeakPtr()));
  }
}

void VgBodySyncBridge::OnStoreCreated(
    const absl::optional<syncer::ModelError>& error,
    std::unique_ptr<syncer::ModelTypeStore> store) {
  if (error) {
    return change_processor()->ReportError(*error);
  }

  store_ = std::move(store);
  store_->ReadAllMetadata(base::BindOnce(&VgBodySyncBridge::OnReadAllMetadata,
                                         weak_ptr_factory_.GetWeakPtr()));
}

void VgBodySyncBridge::OnReadAllMetadata(
    const absl::optional<syncer::ModelError>& error,
    std::unique_ptr<syncer::MetadataBatch> metadata_batch) {
  if (error) {
    change_processor()->ReportError(*error);
  } else {
    change_processor()->ModelReadyToSync(std::move(metadata_batch));
  }
}

void VgBodySyncBridge::OnCommitWriteBatch(
    const absl::optional<syncer::ModelError>& error) {
  if (error) {
    change_processor()->ReportError(*error);
  }
}

void VgBodySyncBridge::OnReadData(
    DataCallback callback,
    const absl::optional<syncer::ModelError>& error,
    std::unique_ptr<syncer::ModelTypeStore::RecordList> data_records,
    std::unique_ptr<syncer::ModelTypeStore::IdList> missing_id_list) {
  OnReadAllData(std::move(callback), error, std::move(data_records));
}

void VgBodySyncBridge::OnReadAllData(
    DataCallback callback,
    const absl::optional<syncer::ModelError>& error,
    std::unique_ptr<syncer::ModelTypeStore::RecordList> data_records) {
  if (error) {
    return change_processor()->ReportError(*error);
  }

  auto batch = std::make_unique<syncer::MutableDataBatch>();

  for (const auto& record : *data_records) {
    sync_pb::VgBodySpecifics vg_body;

    if (vg_body.ParseFromString(record.value)) {
      DCHECK_EQ(record.id, GetStorageKeyFromSpecifics(vg_body));
      batch->Put(record.id, ToEntityData(std::move(vg_body)));
    } else {
      return change_processor()->ReportError(
          {FROM_HERE, "Failed to deserialize VG bodies!"});
    }
  }

  std::move(callback).Run(std::move(batch));
}

void VgBodySyncBridge::OnDeleteAllDataAndMetadata(
    const absl::optional<syncer::ModelError>& error) {
  if (error) {
    change_processor()->ReportError(*error);
  }
}
