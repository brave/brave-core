// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_news/browser/brave_news_sync_bridge.h"

#include <array>
#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "base/auto_reset.h"
#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/values.h"
#include "brave/components/brave_news/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "components/sync/base/data_type.h"
#include "components/sync/model/client_tag_based_data_type_processor.h"
#include "components/sync/model/data_type_store.h"
#include "components/sync/model/metadata_batch.h"
#include "components/sync/model/metadata_change_list.h"
#include "components/sync/model/mutable_data_batch.h"
#include "components/sync/protocol/entity_data.h"

namespace brave_news {

namespace {

enum class PrefType { kBool, kDict };

struct SyncedPref {
  std::string_view name;
  PrefType type;
};

// The single source of truth for which Brave News prefs are synced and how each
// is encoded into BraveNewsSpecifics.
constexpr auto kSyncedPrefs = std::to_array<SyncedPref>({
    {prefs::kBraveNewsSources, PrefType::kDict},
    {prefs::kBraveNewsChannels, PrefType::kDict},
    {prefs::kBraveNewsDirectFeeds, PrefType::kDict},
    {prefs::kShouldShowToolbarButton, PrefType::kBool},
    {prefs::kBraveNewsOpenArticlesInNewTab, PrefType::kBool},
});

std::optional<PrefType> GetPrefType(std::string_view name) {
  for (const auto& pref : kSyncedPrefs) {
    if (pref.name == name) {
      return pref.type;
    }
  }
  return std::nullopt;
}

std::unique_ptr<syncer::EntityData> CreateEntityData(
    const sync_pb::BraveNewsSpecifics& specifics) {
  auto entity = std::make_unique<syncer::EntityData>();
  entity->name = specifics.name();
  entity->specifics.mutable_brave_news()->CopyFrom(specifics);
  return entity;
}

}  // namespace

BraveNewsSyncBridge::BraveNewsSyncBridge(
    PrefService* prefs,
    std::unique_ptr<syncer::DataTypeLocalChangeProcessor> change_processor,
    syncer::OnceDataTypeStoreFactory store_factory)
    : DataTypeSyncBridge(std::move(change_processor)), prefs_(prefs) {
  pref_change_registrar_.Init(prefs_);
  for (const auto& pref : kSyncedPrefs) {
    pref_change_registrar_.Add(
        std::string(pref.name),
        base::BindRepeating(&BraveNewsSyncBridge::OnPrefChanged,
                            base::Unretained(this), std::string(pref.name)));
  }
  std::move(store_factory)
      .Run(syncer::BRAVE_NEWS,
           base::BindOnce(&BraveNewsSyncBridge::OnStoreCreated,
                          weak_factory_.GetWeakPtr()));
}

BraveNewsSyncBridge::~BraveNewsSyncBridge() = default;

// static
std::unique_ptr<BraveNewsSyncBridge> BraveNewsSyncBridge::CreateBridge(
    PrefService* prefs,
    syncer::OnceDataTypeStoreFactory store_factory) {
  return std::make_unique<BraveNewsSyncBridge>(
      prefs,
      std::make_unique<syncer::ClientTagBasedDataTypeProcessor>(
          syncer::BRAVE_NEWS, /*dump_stack=*/base::DoNothing()),
      std::move(store_factory));
}

sync_pb::BraveNewsSpecifics BraveNewsSyncBridge::SpecificsFromPref(
    std::string_view pref_name) const {
  sync_pb::BraveNewsSpecifics specifics;
  specifics.set_name(std::string(pref_name));
  const std::optional<PrefType> type = GetPrefType(pref_name);
  CHECK(type);
  switch (*type) {
    case PrefType::kBool:
      specifics.set_bool_value(prefs_->GetBoolean(pref_name));
      break;
    case PrefType::kDict:
      if (std::optional<std::string> json =
              base::WriteJson(prefs_->GetDict(pref_name))) {
        specifics.set_dict_value(std::move(*json));
      } else {
        specifics.set_dict_value("{}");
      }
      break;
  }
  return specifics;
}

void BraveNewsSyncBridge::ApplySpecificsToPref(
    const sync_pb::BraveNewsSpecifics& specifics) {
  const std::string& name = specifics.name();
  const std::optional<PrefType> type = GetPrefType(name);
  if (!type) {
    return;
  }
  // Guard so the resulting pref change isn't echoed back out as a local change.
  base::AutoReset<bool> applying(&applying_remote_changes_, true);
  switch (*type) {
    case PrefType::kBool:
      if (specifics.has_bool_value() &&
          prefs_->GetBoolean(name) != specifics.bool_value()) {
        prefs_->SetBoolean(name, specifics.bool_value());
      }
      break;
    case PrefType::kDict:
      if (specifics.has_dict_value()) {
        std::optional<base::DictValue> dict = base::JSONReader::ReadDict(
            specifics.dict_value(), base::JSON_PARSE_RFC);
        if (dict.has_value() && *dict != prefs_->GetDict(name)) {
          prefs_->SetDict(name, std::move(*dict));
        }
      }
      break;
  }
}

void BraveNewsSyncBridge::PutPref(std::string_view pref_name,
                                  syncer::DataTypeStore::WriteBatch* batch) {
  sync_pb::BraveNewsSpecifics specifics = SpecificsFromPref(pref_name);
  const std::string storage_key(pref_name);
  const std::string serialized = specifics.SerializeAsString();
  change_processor()->Put(storage_key, CreateEntityData(specifics),
                          batch->GetMetadataChangeList());
  batch->WriteData(storage_key, serialized);
  last_synced_[storage_key] = serialized;
}

void BraveNewsSyncBridge::OnPrefChanged(const std::string& pref_name) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (applying_remote_changes_ || !store_ ||
      !change_processor()->IsTrackingMetadata()) {
    return;
  }
  const std::string serialized =
      SpecificsFromPref(pref_name).SerializeAsString();
  // Skip if nothing actually changed relative to the last synced snapshot.
  auto it = last_synced_.find(pref_name);
  if (it != last_synced_.end() && it->second == serialized) {
    return;
  }
  std::unique_ptr<syncer::DataTypeStore::WriteBatch> batch =
      store_->CreateWriteBatch(CreateMetadataChangeList());
  PutPref(pref_name, batch.get());
  store_->CommitWriteBatch(
      std::move(batch),
      base::BindOnce(&BraveNewsSyncBridge::ReportErrorIfSet,
                     weak_factory_.GetWeakPtr()));
}

