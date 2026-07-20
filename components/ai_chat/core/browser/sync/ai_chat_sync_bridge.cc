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
    std::unique_ptr<syncer::DataTypeLocalChangeProcessor> change_processor)
    : syncer::DataTypeSyncBridge(std::move(change_processor)) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
}

AIChatSyncBridge::~AIChatSyncBridge() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
}

void AIChatSyncBridge::SetDatabase(AIChatDatabase* database) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  database_ = database;

  // The initial metadata load + ModelReadyToSync() happens once, on the first
  // attached database. A later re-attach (storage toggled off then on) just
  // re-points |database_|: the change processor keeps its in-memory metadata,
  // and ModelReadyToSync() must not be called a second time.
  if (model_ready_to_sync_) {
    return;
  }

  // Load metadata synchronously (we're on the database sequence).
  auto batch = std::make_unique<syncer::MetadataBatch>();
  if (!database_->GetAllSyncMetadata(batch.get())) {
    ReportError(
        {FROM_HERE, syncer::ModelError::Type::kAIChatFailedToLoadMetadata});
    return;
  }
  model_ready_to_sync_ = true;
  change_processor()->ModelReadyToSync(std::move(batch));
}

void AIChatSyncBridge::ClearDatabase() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  database_ = nullptr;
  // Deliberately does NOT reset |model_ready_to_sync_| or otherwise poke the
  // change processor: the change processor stays "model ready" across a sync
  // stop (ClientTagBasedDataTypeProcessor::OnSyncStopping CHECKs
  // model_ready_to_sync_ is still set), so re-firing ModelReadyToSync() on a
  // later SetDatabase() would hit its CHECK(!model_ready_to_sync_) and crash.
  //
  // Detaching alone therefore leaves sync tracking untouched: after a storage
  // off->on toggle the processor still believes the (now-empty) model is fully
  // synced, so it will not re-download conversations from the server until the
  // next browser start clears the metadata. Making the toggle perform a fresh
  // initial sync is coordinated at the sync-service layer, not here: the AI
  // Chat sync DataTypeController reports kMustStopAndClearData while on-disk
  // storage is off, so the engine stops and clears the type on disable and
  // re-runs initial sync on re-enable. See
  // https://github.com/brave/brave-browser/issues/53978.
}

void AIChatSyncBridge::ReportError(const syncer::ModelError& error) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  change_processor()->ReportError(error);
}

void AIChatSyncBridge::ForEachLocalEntity(
    base::FunctionRef<bool(const std::string&)> should_include,
    base::FunctionRef<void(std::string, std::unique_ptr<syncer::EntityData>)>
        emit) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  CHECK(database_);

  for (const auto& conversation : database_->GetAllConversations()) {
    if (conversation->temporary) {
      continue;
    }

    std::string conversation_key =
        base::StrCat({kConversationStorageKeyPrefix, conversation->uuid});

    if (should_include(conversation_key)) {
      emit(std::move(conversation_key),
           CreateEntityDataFromSpecifics(
               ConversationMetadataToSpecifics(*conversation)));
    }

    auto archive = database_->GetConversationData(conversation->uuid);
    if (!archive) {
      continue;
    }

    for (const auto& entry : archive->entries) {
      if (!entry->uuid) {
        continue;
      }

      std::string entry_key =
          base::StrCat({kEntryStorageKeyPrefix, *entry->uuid});

      if (!should_include(entry_key)) {
        continue;
      }

      emit(std::move(entry_key),
           CreateEntityDataFromSpecifics(EntryToSpecifics(
               conversation->uuid, *entry, conversation->associated_content)));
    }
  }
}

std::unique_ptr<syncer::MetadataChangeList>
AIChatSyncBridge::CreateMetadataChangeList() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // Without a database (storage disabled) there is nowhere to persist metadata;
  // fall back to the in-memory change list so any changes are simply dropped.
  if (!database_) {
    return syncer::DataTypeSyncBridge::CreateMetadataChangeList();
  }
  return std::make_unique<syncer::SyncMetadataStoreChangeList>(
      database_, syncer::AI_CHAT_CONVERSATION,
      base::BindRepeating(&AIChatSyncBridge::ReportError,
                          weak_ptr_factory_.GetWeakPtr()));
}

std::optional<syncer::ModelError> AIChatSyncBridge::MergeFullSyncData(
    std::unique_ptr<syncer::MetadataChangeList> metadata_change_list,
    syncer::EntityChangeList entity_data) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!database_) {
    return std::nullopt;
  }

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
  ForEachLocalEntity(
      [&](const std::string& storage_key) {
        return !remote_storage_keys.contains(storage_key);
      },
      [&](std::string storage_key,
          std::unique_ptr<syncer::EntityData> entity_data) {
        change_processor()->Put(std::move(storage_key), std::move(entity_data),
                                metadata_change_list.get());
      });

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
  if (!database_) {
    return batch;
  }
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
  if (!database_) {
    return batch;
  }

  ForEachLocalEntity([](const std::string&) { return true; },
                     [&](std::string storage_key,
                         std::unique_ptr<syncer::EntityData> entity_data) {
                       batch->Put(std::move(storage_key),
                                  std::move(entity_data));
                     });

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
  if (!database_) {
    return;
  }
  database_->ClearAllEntityMetadata();
  database_->ClearDataTypeState(syncer::AI_CHAT_CONVERSATION);
}

}  // namespace ai_chat
