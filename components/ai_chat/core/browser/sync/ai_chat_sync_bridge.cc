/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/browser/sync/ai_chat_sync_bridge.h"

#include <algorithm>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include "base/check_deref.h"
#include "base/strings/strcat.h"
#include "brave/components/ai_chat/core/browser/ai_chat_database.h"
#include "brave/components/ai_chat/core/browser/sync/ai_chat_sync_conversions.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/sync/protocol/ai_chat_specifics.pb.h"
#include "components/sync/model/data_type_local_change_processor.h"
#include "components/sync/model/metadata_batch.h"
#include "components/sync/model/model_error.h"
#include "components/sync/model/mutable_data_batch.h"
#include "components/sync/model/sync_metadata_store_change_list.h"
#include "components/sync/protocol/entity_data.h"
#include "third_party/abseil-cpp/absl/container/flat_hash_set.h"

namespace ai_chat {

namespace {

// Looks up a conversation by UUID. Returns nullptr if it does not exist or is
// temporary.
mojom::ConversationPtr FindSyncableConversation(AIChatDatabase& database,
                                                std::string_view uuid) {
  for (auto& conversation : database.GetAllConversations()) {
    if (conversation->uuid == uuid) {
      if (conversation->temporary) {
        return nullptr;
      }
      return std::move(conversation);
    }
  }
  return nullptr;
}

}  // namespace

AIChatSyncBridge::AIChatSyncBridge(
    std::unique_ptr<syncer::DataTypeLocalChangeProcessor> change_processor,
    AIChatDatabase* database)
    : syncer::DataTypeSyncBridge(std::move(change_processor)),
      database_(CHECK_DEREF(database)) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  // Load metadata synchronously (we're on the database sequence).
  auto batch = std::make_unique<syncer::MetadataBatch>();
  if (!database_->GetAllSyncMetadata(batch.get())) {
    ReportError(
        {FROM_HERE, syncer::ModelError::Type::kAIChatFailedToLoadMetadata});
    return;
  }
  this->change_processor()->ModelReadyToSync(std::move(batch));
}

AIChatSyncBridge::~AIChatSyncBridge() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
}

void AIChatSyncBridge::ReportError(const syncer::ModelError& error) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  this->change_processor()->ReportError(error);
}

std::unique_ptr<syncer::MetadataChangeList>
AIChatSyncBridge::CreateMetadataChangeList() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return std::make_unique<syncer::SyncMetadataStoreChangeList>(
      &database_.get(), syncer::AI_CHAT_CONVERSATION,
      base::BindRepeating(&AIChatSyncBridge::ReportError,
                          weak_ptr_factory_.GetWeakPtr()));
}

std::optional<syncer::ModelError> AIChatSyncBridge::MergeFullSyncData(
    std::unique_ptr<syncer::MetadataChangeList> metadata_change_list,
    syncer::EntityChangeList entity_data) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  // Track which storage keys we already have from remote so we don't
  // re-upload them.
  absl::flat_hash_set<std::string> remote_storage_keys;
  for (const auto& change : entity_data) {
    if (change->type() == syncer::EntityChange::ACTION_DELETE) {
      continue;
    }
    remote_storage_keys.insert(change->storage_key());
    // TODO(https://github.com/brave/brave-browser/issues/53978): apply
    // remote ADD/UPDATE to local database.
  }

  // Upload every non-temporary local conversation: its metadata plus each of
  // its entries that aren't already present remotely.
  for (const auto& conversation : database_->GetAllConversations()) {
    if (conversation->temporary) {
      continue;
    }
    const std::string conversation_key =
        base::StrCat({kConversationStorageKeyPrefix, conversation->uuid});
    if (!remote_storage_keys.contains(conversation_key)) {
      auto specifics = ConversationMetadataToSpecifics(*conversation);
      this->change_processor()->Put(conversation_key,
                                    CreateEntityDataFromSpecifics(specifics),
                                    metadata_change_list.get());
    }

    auto archive = database_->GetConversationData(conversation->uuid);
    if (!archive) {
      continue;
    }
    for (const auto& entry : archive->entries) {
      if (!entry->uuid) {
        continue;
      }
      const std::string entry_key =
          base::StrCat({kEntryStorageKeyPrefix, *entry->uuid});
      if (remote_storage_keys.contains(entry_key)) {
        continue;
      }
      auto specifics = EntryToSpecifics(conversation->uuid, *entry,
                                        conversation->associated_content);
      this->change_processor()->Put(entry_key,
                                    CreateEntityDataFromSpecifics(specifics),
                                    metadata_change_list.get());
    }
  }

  return std::nullopt;
}

