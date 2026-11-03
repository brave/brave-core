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
#include "base/containers/flat_set.h"
#include "base/containers/map_util.h"
#include "base/containers/to_vector.h"
#include "base/functional/bind.h"
#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
#include "base/task/sequenced_task_runner.h"
#include "base/time/time.h"
#include "brave/components/ai_chat/core/browser/ai_chat_database.h"
#include "brave/components/ai_chat/core/browser/sync/ai_chat_sync_conversions.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/sync/protocol/ai_chat_specifics.pb.h"
#include "components/sync/base/deletion_origin.h"
#include "components/sync/model/conflict_resolution.h"
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

// Maps each stored content row's UUID to its extracted text, so
// EntryToSpecifics can serialize the associated page text (which the archive
// keeps in |associated_content| as ContentArchive rows, separate from the
// per-turn associated-content metadata).
base::flat_map<std::string, std::string> ContentTextsFromArchive(
    const mojom::ConversationArchive& archive) {
  return base::MakeFlatMap<std::string, std::string>(
      archive.associated_content, {},
      [](const mojom::ContentArchivePtr& content) {
        return std::pair(content->content_uuid, content->content);
      });
}

// Serializes |entry| (with its associated content and page texts) and omits
// oversized fields to fit the per-record sync budget. Returns nullopt when the
// record is too large even after omission, in which case it must not be
// committed.
std::optional<sync_pb::AIChatConversationSpecifics> BuildCommittableEntry(
    const std::string& conversation_uuid,
    const mojom::ConversationTurn& entry,
    base::Time modified_time,
    const std::vector<mojom::AssociatedContentPtr>& associated_content,
    const base::flat_map<std::string, std::string>& content_texts) {
  auto specifics = EntryToSpecifics(conversation_uuid, entry, modified_time,
                                    associated_content, content_texts);
  if (!FitEntryWithinSyncBudget(specifics.mutable_entry())) {
    return std::nullopt;
  }
  return specifics;
}

// Returns the modified time to send with a local record. A failed read sends
// none, so the record counts as the oldest and can't overwrite another device's
// copy that has a time.
base::Time TimeToSend(
    const std::optional<AIChatDatabase::ModifiedTime>& modified_time) {
  return modified_time ? modified_time->time : base::Time();
}

// Reads the modified time of this device's copy of the record |specifics|.
// Returns nullopt when the read fails.
std::optional<AIChatDatabase::ModifiedTime> ReadLocalModifiedTime(
    AIChatDatabase& database,
    const sync_pb::AIChatConversationSpecifics& specifics) {
  switch (specifics.kind_case()) {
    case sync_pb::AIChatConversationSpecifics::kConversation:
      return database.GetConversationModifiedTime(
          specifics.conversation().uuid());
    case sync_pb::AIChatConversationSpecifics::kEntry:
      return database.GetEntryModifiedTime(specifics.entry().uuid());
    case sync_pb::AIChatConversationSpecifics::KIND_NOT_SET:
      return AIChatDatabase::ModifiedTime();
  }
}

// Which copy of a record changed last: the one in a remote change, or this
// device's.
enum class NewerCopy {
  kRemote,
  kSameVersion,
  kLocal,
};

// Compares a remote record, which carries |remote_time|, with this device's
// copy, |local|. With no local copy, including a stub conversation, the remote
// one is newer. A null time is unknown and counts as the oldest. Two null times
// are the same version: every change sync compares moves the time, so both are
// the record as it was before records carried a time.
NewerCopy FindNewerCopy(base::Time remote_time,
                        const AIChatDatabase::ModifiedTime& local) {
  if (!local.exists || remote_time > local.time) {
    return NewerCopy::kRemote;
  }
  return remote_time == local.time ? NewerCopy::kSameVersion
                                   : NewerCopy::kLocal;
}

// Whether |local|, read again when a kept local copy is about to be sent, is
// still the copy kept with |kept_time|. A failed read counts as not.
bool IsSameLocalCopy(const std::optional<AIChatDatabase::ModifiedTime>& local,
                     base::Time kept_time) {
  return local && local->exists && local->time == kept_time;
}

}  // namespace

