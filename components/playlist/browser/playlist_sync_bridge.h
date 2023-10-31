/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PLAYLIST_BROWSER_SYNC_PLAYLIST_SYNC_BRIDGE_H_
#define BRAVE_COMPONENTS_PLAYLIST_BROWSER_SYNC_PLAYLIST_SYNC_BRIDGE_H_

#include <memory>

#include "base/containers/flat_map.h"
#include "base/containers/flat_set.h"
#include "base/memory/weak_ptr.h"
#include "base/synchronization/lock.h"
#include "brave/components/sync/protocol/playlist_specifics.pb.h"
#include "components/sync/model/model_type_store.h"
#include "components/sync/model/model_type_sync_bridge.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace playlist {
namespace sync {

// TODO: implement:
// - fixing relationships after applying changes (low priority, could ignore)
// - add migration of prefs to store after OnDataReady
// - test out any calls to playlistservice before data is ready
// - add delegate calls to propagate events to playlistservice observers
// - see if transport mode applies
// - enable encryption
// - for all the sync build assertions, replace the counts with CHROMIUM_MODEL_TYPE_COUNT + BRAVE_MODEL_TYPE_COUNT
// - handle conflicts correctly when updating PlaylistDetails. merge them accordingly
// - add multiple settings to global details
// - ensure p3a still works
class PlaylistSyncBridge : public syncer::ModelTypeSyncBridge {
 public:
  class Delegate {
   public:
    virtual void OnDataReady() = 0;
  };

  explicit PlaylistSyncBridge(Delegate* delegate, syncer::OnceModelTypeStoreFactory create_store_callback);
  ~PlaylistSyncBridge() override;

  // syncer::ModelTypeSyncBridge:
  std::unique_ptr<syncer::MetadataChangeList> CreateMetadataChangeList() override;

  absl::optional<syncer::ModelError> MergeFullSyncData(
      std::unique_ptr<syncer::MetadataChangeList> metadata_change_list,
      syncer::EntityChangeList entity_data) override;

  absl::optional<syncer::ModelError> ApplyIncrementalSyncChanges(
      std::unique_ptr<syncer::MetadataChangeList> metadata_change_list,
      syncer::EntityChangeList entity_changes) override;

  void GetData(StorageKeyList storage_keys, DataCallback callback) override;
  void GetAllDataForDebugging(DataCallback callback) override;

  std::string GetClientTag(const syncer::EntityData& entity_data) override;
  std::string GetStorageKey(const syncer::EntityData& entity_data) override;

  bool IsEntityDataValid(const syncer::EntityData& entity_data) const override;

  // Called by PlaylistService:
  absl::optional<sync_pb::PlaylistDetails> GetPlaylistDetails(const std::string& id) const;
  bool HasPlaylistDetails(const std::string& id) const;
  void SavePlaylistDetails(const sync_pb::PlaylistDetails& playlist);
  void DeletePlaylistDetails(const std::string& id);

  absl::optional<sync_pb::PlaylistGlobalDetails> GetGlobalDetails() const;
  void SaveGlobalDetails(const sync_pb::PlaylistGlobalDetails& global_details);

  std::vector<sync_pb::PlaylistItemDetails> GetItemDetailsForPlaylist(const std::string& playlist_id) const;
  std::vector<sync_pb::PlaylistItemDetails> GetAllItemDetails() const;
  absl::optional<sync_pb::PlaylistItemDetails> GetItemDetails(const std::string& id) const;
  bool HasItemDetails(const std::string& id) const;
  void SaveItemDetails(const sync_pb::PlaylistItemDetails& item);
  void DeleteItemDetails(const std::string& id);

  void ResetAll();

 private:
  void OnStoreCreated(const absl::optional<syncer::ModelError>& error, std::unique_ptr<syncer::ModelTypeStore> store);
  void OnReadAllData(const absl::optional<syncer::ModelError>& error, std::unique_ptr<syncer::ModelTypeStore::RecordList> records);
  void OnReadAllMetadata(const absl::optional<syncer::ModelError>& error, std::unique_ptr<syncer::MetadataBatch> metadata_batch);
  void OnCommit(const absl::optional<syncer::ModelError>& error);

  bool CacheSpecifics(const std::string& key, const sync_pb::PlaylistSpecifics& specifics);
  bool SaveSpecifics(const std::string& key, const sync_pb::PlaylistSpecifics& specifics, syncer::ModelTypeStore::WriteBatch* batch);
  bool DeleteSpecifics(const std::string& key, syncer::ModelTypeStore::WriteBatch* batch);

  absl::optional<sync_pb::PlaylistSpecifics> GetStoredSpecifics(const std::string& key) const;
  std::unique_ptr<syncer::EntityData> CreateEntityData(const sync_pb::PlaylistSpecifics& specifics) const;
  std::vector<std::string> GetAllKeys() const;
  void CommitBatch(std::unique_ptr<syncer::ModelTypeStore::WriteBatch> batch);

  absl::optional<sync_pb::PlaylistGlobalDetails> global_;
  base::flat_map<std::string, sync_pb::PlaylistDetails> playlists_;
  base::flat_map<std::string, sync_pb::PlaylistItemDetails> items_;

  bool ready_ = false;

  // Lock over |playlist_order_|, |playlists_|, |items_|.
  mutable base::Lock data_lock_;
   
  const raw_ptr<Delegate> delegate_;

  std::unique_ptr<syncer::ModelTypeStore> store_;

  base::WeakPtrFactory<PlaylistSyncBridge> weak_ptr_factory_{this};
};

}  // namespace sync
}  // namespace playlist

#endif  // BRAVE_COMPONENTS_PLAYLIST_BROWSER_SYNC_PLAYLIST_SYNC_BRIDGE_H_
