/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/browser/sync/ai_chat_sync_bridge.h"

#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "base/logging.h"
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

  // Get all local conversations.
  auto local_conversations = database_->GetAllConversations();

  // Build set of remote UUIDs.
  std::set<std::string> remote_uuids;
  for (const auto& change : entity_data) {
    if (change->type() == syncer::EntityChange::ACTION_DELETE) {
      continue;
    }
    const auto& specifics = change->data().specifics.ai_chat_conversation();
    remote_uuids.insert(specifics.uuid());

    // Apply remote conversation to local database.
    auto conversation = SpecificsToConversation(specifics);
    auto archive = SpecificsToArchive(specifics);
    std::vector<mojom::ConversationTurnPtr> entries;
    for (auto& entry : archive->entries) {
      entries.push_back(std::move(entry));
    }
    database_->ApplyRemoteConversation(std::move(conversation),
                                       std::move(entries));
  }

  // Upload local-only conversations to sync.
  for (const auto& conversation : local_conversations) {
    if (conversation->temporary) {
      continue;
    }
    if (remote_uuids.contains(conversation->uuid)) {
      continue;
    }
    auto archive = database_->GetConversationData(conversation->uuid);
    if (!archive) {
      continue;
    }
    auto specifics = ConversationToSpecifics(*conversation, *archive);
    this->change_processor()->Put(conversation->uuid,
                                  CreateEntityDataFromSpecifics(specifics),
                                  metadata_change_list.get());
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
      database_->DeleteConversation(storage_key);
      continue;
    }

    // Apply remote ADD/UPDATE to local database.
    const auto& specifics = change->data().specifics.ai_chat_conversation();
    auto conversation = SpecificsToConversation(specifics);
    auto archive = SpecificsToArchive(specifics);
    std::vector<mojom::ConversationTurnPtr> entries;
    for (auto& entry : archive->entries) {
      entries.push_back(std::move(entry));
    }
    database_->ApplyRemoteConversation(std::move(conversation),
                                       std::move(entries));
  }

  return std::nullopt;
}

std::unique_ptr<syncer::DataBatch> AIChatSyncBridge::GetDataForCommit(
    StorageKeyList storage_keys) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  auto batch = std::make_unique<syncer::MutableDataBatch>();
  for (const auto& key : storage_keys) {
    auto conversations = database_->GetAllConversations();
    for (const auto& conversation : conversations) {
      if (conversation->uuid == key) {
        auto archive = database_->GetConversationData(key);
        if (archive) {
          auto specifics = ConversationToSpecifics(*conversation, *archive);
          batch->Put(key, CreateEntityDataFromSpecifics(specifics));
        }
        break;
      }
    }
  }
  return batch;
}

std::unique_ptr<syncer::DataBatch> AIChatSyncBridge::GetAllDataForDebugging() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  auto batch = std::make_unique<syncer::MutableDataBatch>();
  auto conversations = database_->GetAllConversations();
  for (const auto& conversation : conversations) {
    if (conversation->temporary) {
      continue;
    }
    auto archive = database_->GetConversationData(conversation->uuid);
    if (archive) {
      auto specifics = ConversationToSpecifics(*conversation, *archive);
      batch->Put(conversation->uuid, CreateEntityDataFromSpecifics(specifics));
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
  return entity_data.specifics.has_ai_chat_conversation() &&
         !entity_data.specifics.ai_chat_conversation().uuid().empty();
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

  auto conversations = database_->GetAllConversations();
  for (const auto& conversation : conversations) {
    if (conversation->uuid == uuid) {
      if (conversation->temporary) {
        return;
      }
      auto archive = database_->GetConversationData(uuid);
      if (!archive) {
        return;
      }
      auto specifics = ConversationToSpecifics(*conversation, *archive);
      auto metadata_change_list = CreateMetadataChangeList();
      this->change_processor()->Put(uuid,
                                    CreateEntityDataFromSpecifics(specifics),
                                    metadata_change_list.get());
      return;
    }
  }
}

void AIChatSyncBridge::OnConversationModified(const std::string& uuid) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  OnConversationAdded(uuid);
}

void AIChatSyncBridge::OnConversationDeleted(const std::string& uuid) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!this->change_processor()->IsTrackingMetadata()) {
    return;
  }
  auto metadata_change_list = CreateMetadataChangeList();
  this->change_processor()->Delete(uuid, syncer::DeletionOrigin::Unspecified(),
                                   metadata_change_list.get());
}

}  // namespace ai_chat