AIChatSyncBridge::AIChatSyncBridge(
    std::unique_ptr<syncer::DataTypeLocalChangeProcessor> change_processor,
    RemoteChangesAppliedCallback on_remote_changes_applied)
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
           CreateEntityDataFromSpecifics(ConversationMetadataToSpecifics(
               *conversation, TimeToSend(database_->GetConversationModifiedTime(
                                  conversation->uuid)))));
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

      // Ask whether the entry is wanted before serializing it — building the
      // specifics gzips every compressible field, which is wasted on a
      // re-merge where the predicate rejects everything already downloaded.
      if (!should_include(entry_key)) {
        continue;
      }

      auto specifics = BuildCommittableEntry(
          conversation->uuid, *entry,
          TimeToSend(database_->GetEntryModifiedTime(*entry->uuid)),
          conversation->associated_content, content_texts);
      if (!specifics) {
        continue;
      }

      emit(std::move(entry_key), CreateEntityDataFromSpecifics(*specifics));
    }
  }
}

std::unique_ptr<syncer::SyncMetadataStoreChangeList>
AIChatSyncBridge::CreateMetadataStoreChangeList() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  CHECK(database_);
  return std::make_unique<syncer::SyncMetadataStoreChangeList>(
      database_, syncer::AI_CHAT_CONVERSATION,
      base::BindRepeating(&AIChatSyncBridge::ReportError,
                          weak_ptr_factory_.GetWeakPtr()));
}

void AIChatSyncBridge::ApplyMetadataChanges(
    std::unique_ptr<syncer::MetadataChangeList> metadata_change_list) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  metadata_change_list->TransferChangesTo(
      CreateMetadataStoreChangeList().get());
}

std::optional<syncer::ModelError> AIChatSyncBridge::MergeFullSyncData(
    std::unique_ptr<syncer::MetadataChangeList> metadata_change_list,
    syncer::EntityChangeList entity_data) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!database_) {
    // Nothing is stored, so neither is the metadata, and the records download
    // again on the next start.
    metadata_change_list->DropAllChanges();
    return std::nullopt;
  }

  KeptLocalCopies kept_local_copies;
  if (!ApplyRemoteChanges(entity_data, *metadata_change_list,
                          kept_local_copies)) {
    // First sync runs again on the next start, so no local update is sent
    // this session. That merge keeps a local update made since, which is
    // newer than the server's copy, and sends it then.
    skip_all_local_puts_ = true;
    return syncer::ModelError(
        FROM_HERE, syncer::ModelError::Type::kAIChatFailedToApplyRemoteChange);
  }

  // Track which storage keys we already have from remote so we don't
  // re-upload them, except where ApplyRemoteChanges() kept this device's
  // copy because it's newer.
  absl::flat_hash_set<std::string> remote_storage_keys;
  for (const auto& change : entity_data) {
    if (change->type() != syncer::EntityChange::ACTION_DELETE &&
        !kept_local_copies.contains(change->storage_key())) {
      remote_storage_keys.insert(change->storage_key());
    }
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

  ApplyMetadataChanges(std::move(metadata_change_list));
  return std::nullopt;
}

