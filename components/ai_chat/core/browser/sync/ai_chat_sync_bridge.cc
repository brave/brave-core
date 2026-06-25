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
#include <vector>

#include "base/containers/flat_map.h"
#include "base/containers/map_util.h"
#include "base/functional/function_ref.h"
#include "base/hash/hash.h"
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

using CompressibleStringVisitor =
    base::FunctionRef<void(sync_pb::AIChatCompressibleString&)>;

void VisitWebSourcesEventStrings(sync_pb::AIChatWebSourcesEvent* web_sources,
                                 CompressibleStringVisitor visit) {
  for (auto& source : *web_sources->mutable_sources()) {
    if (source.has_page_content()) {
      visit(*source.mutable_page_content());
    }
  }
  for (auto& rich_result : *web_sources->mutable_rich_results()) {
    visit(rich_result);
  }
}

void VisitToolUseStrings(sync_pb::AIChatToolUseEvent* tool_use,
                         CompressibleStringVisitor visit) {
  if (tool_use->has_arguments_json()) {
    visit(*tool_use->mutable_arguments_json());
  }
  for (auto& block : *tool_use->mutable_output()) {
    if (block.has_text_content_block()) {
      visit(*block.mutable_text_content_block()->mutable_text());
    }
  }
}

void VisitEventStrings(sync_pb::AIChatEntryEventProto* event,
                       CompressibleStringVisitor visit) {
  if (event->has_completion()) {
    visit(*event->mutable_completion());
  }
  if (event->has_web_sources()) {
    VisitWebSourcesEventStrings(event->mutable_web_sources(), visit);
  }
  if (event->has_tool_use()) {
    VisitToolUseStrings(event->mutable_tool_use(), visit);
  }
}

// Invokes |visit| for every AIChatCompressibleString on |entry| that
// FitEntryWithinSyncBudget can omit. Kept in sync with that function's step
// list so that any field which may arrive with an omitted_content_hash can be
// both detected and restored here.
void ForEachOmittableCompressibleString(
    sync_pb::AIChatConversationSpecifics_Entry* entry,
    CompressibleStringVisitor visit) {
  for (auto& ac : *entry->mutable_associated_content()) {
    if (ac.has_last_contents()) {
      visit(*ac.mutable_last_contents());
    }
  }
  for (auto& file : *entry->mutable_uploaded_files()) {
    if (file.has_extracted_text()) {
      visit(*file.mutable_extracted_text());
    }
  }
  for (auto& event : *entry->mutable_events()) {
    VisitEventStrings(&event, visit);
  }
}

// Maps each stored content row's UUID to its extracted text, so
// EntryToSpecifics can serialize the associated page text (which the archive
// keeps in |associated_content| as ContentArchive rows, separate from the
// per-turn associated-content metadata).
base::flat_map<std::string, std::string> ContentTextsFromArchive(
    const mojom::ConversationArchive& archive) {
  base::flat_map<std::string, std::string> texts;
  for (const auto& content : archive.associated_content) {
    texts.emplace(content->content_uuid, content->content);
  }
  return texts;
}

// Serializes |entry| (with its associated content and page texts) and omits
// oversized fields to fit the per-record sync budget. Returns nullopt when the
// record is too large even after omission, in which case it must not be
// committed.
std::optional<sync_pb::AIChatConversationSpecifics> BuildCommittableEntry(
    const std::string& conversation_uuid,
    const mojom::ConversationTurn& entry,
    const std::vector<mojom::AssociatedContentPtr>& associated_content,
    const base::flat_map<std::string, std::string>& content_texts) {
  auto specifics = EntryToSpecifics(conversation_uuid, entry,
                                    associated_content, content_texts);
  if (!FitEntryWithinSyncBudget(specifics.mutable_entry())) {
    return std::nullopt;
  }
  return specifics;
}

}  // namespace

