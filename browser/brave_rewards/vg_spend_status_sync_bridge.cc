/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_rewards/vg_spend_status_sync_bridge.h"

#include <utility>

#include "base/bind.h"
#include "base/callback_helpers.h"
#include "base/check_op.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "components/sync/model/entity_change.h"
#include "components/sync/model/metadata_batch.h"
#include "components/sync/model/mutable_data_batch.h"

namespace {
std::string GetStorageKeyFromSpecifics(
    const sync_pb::VgSpendStatusSpecifics& vg_spend_status) {
  return base::NumberToString(vg_spend_status.token_id());
}

std::unique_ptr<syncer::EntityData> ToEntityData(
    sync_pb::VgSpendStatusSpecifics&& vg_spend_status) {
  auto entity_data = std::make_unique<syncer::EntityData>();
  entity_data->id = base::NumberToString(vg_spend_status.token_id());
  entity_data->name = base::NumberToString(vg_spend_status.token_id());
  entity_data->specifics.mutable_vg_spend_status()->Swap(&vg_spend_status);

  return entity_data;
}
}  // namespace

VgSpendStatusSyncBridge::VgSpendStatusSyncBridge(
    std::unique_ptr<syncer::ModelTypeChangeProcessor> change_processor,
    syncer::OnceModelTypeStoreFactory store_factory)
    : syncer::ModelTypeSyncBridge(std::move(change_processor)) {
  std::move(store_factory)
      .Run(syncer::VG_SPEND_STATUSES,
           base::BindOnce(&VgSpendStatusSyncBridge::OnStoreCreated,
                          weak_ptr_factory_.GetWeakPtr()));
}

VgSpendStatusSyncBridge::~VgSpendStatusSyncBridge() = default;

base::WeakPtr<syncer::ModelTypeControllerDelegate>
VgSpendStatusSyncBridge::GetControllerDelegate() {
  return change_processor()->GetControllerDelegate();
}

void VgSpendStatusSyncBridge::AddVgSpendStatus(
    sync_pb::VgSpendStatusSpecifics vg_spend_status) {
  if (!store_) {
    return;
  }

  if (!change_processor()->IsTrackingMetadata()) {
    return;
  }

  //  LOG(INFO) << "Adding pair { " << pair.key() << ", " << pair.value()
  //            << " } ...";

  const std::string storage_key = GetStorageKeyFromSpecifics(vg_spend_status);

  auto write_batch = store_->CreateWriteBatch();
  write_batch->WriteData(storage_key, vg_spend_status.SerializeAsString());
  change_processor()->Put(storage_key, ToEntityData(std::move(vg_spend_status)),
                          write_batch->GetMetadataChangeList());

  store_->CommitWriteBatch(
      std::move(write_batch),
      base::BindOnce(&VgSpendStatusSyncBridge::OnCommitWriteBatch,
                     weak_ptr_factory_.GetWeakPtr()));
}

void VgSpendStatusSyncBridge::GetVgSpendStatuses(DataCallback callback) {
  GetAllDataForDebugging(std::move(callback));
}

std::unique_ptr<syncer::MetadataChangeList>
VgSpendStatusSyncBridge::CreateMetadataChangeList() {
  return syncer::ModelTypeStore::WriteBatch::CreateMetadataChangeList();
}

absl::optional<syncer::ModelError> VgSpendStatusSyncBridge::MergeSyncData(
    std::unique_ptr<syncer::MetadataChangeList> metadata_change_list,
    syncer::EntityChangeList entity_data) {
  DCHECK(change_processor()->IsTrackingMetadata());
  return ApplySyncChanges(std::move(metadata_change_list),
                          std::move(entity_data));
}

absl::optional<syncer::ModelError> VgSpendStatusSyncBridge::ApplySyncChanges(
    std::unique_ptr<syncer::MetadataChangeList> metadata_change_list,
    syncer::EntityChangeList entity_changes) {
  auto write_batch = store_->CreateWriteBatch();

  for (const auto& change : entity_changes) {
    if (change->type() == syncer::EntityChange::ACTION_DELETE) {
      write_batch->DeleteData(change->storage_key());
    } else {
      write_batch->WriteData(
          change->storage_key(),
          change->data().specifics.vg_spend_status().SerializeAsString());
    }
  }

  write_batch->TakeMetadataChangesFrom(std::move(metadata_change_list));

  store_->CommitWriteBatch(
      std::move(write_batch),
      base::BindOnce(&VgSpendStatusSyncBridge::OnCommitWriteBatch,
                     weak_ptr_factory_.GetWeakPtr()));

  return {};
}