std::optional<syncer::ModelError> AIChatSyncBridge::ApplyIncrementalSyncChanges(
    std::unique_ptr<syncer::MetadataChangeList> metadata_change_list,
    syncer::EntityChangeList entity_changes) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!database_) {
    // Nothing is stored, so neither is the metadata, and the changes download
    // again on the next start.
    metadata_change_list->DropAllChanges();
    return std::nullopt;
  }

  // The processor also calls this with metadata changes alone, for example
  // after every commit. Those are saved like any other batch's.
  KeptLocalCopies kept_local_copies;
  if (!ApplyRemoteChanges(entity_changes, *metadata_change_list,
                          kept_local_copies)) {
    return syncer::ModelError(
        FROM_HERE, syncer::ModelError::Type::kAIChatFailedToApplyRemoteChange);
  }

  ApplyMetadataChanges(std::move(metadata_change_list));

  // The processor counts a Put() made while it applies remote changes
  // (Sync.LocalChangeDuringRemoteUpdate) and means to make it a CHECK, so the
  // local copies kept are sent once this returns. A crash before that task
  // runs leaves them unsent until they change again, since the batch's
  // metadata is already saved. Normal shutdown still runs the task, because
  // the database's task runner blocks shutdown.
  if (!kept_local_copies.empty()) {
    base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE, base::BindOnce(&AIChatSyncBridge::PutKeptLocalCopies,
                                  weak_ptr_factory_.GetWeakPtr(),
                                  std::move(kept_local_copies)));
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
      auto specifics = ConversationMetadataToSpecifics(
          *conversation,
          TimeToSend(database_->GetConversationModifiedTime(uuid)));
      batch->Put(key, CreateEntityDataFromSpecifics(specifics));
    } else if (key.starts_with(kEntryStorageKeyPrefix)) {
      const std::string entry_uuid = key.substr(kEntryStorageKeyPrefix.size());
      auto conversation_uuid =
          database_->GetConversationUuidForEntry(entry_uuid);
      if (!conversation_uuid) {
        continue;
      }
      auto conversation =
          FindSyncableConversation(*database_, *conversation_uuid);
      if (!conversation) {
        continue;
      }
      auto archive = database_->GetConversationData(*conversation_uuid);
      if (!archive) {
        continue;
      }
      auto it = std::ranges::find(archive->entries, entry_uuid,
                                  &mojom::ConversationTurn::uuid);
      if (it == archive->entries.end()) {
        continue;
      }
      auto specifics = BuildCommittableEntry(
          *conversation_uuid, **it,
          TimeToSend(database_->GetEntryModifiedTime(entry_uuid)),
          conversation->associated_content, ContentTextsFromArchive(*archive));
      if (!specifics) {
        // The entry cannot be made to fit the per-record budget, so there is
        // nothing to commit. Untrack it explicitly rather than leave it out of
        // the batch, which the processor reports as a bridge bug
        // (Sync.DataTypeOrphanMetadata.GetData). Untracking only forgets the
        // entity in memory, so the persisted metadata is cleared too.
        database_->ClearEntityMetadata(syncer::AI_CHAT_CONVERSATION, key);
        change_processor()->UntrackEntityForStorageKey(key);
        continue;
      }
      batch->Put(key, CreateEntityDataFromSpecifics(*specifics));
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

syncer::ConflictResolution AIChatSyncBridge::ResolveConflict(
    const std::string& storage_key,
    const syncer::EntityData& remote_data) const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // A tombstone carries no time, so the local change wins, as in the default
  // implementation. With no database, there is no local time to compare.
  if (remote_data.is_deleted() || !database_) {
    return DataTypeSyncBridge::ResolveConflict(storage_key, remote_data);
  }

  // The processor asks only when this device has a change it hasn't committed
  // yet. kUseRemote drops that commit and passes the record to
  // ApplyIncrementalSyncChanges(), so the answer follows what that does with
  // the record. For a newer local copy it would only queue the copy again. For
  // the same version it would write nothing, and the local change, such as new
  // token counts, would never be sent.
  const auto& specifics = remote_data.specifics.ai_chat_conversation();
  auto local = ReadLocalModifiedTime(*database_, specifics);
  if (!local) {
    // ApplyIncrementalSyncChanges() then reads again, and fails the batch if
    // it can't.
    return syncer::ConflictResolution::kUseRemote;
  }
  switch (FindNewerCopy(GetModifiedTimeFromSpecifics(specifics), *local)) {
    case NewerCopy::kRemote:
      return syncer::ConflictResolution::kUseRemote;
    case NewerCopy::kSameVersion:
    case NewerCopy::kLocal:
      return syncer::ConflictResolution::kUseLocal;
  }
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
  // Clearing every row covers each change the list holds.
  delete_metadata_change_list->DropAllChanges();
  if (!database_) {
    return;
  }
  database_->ClearAllEntityMetadata();
  database_->ClearDataTypeState(syncer::AI_CHAT_CONVERSATION);
}

void AIChatSyncBridge::OnConversationAdded(const std::string& uuid) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!database_ || !change_processor()->IsTrackingMetadata() ||
      ShouldSkipLocalPut(base::StrCat({kConversationStorageKeyPrefix, uuid}))) {
    return;
  }
  PutConversationMetadata(
      uuid, TimeToSend(database_->GetConversationModifiedTime(uuid)),
      *CreateMetadataStoreChangeList());
}

void AIChatSyncBridge::OnConversationModified(const std::string& uuid) {
  OnConversationAdded(uuid);
}

void AIChatSyncBridge::OnConversationDeleted(const std::string& uuid) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!database_ || !change_processor()->IsTrackingMetadata()) {
    return;
  }
  auto metadata_change_list = CreateMetadataStoreChangeList();

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
  if (!database_ || !change_processor()->IsTrackingMetadata() ||
      ShouldSkipLocalPut(base::StrCat({kEntryStorageKeyPrefix, entry_uuid}))) {
    return;
  }
  PutEntry(conversation_uuid, entry_uuid,
           TimeToSend(database_->GetEntryModifiedTime(entry_uuid)),
           *CreateMetadataStoreChangeList());
}

void AIChatSyncBridge::OnConversationEntryModified(
    const std::string& conversation_uuid,
    const std::string& entry_uuid) {
  OnConversationEntryAdded(conversation_uuid, entry_uuid);
}