std::optional<syncer::ModelError> BraveNewsSyncBridge::MergeFullSyncData(
    std::unique_ptr<syncer::MetadataChangeList> metadata_change_list,
    syncer::EntityChangeList entity_data) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  std::unique_ptr<syncer::DataTypeStore::WriteBatch> batch =
      store_->CreateWriteBatch(std::move(metadata_change_list));

  // Apply all remote entities to prefs (remote wins on the initial merge).
  for (const std::unique_ptr<syncer::EntityChange>& change : entity_data) {
    const sync_pb::BraveNewsSpecifics& specifics =
        change->data().specifics.brave_news();
    ApplySpecificsToPref(specifics);
    const std::string serialized = specifics.SerializeAsString();
    batch->WriteData(change->storage_key(), serialized);
    last_synced_[change->storage_key()] = serialized;
  }

  // Upload any synced pref that has no corresponding remote entity, so a
  // device's pre-existing Brave News configuration isn't lost on first sync.
  for (const auto& pref : kSyncedPrefs) {
    if (!last_synced_.contains(std::string(pref.name))) {
      PutPref(pref.name, batch.get());
    }
  }

  store_->CommitWriteBatch(
      std::move(batch),
      base::BindOnce(&BraveNewsSyncBridge::ReportErrorIfSet,
                     weak_factory_.GetWeakPtr()));
  return std::nullopt;
}

std::optional<syncer::ModelError>
BraveNewsSyncBridge::ApplyIncrementalSyncChanges(
    std::unique_ptr<syncer::MetadataChangeList> metadata_change_list,
    syncer::EntityChangeList entity_changes) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  std::unique_ptr<syncer::DataTypeStore::WriteBatch> batch =
      store_->CreateWriteBatch(std::move(metadata_change_list));
  for (const std::unique_ptr<syncer::EntityChange>& change : entity_changes) {
    switch (change->type()) {
      case syncer::EntityChange::ACTION_ADD:
      case syncer::EntityChange::ACTION_UPDATE: {
        const sync_pb::BraveNewsSpecifics& specifics =
            change->data().specifics.brave_news();
        ApplySpecificsToPref(specifics);
        const std::string serialized = specifics.SerializeAsString();
        batch->WriteData(change->storage_key(), serialized);
        last_synced_[change->storage_key()] = serialized;
        break;
      }
      case syncer::EntityChange::ACTION_DELETE: {
        // A tombstone resets the pref to its default value.
        if (GetPrefType(change->storage_key())) {
          base::AutoReset<bool> applying(&applying_remote_changes_, true);
          prefs_->ClearPref(change->storage_key());
        }
        batch->DeleteData(change->storage_key());
        last_synced_.erase(change->storage_key());
        break;
      }
    }
  }
  store_->CommitWriteBatch(
      std::move(batch),
      base::BindOnce(&BraveNewsSyncBridge::ReportErrorIfSet,
                     weak_factory_.GetWeakPtr()));
  return std::nullopt;
}

