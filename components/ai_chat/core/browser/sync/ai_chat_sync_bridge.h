/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_SYNC_AI_CHAT_SYNC_BRIDGE_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_SYNC_AI_CHAT_SYNC_BRIDGE_H_

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "base/containers/flat_set.h"
#include "base/functional/callback.h"
#include "base/functional/function_ref.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/sequence_checker.h"
#include "components/sync/model/data_type_sync_bridge.h"
#include "third_party/abseil-cpp/absl/container/flat_hash_set.h"

namespace sync_pb {
class AIChatConversationSpecifics;
}  // namespace sync_pb

namespace syncer {
class SyncMetadataStoreChangeList;
}  // namespace syncer

namespace ai_chat {

class AIChatDatabase;

// Bridge between the AI Chat conversation database and the Chromium Sync
// engine. Sync stores two record kinds per conversation:
//   * one Conversation metadata record (small) keyed "c:<conversation-uuid>",
//   * one Entry record per conversation turn, keyed "e:<entry-uuid>".
// Splitting per turn keeps individual entities under the per-record server
// size limit even for long conversations.
//
// Lives on the same background sequence as AIChatDatabase for direct
// synchronous database access. The controller delegate is exposed to the
// UI thread via ProxyDataTypeControllerDelegate.
//
// The bridge (and its change processor) is created eagerly — before the
// database exists — so the sync engine always has a real delegate to talk to.
// The database is attached later, once the os_crypt encryptor is ready, via
// SetDatabase(); it is detached again via ClearDatabase() whenever on-disk
// storage is turned off (the database is destroyed then, but the bridge and
// its processor live on so the sync start handshake survives storage toggles).
class AIChatSyncBridge : public syncer::DataTypeSyncBridge {
 public:
  // Reports the conversations a batch of remote changes touched: each one a
  // record was written to, and each one a tombstone named, even if it was
  // already gone. The list is never empty.
  using RemoteChangesAppliedCallback = base::RepeatingCallback<void(
      std::vector<std::string> conversation_uuids)>;

  // |on_remote_changes_applied| runs on this sequence once per batch that
  // touched a conversation, naming only those: a listener that reloaded any
  // other conversation would discard state that conversation hasn't saved
  // yet.
  AIChatSyncBridge(
      std::unique_ptr<syncer::DataTypeLocalChangeProcessor> change_processor,
      RemoteChangesAppliedCallback on_remote_changes_applied);
  AIChatSyncBridge(const AIChatSyncBridge&) = delete;
  AIChatSyncBridge& operator=(const AIChatSyncBridge&) = delete;
  ~AIChatSyncBridge() override;

  // Attaches the database (which must outlive the bridge or be detached first
  // via ClearDatabase(), and live on the same sequence). On the first attach
  // this also loads sync metadata and reports the model ready to the change
  // processor; subsequent attaches just re-point at the new database.
  void SetDatabase(AIChatDatabase* database);

  // Detaches the database. Called before the database is destroyed (e.g. when
  // on-disk storage is disabled) so |database_| never dangles.
  void ClearDatabase();

  // syncer::DataTypeSyncBridge:
  std::optional<syncer::ModelError> MergeFullSyncData(
      std::unique_ptr<syncer::MetadataChangeList> metadata_change_list,
      syncer::EntityChangeList entity_data) override;
  std::optional<syncer::ModelError> ApplyIncrementalSyncChanges(
      std::unique_ptr<syncer::MetadataChangeList> metadata_change_list,
      syncer::EntityChangeList entity_changes) override;
  std::unique_ptr<syncer::DataBatch> GetDataForCommit(
      StorageKeyList storage_keys) override;
  std::unique_ptr<syncer::DataBatch> GetAllDataForDebugging() override;
  std::string GetClientTag(
      const syncer::EntityData& entity_data) const override;
  std::string GetStorageKey(
      const syncer::EntityData& entity_data) const override;
  bool IsEntityDataValid(const syncer::EntityData& entity_data) const override;
  sync_pb::EntitySpecifics TrimAllSupportedFieldsFromRemoteSpecifics(
      const sync_pb::EntitySpecifics& entity_specifics) const override;
  void ApplyDisableSyncChanges(std::unique_ptr<syncer::MetadataChangeList>
                                   delete_metadata_change_list) override;

  // Conversation-metadata notifications. Called on this sequence by the
  // service when the conversation row changes (title, model, tokens, ...).
  void OnConversationAdded(const std::string& uuid);
  void OnConversationModified(const std::string& uuid);
  // Must be invoked while entries are still readable from the database, so
  // the bridge can emit a Delete for every Entry storage key before the
  // conversation row and its child rows are removed.
  void OnConversationDeleted(const std::string& uuid);

  // Entry-level notifications. |conversation_uuid| is the parent.
  void OnConversationEntryAdded(const std::string& conversation_uuid,
                                const std::string& entry_uuid);
  void OnConversationEntryModified(const std::string& conversation_uuid,
                                   const std::string& entry_uuid);
  void OnConversationEntryDeleted(const std::string& entry_uuid);