void AIChatSyncBridge::OnConversationEntryDeleted(
    const std::string& entry_uuid) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!database_ || !change_processor()->IsTrackingMetadata()) {
    return;
  }
  auto metadata_change_list = CreateMetadataStoreChangeList();
  change_processor()->Delete(base::StrCat({kEntryStorageKeyPrefix, entry_uuid}),
                             syncer::DeletionOrigin::Unspecified(),
                             metadata_change_list.get());
}

bool AIChatSyncBridge::ApplyRemoteChanges(
    const syncer::EntityChangeList& changes,
    syncer::MetadataChangeList& metadata_change_list,
    KeptLocalCopies& kept_local_copies) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  base::flat_set<std::string> affected_conversation_uuids;
  for (auto it = changes.begin(); it != changes.end(); ++it) {
    const auto& change = *it;
    const bool ok =
        change->type() == syncer::EntityChange::ACTION_DELETE
            ? ApplyRemoteTombstone(change->storage_key(),
                                   affected_conversation_uuids)
            : ApplyRemoteRecord(change->data().specifics.ai_chat_conversation(),
                                kept_local_copies, affected_conversation_uuids);
    if (!ok) {
      // Leave the batch unsaved, so it downloads again on the next start.
      // The records from this one on weren't written, so a local update must
      // not save their new versions either.
      metadata_change_list.DropAllChanges();
      for (; it != changes.end(); ++it) {
        storage_keys_skipping_local_puts_.insert((*it)->storage_key());
      }
      NotifyRemoteChangesApplied(std::move(affected_conversation_uuids));
      return false;
    }
  }
  NotifyRemoteChangesApplied(std::move(affected_conversation_uuids));
  return true;
}

bool AIChatSyncBridge::ShouldSkipLocalPut(
    const std::string& storage_key) const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return skip_all_local_puts_ ||
         storage_keys_skipping_local_puts_.contains(storage_key);
}

bool AIChatSyncBridge::ApplyRemoteRecord(
    const sync_pb::AIChatConversationSpecifics& specifics,
    KeptLocalCopies& kept_local_copies,
    base::flat_set<std::string>& affected_conversation_uuids) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  auto local = ReadLocalModifiedTime(*database_, specifics);
  if (!local) {
    // Taking the failed read for no local copy would write the record over a
    // local copy that may be newer.
    return false;
  }
  // Stored as the record's own time, not the time it arrived. Otherwise this
  // device's copy would look newer than the server's.
  const base::Time modified_time = GetModifiedTimeFromSpecifics(specifics);
  switch (FindNewerCopy(modified_time, *local)) {
    case NewerCopy::kRemote:
      break;
    case NewerCopy::kSameVersion:
      // Already stored, as when this device's own records come back in a
      // first-sync merge. Token counts don't move a conversation's time, so
      // token counts changed alone on another device end here too. They're
      // written with that device's next rename or model change, and otherwise
      // update with this device's next reply.
      return true;
    case NewerCopy::kLocal:
      // Kept, and sent by the caller so the server catches up.
      kept_local_copies.emplace(GetStorageKeyFromSpecifics(specifics),
                                local->time);
      return true;
  }

  if (auto conversation = SpecificsToConversationMetadata(specifics)) {
    std::string uuid = conversation->uuid;
    if (!database_->ApplyRemoteConversationMetadata(std::move(conversation),
                                                    modified_time)) {
      return false;
    }
    affected_conversation_uuids.insert(std::move(uuid));
    return true;
  }

  std::vector<mojom::AssociatedContentPtr> associated_content;
  base::flat_map<std::string, std::string> associated_content_texts;
  auto entry = SpecificsToEntry(specifics, associated_content,
                                &associated_content_texts);
  if (!entry) {
    return true;
  }

  // An associated content uuid is unique across conversations, but
  // AIChatDatabase::AddOrUpdateAssociatedContent() updates only a row in the
  // same conversation, so a uuid another conversation holds would collide on
  // insert. A failed lookup reads as no owner, which leaves the write to report
  // any collision.
  const std::string& conversation_uuid = specifics.entry().conversation_uuid();
  for (const auto& content : associated_content) {
    auto owner =
        database_->GetConversationUuidForAssociatedContent(content->uuid);
    if (owner && *owner != conversation_uuid) {
      return true;
    }
  }

  // TODO(https://github.com/brave/brave-browser/issues/53978): carry this
  // device's edit revisions, and the values the sender omitted for size, over
  // from the local entry. The write replaces the whole entry, so until then
  // both are lost.
  std::vector<std::string> contents = base::ToVector(
      associated_content, [&](const mojom::AssociatedContentPtr& content) {
        const std::string* text =
            base::FindOrNull(associated_content_texts, content->uuid);
        return text ? *text : std::string();
      });
  if (!database_->ApplyRemoteEntry(conversation_uuid, std::move(entry),
                                   modified_time, std::move(associated_content),
                                   std::move(contents))) {
    return false;
  }
  affected_conversation_uuids.insert(conversation_uuid);
  return true;
}