void VgSpendStatusSyncBridge::GetData(StorageKeyList storage_keys,
                                      DataCallback callback) {
  store_->ReadData(
      storage_keys,
      base::BindOnce(&VgSpendStatusSyncBridge::OnReadData,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void VgSpendStatusSyncBridge::GetAllDataForDebugging(DataCallback callback) {
  store_->ReadAllData(base::BindOnce(&VgSpendStatusSyncBridge::OnReadAllData,
                                     weak_ptr_factory_.GetWeakPtr(),
                                     std::move(callback)));
}

std::string VgSpendStatusSyncBridge::GetClientTag(
    const syncer::EntityData& entity_data) {
  return GetStorageKey(entity_data);
}

std::string VgSpendStatusSyncBridge::GetStorageKey(
    const syncer::EntityData& entity_data) {
  return GetStorageKeyFromSpecifics(entity_data.specifics.vg_spend_status());
}

void VgSpendStatusSyncBridge::ApplyStopSyncChanges(
    std::unique_ptr<syncer::MetadataChangeList> delete_metadata_change_list) {
  if (delete_metadata_change_list) {
    store_->DeleteAllDataAndMetadata(
        base::BindOnce(&VgSpendStatusSyncBridge::OnDeleteAllDataAndMetadata,
                       weak_ptr_factory_.GetWeakPtr()));
  }
}

void VgSpendStatusSyncBridge::OnStoreCreated(
    const absl::optional<syncer::ModelError>& error,
    std::unique_ptr<syncer::ModelTypeStore> store) {
  if (error) {
    return change_processor()->ReportError(*error);
  }

  store_ = std::move(store);
  store_->ReadAllMetadata(
      base::BindOnce(&VgSpendStatusSyncBridge::OnReadAllMetadata,
                     weak_ptr_factory_.GetWeakPtr()));
}

void VgSpendStatusSyncBridge::OnReadAllMetadata(
    const absl::optional<syncer::ModelError>& error,
    std::unique_ptr<syncer::MetadataBatch> metadata_batch) {
  if (error) {
    change_processor()->ReportError(*error);
  } else {
    change_processor()->ModelReadyToSync(std::move(metadata_batch));
  }
}

void VgSpendStatusSyncBridge::OnCommitWriteBatch(
    const absl::optional<syncer::ModelError>& error) {
  if (error) {
    change_processor()->ReportError(*error);
  }
}

void VgSpendStatusSyncBridge::OnReadData(
    DataCallback callback,
    const absl::optional<syncer::ModelError>& error,
    std::unique_ptr<syncer::ModelTypeStore::RecordList> data_records,
    std::unique_ptr<syncer::ModelTypeStore::IdList> missing_id_list) {
  OnReadAllData(std::move(callback), error, std::move(data_records));
}

void VgSpendStatusSyncBridge::OnReadAllData(
    DataCallback callback,
    const absl::optional<syncer::ModelError>& error,
    std::unique_ptr<syncer::ModelTypeStore::RecordList> data_records) {
  if (error) {
    return change_processor()->ReportError(*error);
  }

  auto batch = std::make_unique<syncer::MutableDataBatch>();

  for (const auto& record : *data_records) {
    sync_pb::VgSpendStatusSpecifics vg_spend_status;

    if (vg_spend_status.ParseFromString(record.value)) {
      DCHECK_EQ(record.id, GetStorageKeyFromSpecifics(vg_spend_status));
      batch->Put(record.id, ToEntityData(std::move(vg_spend_status)));
    } else {
      return change_processor()->ReportError(
          {FROM_HERE, "Failed to deserialize VG spend statuses!"});
    }
  }

  std::move(callback).Run(std::move(batch));
}

void VgSpendStatusSyncBridge::OnDeleteAllDataAndMetadata(
    const absl::optional<syncer::ModelError>& error) {
  if (error) {
    change_processor()->ReportError(*error);
  }
}
