/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/playlist/browser/playlist_sync_bridge.h"

#include <algorithm>
#include <memory>

#include "absl/types/optional.h"
#include "base/check.h"
#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/strings/string_split.h"
#include "base/synchronization/lock.h"
#include "brave/components/sync/protocol/playlist_specifics.pb.h"
#include "components/sync/base/model_type.h"
#include "components/sync/model/client_tag_based_model_type_processor.h"
#include "components/sync/model/metadata_change_list.h"
#include "components/sync/model/model_type_sync_bridge.h"
#include "components/sync/model/mutable_data_batch.h"

namespace playlist {
namespace sync {

namespace {

using syncer::EntityChange;
using syncer::EntityChangeList;
using syncer::EntityData;
using syncer::MetadataChangeList;
using syncer::ModelError;
using syncer::ModelTypeStore;

enum class DetailsType {
  kOrder,
  kList,
  kItem
};

constexpr char kOrderStorageKey[] = "playlist-order";

std::unique_ptr<syncer::ClientTagBasedModelTypeProcessor> CreateProcessor() {
  return std::make_unique<syncer::ClientTagBasedModelTypeProcessor>(syncer::PLAYLIST,
                                                                    base::BindRepeating([]() {
    LOG(ERROR) << "playlist sync error!";
  }));
}

}  // namespace

PlaylistSyncBridge::PlaylistSyncBridge(Delegate* delegate, syncer::OnceModelTypeStoreFactory create_store_callback)
    : ModelTypeSyncBridge(CreateProcessor()),
      delegate_(delegate) {
  std::move(create_store_callback).Run(syncer::PLAYLIST, base::BindOnce(&PlaylistSyncBridge::OnStoreCreated, weak_ptr_factory_.GetWeakPtr()));
}

PlaylistSyncBridge::~PlaylistSyncBridge() = default;

void PlaylistSyncBridge::OnStoreCreated(const absl::optional<ModelError>& error, std::unique_ptr<ModelTypeStore> store) {
  if (error) {
    change_processor()->ReportError(*error);
    return;
  }
  store_ = std::move(store);
  store_->ReadAllData(base::BindOnce(&PlaylistSyncBridge::OnReadAllData, weak_ptr_factory_.GetWeakPtr()));
}

void PlaylistSyncBridge::OnReadAllData(const absl::optional<ModelError>& error, std::unique_ptr<ModelTypeStore::RecordList> records) {
  if (error) {
    change_processor()->ReportError(*error);
    return;
  }
  CHECK(records);

  absl::optional<ModelError> cache_result;
  {
    base::AutoLock lock(data_lock_);
    for (const auto& record : *records) {
      sync_pb::PlaylistSpecifics specifics;
      if (!specifics.ParseFromString(record.value)) {
        cache_result = {FROM_HERE, "Failed to deserialize stored specifics."};
        continue;
      }
      CacheSpecifics(record.id, specifics);
    }
  }

  if (cache_result) {
    change_processor()->ReportError(*cache_result);
    return;
  }
  
  store_->ReadAllMetadata(base::BindOnce(&PlaylistSyncBridge::OnReadAllMetadata, weak_ptr_factory_.GetWeakPtr()));
}

void PlaylistSyncBridge::OnReadAllMetadata(const absl::optional<ModelError>& error, std::unique_ptr<syncer::MetadataBatch> metadata_batch) {
  if (error) {
    change_processor()->ReportError(*error);
    return;
  }
  change_processor()->ModelReadyToSync(std::move(metadata_batch));
  ready_ = true;
  delegate_->OnDataReady();
}

void PlaylistSyncBridge::OnCommit(const absl::optional<ModelError>& error) {
  if (error) {
    change_processor()->ReportError(*error);
  }
}

bool PlaylistSyncBridge::CacheSpecifics(const std::string& key, const sync_pb::PlaylistSpecifics& specifics) {
  data_lock_.AssertAcquired();
  switch (specifics.details_case()) {
    case sync_pb::PlaylistSpecifics::DetailsCase::kOrderListDetails:
      playlist_order_ = specifics.order_list_details();
      break;
    case sync_pb::PlaylistSpecifics::DetailsCase::kListDetails:
      playlists_[key] = specifics.list_details();
      break;
    case sync_pb::PlaylistSpecifics::DetailsCase::kItemDetails:
      items_[key] = specifics.item_details();
      break;
    case sync_pb::PlaylistSpecifics::DetailsCase::DETAILS_NOT_SET:
      return false;
  }
  return true;
}

bool PlaylistSyncBridge::SaveSpecifics(const std::string& key, const sync_pb::PlaylistSpecifics& specifics, ModelTypeStore::WriteBatch* batch) {
  if (!CacheSpecifics(key, specifics)) {
    return false;
  }
  std::string specifics_str;
  if (!specifics.SerializeToString(&specifics_str)) {
    return false;
  }
  batch->WriteData(key, specifics_str);
  return true;
}

bool PlaylistSyncBridge::DeleteSpecifics(const std::string& key, ModelTypeStore::WriteBatch* batch) {
  data_lock_.AssertAcquired();
  if (key == kOrderStorageKey) {
    playlist_order_ = absl::nullopt;
  }
  playlists_.erase(key);
  items_.erase(key);
  batch->DeleteData(key);
  return true;
}

absl::optional<sync_pb::PlaylistSpecifics> PlaylistSyncBridge::GetStoredSpecifics(const std::string& key) const {
  data_lock_.AssertAcquired();
  sync_pb::PlaylistSpecifics specifics;
  if (key == kOrderStorageKey && playlist_order_) {
    *specifics.mutable_order_list_details() = *playlist_order_;
  } else if (items_.contains(key)) {
    *specifics.mutable_item_details() = items_.at(key);
  } else if (playlists_.contains(key)) {
    *specifics.mutable_list_details() = playlists_.at(key);
  } else {
    return absl::nullopt;
  }
  return specifics;
}

std::unique_ptr<EntityData> PlaylistSyncBridge::CreateEntityData(const sync_pb::PlaylistSpecifics& specifics) const {
  auto result = std::make_unique<EntityData>();
  *result->specifics.mutable_playlist() = specifics;
  return result;
}

std::vector<std::string> PlaylistSyncBridge::GetAllKeys() const {
  data_lock_.AssertAcquired();
  std::vector<std::string> keys;
  std::transform(items_.cbegin(), items_.cend(), std::back_inserter(keys), [](const auto& entry) {
    return entry.first;
  });
  std::transform(playlists_.cbegin(), playlists_.cend(), std::back_inserter(keys), [](const auto& entry) {
    return entry.first;
  });
  return keys;
}

void PlaylistSyncBridge::CommitBatch(std::unique_ptr<syncer::ModelTypeStore::WriteBatch> batch) {
  store_->CommitWriteBatch(std::move(batch), base::BindOnce(&PlaylistSyncBridge::OnCommit, weak_ptr_factory_.GetWeakPtr()));
}

std::unique_ptr<MetadataChangeList> PlaylistSyncBridge::CreateMetadataChangeList() {
  return ModelTypeStore::WriteBatch::CreateMetadataChangeList();
}

absl::optional<ModelError> PlaylistSyncBridge::MergeFullSyncData(
      std::unique_ptr<MetadataChangeList> metadata_change_list,
      EntityChangeList entity_data) {
  absl::optional<ModelError> apply_result;
  auto batch = store_->CreateWriteBatch();
  base::flat_set<std::string> remote_synced_keys;
  {
    base::AutoLock lock(data_lock_);

    // Receive updates from sync engine
    for (const auto& change : entity_data) {
      if (!change->data().specifics.has_playlist()) {
        apply_result = ModelError({FROM_HERE, "Failed to receive full sync update due to unrecognized specifics."});
        continue;
      }
      if (!SaveSpecifics(change->storage_key(), change->data().specifics.playlist(), batch.get())) {
        apply_result = ModelError({FROM_HERE, "Failed to save recevied full sync update."});
        continue;
      }
      remote_synced_keys.insert(change->storage_key());
    }

    // Send updates to sync engine
    auto keys = GetAllKeys();
    for (const auto& key : keys) {
      if (!remote_synced_keys.contains(key)) {
        // Skip notifying sync of any entities that we just received
        continue;
      }

      auto specifics = GetStoredSpecifics(key);
      if (specifics) {
        change_processor()->Put(key, CreateEntityData(*specifics), metadata_change_list.get());
      }
    }
  }

  batch->TakeMetadataChangesFrom(std::move(metadata_change_list));
  CommitBatch(std::move(batch));

  return apply_result;
}

absl::optional<ModelError> PlaylistSyncBridge::ApplyIncrementalSyncChanges(
    std::unique_ptr<MetadataChangeList> metadata_change_list,
    EntityChangeList entity_changes) {
  absl::optional<ModelError> apply_result;
  auto batch = store_->CreateWriteBatch();
  {
    base::AutoLock lock(data_lock_);
    for (const auto& change : entity_changes) {
      if (!change->data().specifics.has_playlist()) {
        apply_result = ModelError({FROM_HERE, "Failed to receive incremental sync update due to unrecognized specifics."});
        continue;
      }
      const auto& key = change->storage_key();
      if (change->type() == EntityChange::ChangeType::ACTION_DELETE) {
        if (!DeleteSpecifics(key, batch.get())) {
          apply_result = ModelError({FROM_HERE, "Failed to delete recevied incremental sync update."});
        }
      } else {
        const auto& specifics = change->data().specifics.playlist();
        if (!SaveSpecifics(key, specifics, batch.get())) {
          apply_result = ModelError({FROM_HERE, "Failed to save recevied incremental sync update."});
        }
      }
    }
  }
  batch->TakeMetadataChangesFrom(std::move(metadata_change_list));
  CommitBatch(std::move(batch));

  return apply_result;
}

void PlaylistSyncBridge::GetData(StorageKeyList storage_keys, DataCallback callback) {
  auto batch = std::make_unique<syncer::MutableDataBatch>();
  {
    base::AutoLock lock(data_lock_);
    for (const auto& key : storage_keys) {
      auto specifics = GetStoredSpecifics(key);
      if (!specifics) {
        continue;
      }
      batch->Put(key, CreateEntityData(*specifics));
    }
  }
  std::move(callback).Run(std::move(batch));
}

void PlaylistSyncBridge::GetAllDataForDebugging(DataCallback callback) {
  auto batch = std::make_unique<syncer::MutableDataBatch>();
  {
    base::AutoLock lock(data_lock_);
    auto keys = GetAllKeys();
    for (const auto& key : keys) {
      batch->Put(key, CreateEntityData(*GetStoredSpecifics(key)));
    }
  }
  std::move(callback).Run(std::move(batch));
}

std::string PlaylistSyncBridge::GetClientTag(const syncer::EntityData& entity_data) {
  return GetStorageKey(entity_data);
}

std::string PlaylistSyncBridge::GetStorageKey(const syncer::EntityData& entity_data) {
  CHECK(entity_data.specifics.has_playlist());
  switch (entity_data.specifics.playlist().details_case()) {
    case sync_pb::PlaylistSpecifics::DetailsCase::kOrderListDetails:
      return kOrderStorageKey;
    case sync_pb::PlaylistSpecifics::DetailsCase::kListDetails:
      return entity_data.specifics.playlist().list_details().id();
    case sync_pb::PlaylistSpecifics::DetailsCase::kItemDetails:
      return entity_data.specifics.playlist().item_details().id();
    case sync_pb::PlaylistSpecifics::DetailsCase::DETAILS_NOT_SET:
      NOTREACHED();
  }
  return std::string();
}

bool PlaylistSyncBridge::IsEntityDataValid(const syncer::EntityData& entity_data) const {
  return entity_data.specifics.has_playlist() &&
      entity_data.specifics.playlist().details_case() != sync_pb::PlaylistSpecifics::DetailsCase::DETAILS_NOT_SET;
}

std::vector<sync_pb::PlaylistDetails> PlaylistSyncBridge::GetAllPlaylists() const {
  base::AutoLock lock(data_lock_);
  std::vector<sync_pb::PlaylistDetails> result;

  std::transform(playlists_.cbegin(), playlists_.cend(), std::back_inserter(result), [](const auto& entry) {
    return entry.second;
  });

  return result;
}

absl::optional<sync_pb::PlaylistDetails> PlaylistSyncBridge::GetPlaylistDetails(const std::string& id) const {
  base::AutoLock lock(data_lock_);
  return playlists_.contains(id) ? absl::optional<sync_pb::PlaylistDetails>(playlists_.at(id)) : absl::nullopt;
}

void PlaylistSyncBridge::SavePlaylistDetails(const sync_pb::PlaylistDetails& playlist) {
  base::AutoLock lock(data_lock_);

  auto batch = store_->CreateWriteBatch();

  sync_pb::PlaylistSpecifics specifics;
  *specifics.mutable_list_details() = playlist;

  SaveSpecifics(playlist.id(), specifics, batch.get());
  change_processor()->Put(playlist.id(), CreateEntityData(specifics), batch->GetMetadataChangeList());

  CommitBatch(std::move(batch));
}

void PlaylistSyncBridge::DeletePlaylistDetails(const std::string& id) {
  base::AutoLock lock(data_lock_);

  auto batch = store_->CreateWriteBatch();

  DeleteSpecifics(id, batch.get());
  change_processor()->Delete(id, batch->GetMetadataChangeList());

  CommitBatch(std::move(batch));
}

absl::optional<sync_pb::PlaylistOrderDetails> PlaylistSyncBridge::GetOrderDetails() const {
  base::AutoLock lock(data_lock_);
  return playlist_order_;
}

void PlaylistSyncBridge::SaveOrderDetails(const sync_pb::PlaylistOrderDetails& playlist_order) {
  base::AutoLock lock(data_lock_);

  auto batch = store_->CreateWriteBatch();

  sync_pb::PlaylistSpecifics specifics;
  *specifics.mutable_order_list_details() = playlist_order;

  SaveSpecifics(kOrderStorageKey, specifics, batch.get());
  change_processor()->Put(kOrderStorageKey, CreateEntityData(specifics), batch->GetMetadataChangeList());

  CommitBatch(std::move(batch));
}

std::vector<sync_pb::PlaylistItemDetails> PlaylistSyncBridge::GetItemDetailsForPlaylist(const std::string& playlist_id) const {
  base::AutoLock lock(data_lock_);
  std::vector<sync_pb::PlaylistItemDetails> result;

  if (!playlists_.contains(playlist_id)) {
    return result;
  }

  for (const auto& item_id : playlists_.at(playlist_id).playlist_item_ids()) {
    if (!items_.contains(item_id)) {
      continue;
    }
    result.emplace_back(items_.at(item_id));
  }

  return result;
}

std::vector<sync_pb::PlaylistItemDetails> PlaylistSyncBridge::GetAllItemDetails() const {
  base::AutoLock lock(data_lock_);
  std::vector<sync_pb::PlaylistItemDetails> result;

  std::transform(items_.cbegin(), items_.cend(), std::back_inserter(result), [](const auto& entry) {
    return entry.second;
  });

  return result;
}

absl::optional<sync_pb::PlaylistItemDetails> PlaylistSyncBridge::GetItemDetails(const std::string& id) const {
  base::AutoLock lock(data_lock_);
  return items_.contains(id) ? absl::optional<sync_pb::PlaylistItemDetails>(items_.at(id)) : absl::nullopt;
}

void PlaylistSyncBridge::SaveItemDetails(const sync_pb::PlaylistItemDetails& item) {
  base::AutoLock lock(data_lock_);

  auto batch = store_->CreateWriteBatch();

  sync_pb::PlaylistSpecifics specifics;
  *specifics.mutable_item_details() = item;

  SaveSpecifics(item.id(), specifics, batch.get());
  change_processor()->Put(item.id(), CreateEntityData(specifics), batch->GetMetadataChangeList());

  CommitBatch(std::move(batch));
}

void PlaylistSyncBridge::DeleteItemDetails(const std::string& id) {
  base::AutoLock lock(data_lock_);

  auto batch = store_->CreateWriteBatch();

  DeleteSpecifics(id, batch.get());
  change_processor()->Delete(id, batch->GetMetadataChangeList());

  CommitBatch(std::move(batch));
}

}  // namespace sync
}  // namespace playlist