bool AIChatSyncBridge::ApplyRemoteTombstone(
    std::string_view storage_key,
    base::flat_set<std::string>& affected_conversation_uuids) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // Tombstones bypass IsEntityDataValid(), so the uuid taken from the storage
  // key isn't validated. Skip an empty one rather than pass it to the
  // database, which requires a non-empty uuid.
  if (auto uuid =
          base::RemovePrefix(storage_key, kConversationStorageKeyPrefix)) {
    if (uuid->empty()) {
      return true;
    }
    if (!database_->DeleteConversation(*uuid)) {
      return false;
    }
    affected_conversation_uuids.emplace(*uuid);
    return true;
  }
  if (auto uuid = base::RemovePrefix(storage_key, kEntryStorageKeyPrefix)) {
    if (uuid->empty()) {
      return true;
    }
    // A tombstone carries only the entry's uuid, so look up its conversation
    // while the row is still there.
    auto conversation_uuid = database_->GetConversationUuidForEntry(*uuid);
    if (!database_->DeleteConversationEntry(*uuid)) {
      return false;
    }
    if (conversation_uuid) {
      affected_conversation_uuids.insert(std::move(*conversation_uuid));
    }
  }
  return true;
}

void AIChatSyncBridge::NotifyRemoteChangesApplied(
    base::flat_set<std::string> affected_conversation_uuids) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!affected_conversation_uuids.empty()) {
    on_remote_changes_applied_.Run(
        std::move(affected_conversation_uuids).extract());
  }
}

void AIChatSyncBridge::PutKeptLocalCopies(KeptLocalCopies kept_local_copies) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!database_ || !change_processor()->IsTrackingMetadata()) {
    return;
  }
  auto metadata_change_list = CreateMetadataStoreChangeList();
  for (const auto& [storage_key, modified_time] : kept_local_copies) {
    if (ShouldSkipLocalPut(storage_key)) {
      continue;
    }
    // A local change or a newer remote record may have replaced the copy
    // since it was kept. A local change sends itself, and a remote record
    // needn't be sent back, so only a copy that is still the one kept is
    // sent.
    if (auto uuid =
            base::RemovePrefix(storage_key, kConversationStorageKeyPrefix)) {
      if (IsSameLocalCopy(database_->GetConversationModifiedTime(*uuid),
                          modified_time)) {
        PutConversationMetadata(std::string(*uuid), modified_time,
                                *metadata_change_list);
      }
    } else if (auto entry_uuid =
                   base::RemovePrefix(storage_key, kEntryStorageKeyPrefix)) {
      if (!IsSameLocalCopy(database_->GetEntryModifiedTime(*entry_uuid),
                           modified_time)) {
        continue;
      }
      if (auto conversation_uuid =
              database_->GetConversationUuidForEntry(*entry_uuid)) {
        PutEntry(*conversation_uuid, std::string(*entry_uuid), modified_time,
                 *metadata_change_list);
      }
    }
  }
}

void AIChatSyncBridge::PutConversationMetadata(
    const std::string& uuid,
    base::Time modified_time,
    syncer::MetadataChangeList& metadata_change_list) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  auto conversation = FindSyncableConversation(*database_, uuid);
  if (!conversation) {
    return;
  }
  auto specifics =
      ConversationMetadataToSpecifics(*conversation, modified_time);
  change_processor()->Put(base::StrCat({kConversationStorageKeyPrefix, uuid}),
                          CreateEntityDataFromSpecifics(specifics),
                          &metadata_change_list);
}

void AIChatSyncBridge::PutEntry(
    const std::string& conversation_uuid,
    const std::string& entry_uuid,
    base::Time modified_time,
    syncer::MetadataChangeList& metadata_change_list) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
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
  auto specifics = BuildCommittableEntry(conversation_uuid, **it, modified_time,
                                         conversation->associated_content,
                                         ContentTextsFromArchive(*archive));
  if (!specifics) {
    // Too large to commit even after omitting every omittable field, so there
    // is nothing to Put. Leave the entity tracked: untracking one committed
    // earlier would turn a later local delete into a no-op and strand the
    // server's copy. GetDataForCommit() untracks it if a commit asks for it.
    return;
  }
  change_processor()->Put(base::StrCat({kEntryStorageKeyPrefix, entry_uuid}),
                          CreateEntityDataFromSpecifics(*specifics),
                          &metadata_change_list);
}

}  // namespace ai_chat