  base::WeakPtr<AIChatSyncBridge> GetWeakPtr() {
    return weak_ptr_factory_.GetWeakPtr();
  }

 private:
  // Forwards a sync model error to the change processor. Bound as a WeakPtr
  // callback for the metadata store's error reporting.
  void ReportError(const syncer::ModelError& error);

  // Iterate through all sync-allowed entities in the local database, invoking
  // |emit| for each one that additionally satisfies |should_include|.
  void ForEachLocalEntity(
      base::FunctionRef<bool(const std::string&)> should_include,
      base::FunctionRef<void(std::string, std::unique_ptr<syncer::EntityData>)>
          emit);

  // Emits a Put for the conversation metadata of |uuid|. No-op for temporary
  // or unknown conversations.
  void PutConversationMetadata(const std::string& uuid);
  // Emits a Put for the single entry |entry_uuid| belonging to
  // |conversation_uuid|. No-op when the conversation is temporary or unknown.
  void PutEntry(const std::string& conversation_uuid,
                const std::string& entry_uuid);

  // Returns a change list that writes straight to the database's sync metadata
  // table. Requires a database.
  std::unique_ptr<syncer::SyncMetadataStoreChangeList>
  CreateMetadataStoreChangeList();

  // Saves the metadata changes the processor passed to MergeFullSyncData() or
  // ApplyIncrementalSyncChanges(). The processor records the batch's progress
  // marker and each record's new version before it calls the bridge, so they
  // are kept in memory (the default CreateMetadataChangeList()) and saved only
  // once the batch has been written. Otherwise a batch that failed would count
  // as received, and sync would never download it again.
  void ApplyMetadataChanges(
      std::unique_ptr<syncer::MetadataChangeList> metadata_change_list);

  // Writes |changes|, a batch from the server, to the database, and runs
  // |on_remote_changes_applied_| for the conversations it touched. Returns
  // false when a record fails to write. The batch's metadata changes are then
  // dropped, so it downloads again on the next start, and local updates to the
  // records from the failed one on are skipped for the rest of the session.
  [[nodiscard]] bool ApplyRemoteChanges(
      const syncer::EntityChangeList& changes,
      syncer::MetadataChangeList& metadata_change_list);

  // Whether a local update to |storage_key| must not reach the processor,
  // because a batch failed; see |storage_keys_skipping_local_puts_|.
  bool ShouldSkipLocalPut(const std::string& storage_key) const;

  // Writes |specifics|, an added or updated record from the server, to the
  // database, and inserts the conversation it wrote to into
  // |affected_conversation_uuids|. A record this device can't store because of
  // its own content (it doesn't decode, for example it comes from a newer
  // client, or its associated content would collide) is skipped: retrying it
  // would fail the same way every time. Returns false only when the database
  // fails to store a good record.
  [[nodiscard]] bool ApplyRemoteRecord(
      const sync_pb::AIChatConversationSpecifics& specifics,
      base::flat_set<std::string>& affected_conversation_uuids);

  // Deletes the record |storage_key| names, and inserts its conversation into
  // |affected_conversation_uuids|. Returns false only when the database fails
  // to delete.
  [[nodiscard]] bool ApplyRemoteTombstone(
      std::string_view storage_key,
      base::flat_set<std::string>& affected_conversation_uuids);

  // Runs |on_remote_changes_applied_| when |affected_conversation_uuids| isn't
  // empty. Also called when a batch fails part-way, since the records written
  // before the failure stay written.
  void NotifyRemoteChangesApplied(
      base::flat_set<std::string> affected_conversation_uuids);

  // Attached via SetDatabase()/ClearDatabase(); null before the first attach
  // and whenever on-disk storage is disabled.
  raw_ptr<AIChatDatabase> database_ = nullptr;

  // True once the initial metadata load + ModelReadyToSync() has run, so it is
  // done only for the first attached database.
  bool model_ready_to_sync_ = false;

  RemoteChangesAppliedCallback on_remote_changes_applied_;

  // The records a failed batch didn't write: the failing one and those after
  // it. The processor records a batch's new record versions in memory before
  // calling the bridge, and keeps them after the failure. A local update to
  // one of those records would save that version with its own metadata, so the
  // next start would take the remote change as already received and drop it.
  // Local updates to them are therefore skipped for the rest of the session,
  // and the next start applies the remote change over them. A local delete
  // still goes through: saving the tombstone at the batch's version is right,
  // since the delete came after the remote change.
  absl::flat_hash_set<std::string> storage_keys_skipping_local_puts_;

  // Set when a first-sync merge fails, to skip every local update for the
  // rest of the session. First sync runs again on the next start.
  bool skip_all_local_puts_ = false;

  SEQUENCE_CHECKER(sequence_checker_);

  base::WeakPtrFactory<AIChatSyncBridge> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_SYNC_AI_CHAT_SYNC_BRIDGE_H_
