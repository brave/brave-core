/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/browser/sync/ai_chat_sync_bridge.h"

#include <algorithm>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <utility>

#include "base/logging.h"
#include "base/strings/strcat.h"
#include "brave/components/ai_chat/core/browser/ai_chat_database.h"
#include "brave/components/ai_chat/core/browser/sync/ai_chat_sync_conversions.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/sync/protocol/ai_chat_specifics.pb.h"
#include "components/sync/base/deletion_origin.h"
#include "components/sync/model/data_type_local_change_processor.h"
#include "components/sync/model/metadata_batch.h"
#include "components/sync/model/model_error.h"
#include "components/sync/model/mutable_data_batch.h"
#include "components/sync/model/sync_metadata_store_change_list.h"
#include "components/sync/protocol/entity_data.h"

namespace ai_chat {

namespace {

// Looks up a conversation by UUID. Returns nullptr if it does not exist or is
// temporary.
mojom::ConversationPtr FindSyncableConversation(AIChatDatabase* database,
                                                std::string_view uuid) {
  for (auto& conversation : database->GetAllConversations()) {
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
      database_(database) {
  DCHECK(database_);
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  // Load metadata synchronously (we're on the database sequence).
  auto batch = std::make_unique<syncer::MetadataBatch>();
  if (!database_->GetAllSyncMetadata(batch.get())) {
    this->change_processor()->ReportError(
        {FROM_HERE, syncer::ModelError::Type::kAIChatFailedToLoadMetadata});
    return;
  }
  this->change_processor()->ModelReadyToSync(std::move(batch));
}

AIChatSyncBridge::~AIChatSyncBridge() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
}

std::unique_ptr<syncer::MetadataChangeList>
AIChatSyncBridge::CreateMetadataChangeList() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return std::make_unique<syncer::SyncMetadataStoreChangeList>(
      database_, syncer::AI_CHAT_CONVERSATION,
      base::BindRepeating(
          [](base::WeakPtr<AIChatSyncBridge> bridge,
             const syncer::ModelError& error) {
            if (bridge) {
              bridge->change_processor()->ReportError(error);
            }
          },
          weak_ptr_factory_.GetWeakPtr()));
}

std::optional<syncer::ModelError> AIChatSyncBridge::MergeFullSyncData(
    std::unique_ptr<syncer::MetadataChangeList> metadata_change_list,
    syncer::EntityChangeList entity_data) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  // Track which storage keys we already have from remote so we don't
  // re-upload them.
  std::set<std::string> remote_storage_keys;
  for (const auto& change : entity_data) {
    if (change->type() == syncer::EntityChange::ACTION_DELETE) {
      continue;
    }
    remote_storage_keys.insert(change->storage_key());
    // TODO(https://github.com/brave/brave-browser/issues/53978): apply
    // remote ADD/UPDATE to local database (handled in -flow).
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

  for (const auto& change : entity_changes) {
    const std::string& storage_key = change->storage_key();

    if (change->type() == syncer::EntityChange::ACTION_DELETE) {
      if (storage_key.starts_with(kConversationStorageKeyPrefix)) {
        database_->DeleteConversation(
            storage_key.substr(kConversationStorageKeyPrefix.size()));
      } else if (storage_key.starts_with(kEntryStorageKeyPrefix)) {
        database_->DeleteConversationEntry(
            storage_key.substr(kEntryStorageKeyPrefix.size()));
      }
      continue;
    }

    // TODO(https://github.com/brave/brave-browser/issues/53978): apply remote
    // ADD/UPDATE for both record kinds (handled in -flow).
    DVLOG(1) << "Received remote sync change for: " << storage_key;
  }

  return std::nullopt;
}

std::unique_ptr<syncer::DataBatch> AIChatSyncBridge::GetDataForCommit(
    StorageKeyList storage_keys) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  auto batch = std::make_unique<syncer::MutableDataBatch>();
  for (const auto& key : storage_keys) {
    if (key.starts_with(kConversationStorageKeyPrefix)) {
      const std::string uuid = key.substr(kConversationStorageKeyPrefix.size());
      auto conversation = FindSyncableConversation(database_, uuid);
      if (!conversation) {
        continue;
      }
      auto specifics = ConversationMetadataToSpecifics(*conversation);
      batch->Put(key, CreateEntityDataFromSpecifics(specifics));
    } else if (key.starts_with(kEntryStorageKeyPrefix)) {
      const std::string entry_uuid = key.substr(kEntryStorageKeyPrefix.size());
      // Locate the parent conversation by scanning. A long-conversation
      // optimisation would index entry→conversation in the DB.
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
  return GetClientTagFromSpecifics(
      entity_data.specifics.ai_chat_conversation());
}

std::string AIChatSyncBridge::GetStorageKey(
    const syncer::EntityData& entity_data) const {
  return GetStorageKeyFromSpecifics(
      entity_data.specifics.ai_chat_conversation());
}

bool AIChatSyncBridge::IsEntityDataValid(
    const syncer::EntityData& entity_data) const {
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

void AIChatSyncBridge::OnConversationAdded(const std::string& uuid) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!this->change_processor()->IsTrackingMetadata()) {
    return;
  }
  PutConversationMetadata(uuid);
}

void AIChatSyncBridge::OnConversationModified(const std::string& uuid) {
  OnConversationAdded(uuid);
}

void AIChatSyncBridge::OnConversationDeleted(const std::string& uuid) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!this->change_processor()->IsTrackingMetadata()) {
    return;
  }
  auto metadata_change_list = CreateMetadataChangeList();