std::optional<syncer::ModelError> AIChatSyncBridge::ApplyIncrementalSyncChanges(
    std::unique_ptr<syncer::MetadataChangeList> metadata_change_list,
    syncer::EntityChangeList entity_changes) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // TODO(https://github.com/brave/brave-browser/issues/53978): apply remote
  // ADD/UPDATE/DELETE to the local database.
  return std::nullopt;
}

std::unique_ptr<syncer::DataBatch> AIChatSyncBridge::GetDataForCommit(
    StorageKeyList storage_keys) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  auto batch = std::make_unique<syncer::MutableDataBatch>();
  for (const auto& key : storage_keys) {
    if (key.starts_with(kConversationStorageKeyPrefix)) {
      const std::string uuid = key.substr(kConversationStorageKeyPrefix.size());
      auto conversation = FindSyncableConversation(*database_, uuid);
      if (!conversation) {
        continue;
      }
      auto specifics = ConversationMetadataToSpecifics(*conversation);
      batch->Put(key, CreateEntityDataFromSpecifics(specifics));
    } else if (key.starts_with(kEntryStorageKeyPrefix)) {
      const std::string entry_uuid = key.substr(kEntryStorageKeyPrefix.size());
      // Locate the parent conversation by scanning.
      // TODO(https://github.com/brave/brave-browser/issues/53978): Optimize via
      // indexing entry→conversation in the DB, or by creating a
      // GetConversationEntry(uuid) method in AIChatDatabase.
      for (const auto& conversation : database_->GetAllConversations()) {
        if (conversation->temporary) {
          continue;
        }
        auto archive = database_->GetConversationData(conversation->uuid);
        if (!archive) {
          continue;
        }
        auto it = std::ranges::find_if(
            archive->entries, [&](const mojom::ConversationTurnPtr& entry) {
              return entry->uuid && *entry->uuid == entry_uuid;
            });
        if (it == archive->entries.end()) {
          continue;
        }
        auto specifics = EntryToSpecifics(conversation->uuid, **it,
                                          conversation->associated_content);
        batch->Put(key, CreateEntityDataFromSpecifics(specifics));
        break;
      }
    }
  }
  return batch;
}

std::unique_ptr<syncer::DataBatch> AIChatSyncBridge::GetAllDataForDebugging() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  auto batch = std::make_unique<syncer::MutableDataBatch>();
  for (const auto& conversation : database_->GetAllConversations()) {
    if (conversation->temporary) {
      continue;
    }
    auto meta_specifics = ConversationMetadataToSpecifics(*conversation);
    batch->Put(
        base::StrCat({kConversationStorageKeyPrefix, conversation->uuid}),
        CreateEntityDataFromSpecifics(meta_specifics));

    auto archive = database_->GetConversationData(conversation->uuid);
    if (!archive) {
      continue;
    }
    for (const auto& entry : archive->entries) {
      if (!entry->uuid) {
        continue;
      }
      auto entry_specifics = EntryToSpecifics(conversation->uuid, *entry,
                                              conversation->associated_content);
      batch->Put(base::StrCat({kEntryStorageKeyPrefix, *entry->uuid}),
                 CreateEntityDataFromSpecifics(entry_specifics));
    }
  }
  return batch;
}

std::string AIChatSyncBridge::GetClientTag(
    const syncer::EntityData& entity_data) const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return GetClientTagFromSpecifics(
      entity_data.specifics.ai_chat_conversation());
}

std::string AIChatSyncBridge::GetStorageKey(
    const syncer::EntityData& entity_data) const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return GetStorageKeyFromSpecifics(
      entity_data.specifics.ai_chat_conversation());
}

bool AIChatSyncBridge::IsEntityDataValid(
    const syncer::EntityData& entity_data) const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!entity_data.specifics.has_ai_chat_conversation()) {
    return false;
  }
  const auto& specifics = entity_data.specifics.ai_chat_conversation();
  if (specifics.has_conversation()) {
    return !specifics.conversation().uuid().empty();
  }
  if (specifics.has_entry()) {
    return !specifics.entry().uuid().empty() &&
           !specifics.entry().conversation_uuid().empty();
  }
  return false;
}

sync_pb::EntitySpecifics
AIChatSyncBridge::TrimAllSupportedFieldsFromRemoteSpecifics(
    const sync_pb::EntitySpecifics& entity_specifics) const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // The bridge actively uses every field in AIChatConversationSpecifics, so
  // there is nothing to preserve for older clients. Returning an empty
  // EntitySpecifics avoids the I/O cost of caching a copy.
  return sync_pb::EntitySpecifics();
}

void AIChatSyncBridge::ApplyDisableSyncChanges(
    std::unique_ptr<syncer::MetadataChangeList> delete_metadata_change_list) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  database_->ClearAllEntityMetadata();
  database_->ClearDataTypeState(syncer::AI_CHAT_CONVERSATION);
}

}  // namespace ai_chat