AIChatSyncBridge::AIChatSyncBridge(
    std::unique_ptr<syncer::DataTypeLocalChangeProcessor> change_processor,
    base::RepeatingClosure on_remote_changes_applied)
    : syncer::DataTypeSyncBridge(std::move(change_processor)),
      on_remote_changes_applied_(std::move(on_remote_changes_applied)) {
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

    const auto content_texts = ContentTextsFromArchive(*archive);

    for (const auto& entry : archive->entries) {
      if (!entry->uuid) {
        continue;
      }

      std::string entry_key =
          base::StrCat({kEntryStorageKeyPrefix, *entry->uuid});

      auto specifics = BuildCommittableEntry(conversation->uuid, *entry,
                                             conversation->associated_content,
                                             content_texts);

      if (!specifics || !should_include(entry_key)) {
        continue;
      }

      emit(std::move(entry_key), CreateEntityDataFromSpecifics(*specifics));
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
  bool any_remote_change_applied = false;
  for (const auto& change : entity_data) {
    if (change->type() == syncer::EntityChange::ACTION_DELETE) {
      continue;
    }
    remote_storage_keys.insert(change->storage_key());
    ApplyRemoteRecord(change->data().specifics.ai_chat_conversation());
    any_remote_change_applied = true;
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

  if (any_remote_change_applied && on_remote_changes_applied_) {
    on_remote_changes_applied_.Run();
  }
  return std::nullopt;
}

std::optional<syncer::ModelError> AIChatSyncBridge::ApplyIncrementalSyncChanges(
    std::unique_ptr<syncer::MetadataChangeList> metadata_change_list,
    syncer::EntityChangeList entity_changes) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!database_) {
    return std::nullopt;
  }

  bool any_remote_change_applied = false;
  for (const auto& change : entity_changes) {
    const std::string& storage_key = change->storage_key();

    if (change->type() == syncer::EntityChange::ACTION_DELETE) {
      if (storage_key.starts_with(kConversationStorageKeyPrefix)) {
        database_->DeleteConversation(
            storage_key.substr(kConversationStorageKeyPrefix.size()));
        any_remote_change_applied = true;
      } else if (storage_key.starts_with(kEntryStorageKeyPrefix)) {
        database_->DeleteConversationEntry(
            storage_key.substr(kEntryStorageKeyPrefix.size()));
        any_remote_change_applied = true;
      }
      continue;
    }

    ApplyRemoteRecord(change->data().specifics.ai_chat_conversation());
    any_remote_change_applied = true;
  }

  if (any_remote_change_applied && on_remote_changes_applied_) {
    on_remote_changes_applied_.Run();
  }
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
        auto specifics = BuildCommittableEntry(
            conversation->uuid, **it, conversation->associated_content,
            ContentTextsFromArchive(*archive));
        if (!specifics) {
          break;
        }
        batch->Put(key, CreateEntityDataFromSpecifics(*specifics));
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

void AIChatSyncBridge::OnConversationAdded(const std::string& uuid) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!change_processor()->IsTrackingMetadata()) {
    return;
  }
  PutConversationMetadata(uuid);
}

void AIChatSyncBridge::OnConversationModified(const std::string& uuid) {
  OnConversationAdded(uuid);
}

void AIChatSyncBridge::OnConversationDeleted(const std::string& uuid) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!database_ || !change_processor()->IsTrackingMetadata()) {
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
      change_processor()->Delete(
          base::StrCat({kEntryStorageKeyPrefix, *entry->uuid}),
          syncer::DeletionOrigin::Unspecified(), metadata_change_list.get());
    }
  }

  change_processor()->Delete(
      base::StrCat({kConversationStorageKeyPrefix, uuid}),
      syncer::DeletionOrigin::Unspecified(), metadata_change_list.get());
}

void AIChatSyncBridge::OnConversationEntryAdded(
    const std::string& conversation_uuid,
    const std::string& entry_uuid) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!change_processor()->IsTrackingMetadata()) {
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
  if (!change_processor()->IsTrackingMetadata()) {
    return;
  }
  auto metadata_change_list = CreateMetadataChangeList();
  change_processor()->Delete(base::StrCat({kEntryStorageKeyPrefix, entry_uuid}),
                             syncer::DeletionOrigin::Unspecified(),
                             metadata_change_list.get());
}

void AIChatSyncBridge::ApplyRemoteRecord(
    const sync_pb::AIChatConversationSpecifics& specifics) {
  if (specifics.has_conversation()) {
    auto conversation = SpecificsToConversationMetadata(specifics);
    if (conversation) {
      database_->ApplyRemoteConversationMetadata(std::move(conversation));
    }
    return;
  }
  if (!specifics.has_entry()) {
    return;
  }

  // Restore local values into the remote proto for any field the sender
  // omitted to fit the size budget. This guarantees a re-sync of an existing
  // entry does not overwrite locally-present content (page text, uploaded
  // file data, extracted text, completions) with empty placeholders.
  sync_pb::AIChatConversationSpecifics merged = specifics;
  RestoreOmittedFieldsFromLocal(merged.mutable_entry());

  std::vector<mojom::AssociatedContentPtr> associated_content;
  base::flat_map<std::string, std::string> associated_content_texts;
  auto entry =
      SpecificsToEntry(merged, &associated_content, &associated_content_texts);
  if (!entry) {
    return;
  }

  // Build a parallel |contents| vector for ApplyRemoteEntry. When the map
  // has no entry for an AC UUID (because the sender chose not to send a
  // text and we could not recover one locally), leave it empty — the DB
  // will write an empty last_contents column.
  std::vector<std::string> contents;
  contents.reserve(associated_content.size());
  for (const auto& ac : associated_content) {
    auto it = associated_content_texts.find(ac->uuid);
    contents.emplace_back(it != associated_content_texts.end() ? it->second
                                                               : std::string());
  }
  database_->ApplyRemoteEntry(merged.entry().conversation_uuid(),
                              std::move(entry), std::move(associated_content),
                              std::move(contents));
}

