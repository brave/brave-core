/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_SYNC_AI_CHAT_SYNC_BRIDGE_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_SYNC_AI_CHAT_SYNC_BRIDGE_H_

#include <map>
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
// engine. Each sync entity represents one conversation.
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
  void ApplyDisableSyncChanges(std::unique_ptr<syncer::MetadataChangeList>
                                   delete_metadata_change_list) override;

  // Called by AIChatService (posted to this sequence) when local data changes.
  void OnConversationAdded(const std::string& uuid);
  void OnConversationModified(const std::string& uuid);
  void OnConversationDeleted(const std::string& uuid);

  base::WeakPtr<AIChatSyncBridge> GetWeakPtr() {
    return weak_ptr_factory_.GetWeakPtr();
  }

 private:
  const raw_ptr<AIChatDatabase> database_;

  SEQUENCE_CHECKER(sequence_checker_);

  base::WeakPtrFactory<AIChatSyncBridge> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_SYNC_AI_CHAT_SYNC_BRIDGE_H_
