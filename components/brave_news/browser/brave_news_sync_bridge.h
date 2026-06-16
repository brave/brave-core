// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_BRAVE_NEWS_SYNC_BRIDGE_H_
#define BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_BRAVE_NEWS_SYNC_BRIDGE_H_

#include <memory>
#include <optional>
#include <string>
#include <string_view>

#include "base/containers/flat_map.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/sequence_checker.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/sync/model/data_type_store.h"
#include "components/sync/model/data_type_sync_bridge.h"
#include "brave/components/sync/protocol/brave_news_specifics.pb.h"

class PrefService;

namespace brave_news {

// Syncs a fixed set of Brave News preferences as their own sync DataType
// (syncer::BRAVE_NEWS), surfaced under the "Brave News" sync toggle.
//
// Unlike most USS bridges, the PrefService is the source of truth: each synced
// pref maps to one sync entity whose storage key / client tag is the pref name.
// The DataTypeStore persists only sync metadata plus a snapshot of the
// last-synced specifics (used to detect offline local changes). Remote updates
// are written back into prefs under an echo-loop guard so they don't bounce back
// out as local changes.
//
// Lives on the UI thread and is owned by `BraveNewsController`.
class BraveNewsSyncBridge : public syncer::DataTypeSyncBridge {
 public:
  BraveNewsSyncBridge(
      PrefService* prefs,
      std::unique_ptr<syncer::DataTypeLocalChangeProcessor> change_processor,
      syncer::OnceDataTypeStoreFactory store_factory);
  ~BraveNewsSyncBridge() override;

  BraveNewsSyncBridge(const BraveNewsSyncBridge&) = delete;
  BraveNewsSyncBridge& operator=(const BraveNewsSyncBridge&) = delete;

  // Builds a bridge with a real change processor for syncer::BRAVE_NEWS.
  static std::unique_ptr<BraveNewsSyncBridge> CreateBridge(
      PrefService* prefs,
      syncer::OnceDataTypeStoreFactory store_factory);

  // syncer::DataTypeSyncBridge:
  std::optional<syncer::ModelError> MergeFullSyncData(
      std::unique_ptr<syncer::MetadataChangeList> metadata_change_list,
      syncer::EntityChangeList entity_data) override;
  std::optional<syncer::ModelError> ApplyIncrementalSyncChanges(
      std::unique_ptr<syncer::MetadataChangeList> metadata_change_list,
      syncer::EntityChangeList entity_changes) override;
  void ApplyDisableSyncChanges(std::unique_ptr<syncer::MetadataChangeList>
                                   delete_metadata_change_list) override;
  std::unique_ptr<syncer::DataBatch> GetDataForCommit(
      StorageKeyList storage_keys) override;
  std::unique_ptr<syncer::DataBatch> GetAllDataForDebugging() override;
  bool IsEntityDataValid(const syncer::EntityData& entity_data) const override;
  sync_pb::EntitySpecifics TrimAllSupportedFieldsFromRemoteSpecifics(
      const sync_pb::EntitySpecifics& entity_specifics) const override;
  std::string GetClientTag(
      const syncer::EntityData& entity_data) const override;
  std::string GetStorageKey(
      const syncer::EntityData& entity_data) const override;

 private:
  // Builds the specifics representing the current value of `pref_name`.
  sync_pb::BraveNewsSpecifics SpecificsFromPref(
      std::string_view pref_name) const;
  // Writes `specifics` back into its pref (under `applying_remote_changes_`),
  // only if the value actually differs from the current pref value.
  void ApplySpecificsToPref(const sync_pb::BraveNewsSpecifics& specifics);

  // Uploads `pref_name`'s current value to sync and records the snapshot.
  void PutPref(std::string_view pref_name,
               syncer::DataTypeStore::WriteBatch* batch);

  // PrefChangeRegistrar callback: a synced pref changed locally.
  void OnPrefChanged(const std::string& pref_name);

  // Store callbacks.
  void OnStoreCreated(const std::optional<syncer::ModelError>& error,
                      std::unique_ptr<syncer::DataTypeStore> store);
  void OnReadAllDataAndMetadata(
      const std::optional<syncer::ModelError>& error,
      std::unique_ptr<syncer::DataTypeStore::RecordList> records,
      std::unique_ptr<syncer::MetadataBatch> metadata_batch);
  void ReportErrorIfSet(const std::optional<syncer::ModelError>& error);

  const raw_ptr<PrefService> prefs_;

  // Storage layer; asynchronously created. Non-null once creation succeeds.
  std::unique_ptr<syncer::DataTypeStore> store_;

  // Snapshot of the last-synced specifics per pref name (serialized), used to
  // detect offline local changes and to skip redundant commits.
  base::flat_map<std::string, std::string> last_synced_;

  // True while applying remote changes to prefs, so the PrefChangeRegistrar
  // callback skips re-uploading them.
  bool applying_remote_changes_ = false;

  PrefChangeRegistrar pref_change_registrar_;

  SEQUENCE_CHECKER(sequence_checker_);

  base::WeakPtrFactory<BraveNewsSyncBridge> weak_factory_{this};
};

}  // namespace brave_news

#endif  // BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_BRAVE_NEWS_SYNC_BRIDGE_H_