void BraveNewsSyncBridge::ApplyDisableSyncChanges(
    std::unique_ptr<syncer::MetadataChangeList> delete_metadata_change_list) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // Note: local prefs are intentionally left intact. Disabling sync must not
  // wipe the user's local Brave News configuration.
  store_->DeleteAllDataAndMetadata(
      std::move(delete_metadata_change_list),
      base::BindOnce(&BraveNewsSyncBridge::ReportErrorIfSet,
                     weak_factory_.GetWeakPtr()));
  last_synced_.clear();
}

std::unique_ptr<syncer::DataBatch> BraveNewsSyncBridge::GetDataForCommit(
    StorageKeyList storage_keys) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  auto batch = std::make_unique<syncer::MutableDataBatch>();
  for (const std::string& key : storage_keys) {
    if (GetPrefType(key)) {
      batch->Put(key, CreateEntityData(SpecificsFromPref(key)));
    }
  }
  return batch;
}

std::unique_ptr<syncer::DataBatch>
BraveNewsSyncBridge::GetAllDataForDebugging() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  auto batch = std::make_unique<syncer::MutableDataBatch>();
  for (const auto& pref : kSyncedPrefs) {
    batch->Put(std::string(pref.name),
               CreateEntityData(SpecificsFromPref(pref.name)));
  }
  return batch;
}

bool BraveNewsSyncBridge::IsEntityDataValid(
    const syncer::EntityData& entity_data) const {
  if (!entity_data.specifics.has_brave_news()) {
    return false;
  }
  const sync_pb::BraveNewsSpecifics& specifics =
      entity_data.specifics.brave_news();
  return specifics.has_name() && GetPrefType(specifics.name()).has_value();
}

sync_pb::EntitySpecifics
BraveNewsSyncBridge::TrimAllSupportedFieldsFromRemoteSpecifics(
    const sync_pb::EntitySpecifics& entity_specifics) const {
  // Nothing to preserve: the bridge round-trips the full specifics.
  return sync_pb::EntitySpecifics();
}

std::string BraveNewsSyncBridge::GetClientTag(
    const syncer::EntityData& entity_data) const {
  return GetStorageKey(entity_data);
}

std::string BraveNewsSyncBridge::GetStorageKey(
    const syncer::EntityData& entity_data) const {
  return entity_data.specifics.brave_news().name();
}

void BraveNewsSyncBridge::OnStoreCreated(
    const std::optional<syncer::ModelError>& error,
    std::unique_ptr<syncer::DataTypeStore> store) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (error) {
    change_processor()->ReportError(*error);
    return;
  }
  store_ = std::move(store);
  store_->ReadAllDataAndMetadata(
      base::BindOnce(&BraveNewsSyncBridge::OnReadAllDataAndMetadata,
                     weak_factory_.GetWeakPtr()));
}

void BraveNewsSyncBridge::OnReadAllDataAndMetadata(
    const std::optional<syncer::ModelError>& error,
    std::unique_ptr<syncer::DataTypeStore::RecordList> records,
    std::unique_ptr<syncer::MetadataBatch> metadata_batch) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (error) {
    change_processor()->ReportError(*error);
    return;
  }
  // Load the last-synced snapshot. Unparseable records are simply dropped; the
  // pref still holds the authoritative value.
  for (const syncer::DataTypeStore::Record& record : *records) {
    if (GetPrefType(record.id)) {
      last_synced_[record.id] = record.value;
    }
  }
  change_processor()->ModelReadyToSync(std::move(metadata_batch));

  // Reconcile local changes made while the engine was down: if a pref now
  // differs from the snapshot, upload it.
  if (!change_processor()->IsTrackingMetadata()) {
    return;
  }
  std::unique_ptr<syncer::DataTypeStore::WriteBatch> batch =
      store_->CreateWriteBatch(CreateMetadataChangeList());
  bool dirty = false;
  for (const auto& pref : kSyncedPrefs) {
    const std::string serialized =
        SpecificsFromPref(pref.name).SerializeAsString();
    auto it = last_synced_.find(std::string(pref.name));
    if (it == last_synced_.end() || it->second != serialized) {
      PutPref(pref.name, batch.get());
      dirty = true;
    }
  }
  if (dirty) {
    store_->CommitWriteBatch(
        std::move(batch),
        base::BindOnce(&BraveNewsSyncBridge::ReportErrorIfSet,
                       weak_factory_.GetWeakPtr()));
  }
}

void BraveNewsSyncBridge::ReportErrorIfSet(
    const std::optional<syncer::ModelError>& error) {
  if (error) {
    change_processor()->ReportError(*error);
  }
}

}  // namespace brave_news