base::flat_map<uint32_t, std::string> AIChatSyncBridge::BuildLocalContentByHash(
    const std::string& conversation_uuid,
    const std::string& entry_uuid) {
  base::flat_map<uint32_t, std::string> content_by_hash;
  auto remember = [&content_by_hash](std::string_view value) {
    if (!value.empty()) {
      content_by_hash.emplace(base::PersistentHash(value), std::string(value));
    }
  };

  if (auto archive = database_->GetConversationData(conversation_uuid)) {
    for (const auto& local_entry : archive->entries) {
      if (!local_entry->uuid || *local_entry->uuid != entry_uuid) {
        continue;
      }
      // Re-serialize the local entry to reuse the proto walk over all of its
      // compressible strings.
      auto local_specifics =
          EntryToSpecifics(conversation_uuid, *local_entry, {});
      ForEachOmittableCompressibleString(
          local_specifics.mutable_entry(),
          [&remember](sync_pb::AIChatCompressibleString& value) {
            if (auto plaintext = ReadCompressibleString(value)) {
              remember(*plaintext);
            }
          });
      if (local_entry->uploaded_files) {
        for (const auto& file : *local_entry->uploaded_files) {
          remember(
              std::string_view(reinterpret_cast<const char*>(file->data.data()),
                               file->data.size()));
        }
      }
      break;
    }
  }
  for (const auto& content :
       database_->GetArchiveContentsForConversation(conversation_uuid)) {
    remember(content->content);
  }
  return content_by_hash;
}

void AIChatSyncBridge::RestoreOmittedFieldsFromLocal(
    sync_pb::AIChatConversationSpecifics_Entry* remote_entry) {
  const std::string& conversation_uuid = remote_entry->conversation_uuid();
  const std::string& entry_uuid = remote_entry->uuid();
  if (conversation_uuid.empty() || entry_uuid.empty()) {
    return;
  }

  // Skip the local lookup entirely unless the sender omitted something.
  bool needs_restore = false;
  ForEachOmittableCompressibleString(
      remote_entry, [&needs_restore](sync_pb::AIChatCompressibleString& value) {
        needs_restore |= value.has_omitted_content_hash();
      });
  for (const auto& file : remote_entry->uploaded_files()) {
    needs_restore |= file.has_omitted_data_hash();
  }
  if (!needs_restore) {
    return;
  }

  // Matching by content hash restores a value only when the local copy is
  // byte-identical to what the sender omitted, so it never fabricates
  // divergent content.
  const base::flat_map<uint32_t, std::string> content_by_hash =
      BuildLocalContentByHash(conversation_uuid, entry_uuid);

  ForEachOmittableCompressibleString(
      remote_entry, [&content_by_hash](sync_pb::AIChatCompressibleString& v) {
        if (!v.has_omitted_content_hash()) {
          return;
        }
        if (const std::string* local =
                base::FindOrNull(content_by_hash, v.omitted_content_hash())) {
          WriteCompressibleString(*local, &v);
        }
      });
  for (auto& file : *remote_entry->mutable_uploaded_files()) {
    if (!file.has_omitted_data_hash()) {
      continue;
    }
    if (const std::string* local =
            base::FindOrNull(content_by_hash, file.omitted_data_hash())) {
      file.set_data(*local);
    }
  }
}

void AIChatSyncBridge::PutConversationMetadata(const std::string& uuid) {
  if (!database_) {
    return;
  }
  auto conversation = FindSyncableConversation(*database_, uuid);
  if (!conversation) {
    return;
  }
  auto specifics = ConversationMetadataToSpecifics(*conversation);
  auto metadata_change_list = CreateMetadataChangeList();
  change_processor()->Put(base::StrCat({kConversationStorageKeyPrefix, uuid}),
                          CreateEntityDataFromSpecifics(specifics),
                          metadata_change_list.get());
}

void AIChatSyncBridge::PutEntry(const std::string& conversation_uuid,
                                const std::string& entry_uuid) {
  if (!database_) {
    return;
  }
  auto conversation = FindSyncableConversation(*database_, conversation_uuid);
  if (!conversation) {
    return;
  }
  auto archive = database_->GetConversationData(conversation_uuid);
  if (!archive) {
    return;
  }
  auto it = std::ranges::find(archive->entries, entry_uuid,
                              &mojom::ConversationTurn::uuid);
  if (it == archive->entries.end()) {
    return;
  }
  auto specifics = BuildCommittableEntry(conversation_uuid, **it,
                                         conversation->associated_content,
                                         ContentTextsFromArchive(*archive));
  if (!specifics) {
    return;
  }
  auto metadata_change_list = CreateMetadataChangeList();
  change_processor()->Put(base::StrCat({kEntryStorageKeyPrefix, entry_uuid}),
                          CreateEntityDataFromSpecifics(*specifics),
                          metadata_change_list.get());
}

}  // namespace ai_chat
