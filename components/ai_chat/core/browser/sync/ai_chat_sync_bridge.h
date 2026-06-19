/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_SYNC_AI_CHAT_SYNC_BRIDGE_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_SYNC_AI_CHAT_SYNC_BRIDGE_H_

#include <memory>
#include <optional>
#include <string>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/sequence_checker.h"
#include "components/sync/model/data_type_sync_bridge.h"

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
class AIChatSyncBridge : public syncer::DataTypeSyncBridge {
 public:
  // |database| must outlive this bridge and live on the same sequence.
  AIChatSyncBridge(
      std::unique_ptr<syncer::DataTypeLocalChangeProcessor> change_processor,
      AIChatDatabase* database);
  AIChatSyncBridge(const AIChatSyncBridge&) = delete;
  AIChatSyncBridge& operator=(const AIChatSyncBridge&) = delete;
  ~AIChatSyncBridge() override;

  // syncer::DataTypeSyncBridge:
  std::unique_ptr<syncer::MetadataChangeList> CreateMetadataChangeList()
      override;
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
  // Emits a Put for the conversation metadata of |uuid|. No-op for temporary
  // or unknown conversations.
  void PutConversationMetadata(const std::string& uuid);
  // Emits a Put for the single entry |entry_uuid| belonging to
  // |conversation_uuid|. No-op when the conversation is temporary or unknown.
  void PutEntry(const std::string& conversation_uuid,
                const std::string& entry_uuid);

  const raw_ptr<AIChatDatabase> database_;

  SEQUENCE_CHECKER(sequence_checker_);

  base::WeakPtrFactory<AIChatSyncBridge> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_SYNC_AI_CHAT_SYNC_BRIDGE_H_