  // Emit a Delete for every entry of the conversation BEFORE deleting the
  // conversation itself. This must run while the entries still exist in the
  // database.
  if (auto archive = database_->GetConversationData(uuid)) {
    for (const auto& entry : archive->entries) {
      if (!entry->uuid) {
        continue;
      }
      this->change_processor()->Delete(
          base::StrCat({kEntryStorageKeyPrefix, *entry->uuid}),
          syncer::DeletionOrigin::Unspecified(), metadata_change_list.get());
    }
  }

  this->change_processor()->Delete(
      base::StrCat({kConversationStorageKeyPrefix, uuid}),
      syncer::DeletionOrigin::Unspecified(), metadata_change_list.get());
}

void AIChatSyncBridge::OnConversationEntryAdded(
    const std::string& conversation_uuid,
    const std::string& entry_uuid) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!this->change_processor()->IsTrackingMetadata()) {
    return;
  }
  PutEntry(conversation_uuid, entry_uuid);
}

void AIChatSyncBridge::OnConversationEntryModified(
    const std::string& conversation_uuid,
    const std::string& entry_uuid) {
  OnConversationEntryAdded(conversation_uuid, entry_uuid);
}

void AIChatSyncBridge::OnConversationEntryDeleted(
    const std::string& entry_uuid) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!this->change_processor()->IsTrackingMetadata()) {
    return;
  }
  auto metadata_change_list = CreateMetadataChangeList();
  this->change_processor()->Delete(
      base::StrCat({kEntryStorageKeyPrefix, entry_uuid}),
      syncer::DeletionOrigin::Unspecified(), metadata_change_list.get());
}

void AIChatSyncBridge::PutConversationMetadata(const std::string& uuid) {
  auto conversation = FindSyncableConversation(database_, uuid);
  if (!conversation) {
    return;
  }
  auto specifics = ConversationMetadataToSpecifics(*conversation);
  auto metadata_change_list = CreateMetadataChangeList();
  this->change_processor()->Put(
      base::StrCat({kConversationStorageKeyPrefix, uuid}),
      CreateEntityDataFromSpecifics(specifics), metadata_change_list.get());
}

void AIChatSyncBridge::PutEntry(const std::string& conversation_uuid,
                                const std::string& entry_uuid) {
  auto conversation = FindSyncableConversation(database_, conversation_uuid);
  if (!conversation) {
    return;
  }
  auto archive = database_->GetConversationData(conversation_uuid);
  if (!archive) {
    return;
  }
  auto it = std::ranges::find_if(
      archive->entries, [&](const mojom::ConversationTurnPtr& entry) {
        return entry->uuid && *entry->uuid == entry_uuid;
      });
  if (it == archive->entries.end()) {
    return;
  }
  auto specifics = EntryToSpecifics(conversation_uuid, **it,
                                    conversation->associated_content);
  auto metadata_change_list = CreateMetadataChangeList();
  this->change_processor()->Put(
      base::StrCat({kEntryStorageKeyPrefix, entry_uuid}),
      CreateEntityDataFromSpecifics(specifics), metadata_change_list.get());
}

}  // namespace ai_chat
