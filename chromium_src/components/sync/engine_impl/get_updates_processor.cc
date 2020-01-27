/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/sync/engine_impl/get_updates_processor.h"

#include <algorithm>
#include <memory>
#include <string>
#include <utility>

namespace syncer {
namespace {
SyncerError ApplyBraveRecords(sync_pb::ClientToServerResponse*,
                              ModelTypeSet*,
                              std::unique_ptr<brave_sync::RecordsList>);
}  // namespace
}  // namespace syncer

#include "../../../../../components/sync/engine_impl/get_updates_processor.cc"  // NOLINT
#include "base/base64.h"
#include "base/guid.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"
#include "brave/components/brave_sync/jslib_messages.h"
#include "components/sync/base/hash_util.h"
#include "components/sync/base/system_encryptor.h"
#include "components/sync/base/time.h"
#include "components/sync/engine_impl/loopback_server/loopback_server_entity.h"
#include "components/sync/syncable/directory_cryptographer.h"
#include "components/sync/syncable/syncable_proto_util.h"
#include "url/gurl.h"

namespace syncer {
namespace {

using brave_sync::jslib::Bookmark;
using brave_sync::jslib::SyncRecord;
using syncable::Id;
static const char kBookmarkBarFolderServerTag[] = "bookmark_bar";
static const char kBookmarkBarFolderName[] = "Bookmark Bar";
static const char kOtherBookmarksFolderServerTag[] = "other_bookmarks";
static const char kOtherBookmarksFolderName[] = "Other Bookmarks";
static const char kSyncedBookmarksFolderServerTag[] = "synced_bookmarks";
static const char kSyncedBookmarksFolderName[] = "Synced Bookmarks";
// The parent tag for children of the root entity. Entities with this parent are
// referred to as top level enities.
static const char kRootParentTag[] = "0";

void AddBookmarkSpecifics(sync_pb::EntitySpecifics* specifics,
                          const SyncRecord* record) {
  DCHECK(specifics);
  auto bookmark = record->GetBookmark();
  sync_pb::BookmarkSpecifics* bm_specifics = specifics->mutable_bookmark();
  bm_specifics->set_url(bookmark.site.location);

  bm_specifics->set_title(bookmark.site.TryGetNonEmptyTitle());
  bm_specifics->set_creation_time_us(
      TimeToProtoTime(bookmark.site.creationTime));
  // base::Time::Now().ToDeltaSinceWindowsEpoch().InMicroseconds());
  sync_pb::MetaInfo* meta_info = bm_specifics->add_meta_info();
  meta_info->set_key("order");
  meta_info->set_value(bookmark.order);
  // For GetExistingObjects lookup
  meta_info = bm_specifics->add_meta_info();
  meta_info->set_key("object_id");
  meta_info->set_value(record->objectId);
  meta_info = bm_specifics->add_meta_info();
  meta_info->set_key("parent_object_id");
  meta_info->set_value(bookmark.parentFolderObjectId);
  meta_info = bm_specifics->add_meta_info();
  meta_info->set_key("sync_timestamp");
  meta_info->set_value(std::to_string(record->syncTimestamp.ToJsTime()));
}

void ExtractBookmarkMeta(sync_pb::SyncEntity* entity,
                         sync_pb::EntitySpecifics* specifics,
                         const Bookmark& bookmark) {
  sync_pb::BookmarkSpecifics* bm_specifics = specifics->mutable_bookmark();
  for (const auto metaInfo : bookmark.metaInfo) {
    // version need to be incremented
    if (metaInfo.key != "version") {
      sync_pb::MetaInfo* meta_info = bm_specifics->add_meta_info();
      meta_info->set_key(metaInfo.key);
      meta_info->set_value(metaInfo.value);
    }
    if (metaInfo.key == "originator_cache_guid") {
      entity->set_originator_cache_guid(metaInfo.value);
    } else if (metaInfo.key == "originator_client_item_id") {
      entity->set_originator_client_item_id(metaInfo.value);
    } else if (metaInfo.key == "version") {
      int64_t version;
      bool result = base::StringToInt64(metaInfo.value, &version);
      DCHECK(result);
      entity->set_version(++version);
      sync_pb::MetaInfo* meta_info = bm_specifics->add_meta_info();
      meta_info->set_key(metaInfo.key);
      meta_info->set_value(std::to_string(version));
    } else if (metaInfo.key == "position_in_parent") {
      int64_t position_in_parent;
      bool result = base::StringToInt64(metaInfo.value, &position_in_parent);
      DCHECK(result);
      entity->set_position_in_parent(position_in_parent);
    }
  }
  DCHECK(entity->has_version());
}

void MigrateFromLegacySync(sync_pb::SyncEntity* entity) {
  if (!entity->has_originator_cache_guid()) {
    entity->set_originator_cache_guid("legacy_originator_cache_guid");
  }
  if (!entity->has_originator_client_item_id()) {
    entity->set_originator_client_item_id(base::GenerateGUID());
  }
  if (!entity->has_position_in_parent()) {
    entity->set_position_in_parent(0);
  }
}

void AddRootForType(sync_pb::SyncEntity* entity, ModelType type) {
  DCHECK(entity);
  sync_pb::EntitySpecifics specifics;
  AddDefaultFieldValue(type, &specifics);
  std::string server_tag = ModelTypeToRootTag(type);
  std::string name = syncer::ModelTypeToString(type);
  std::string id = LoopbackServerEntity::GetTopLevelId(type);
  entity->set_server_defined_unique_tag(server_tag);
  entity->set_id_string(id);
  entity->set_parent_id_string(kRootParentTag);
  entity->set_name(name);
  entity->set_version(1);
  entity->set_folder(true);
  entity->mutable_specifics()->CopyFrom(specifics);
}

void AddPermanentNode(sync_pb::SyncEntity* entity,
                      const std::string& name,
                      const std::string& tag) {
  DCHECK(entity);
  sync_pb::EntitySpecifics specifics;
  AddDefaultFieldValue(BOOKMARKS, &specifics);
  std::string parent = ModelTypeToRootTag(BOOKMARKS);
  std::string id = tag;
  std::string parent_id = LoopbackServerEntity::CreateId(BOOKMARKS, parent);
  entity->set_server_defined_unique_tag(tag);
  entity->set_id_string(id);
  entity->set_parent_id_string(parent_id);
  entity->set_name(name);
  entity->set_folder(true);
  entity->set_version(1);
  entity->mutable_specifics()->CopyFrom(specifics);
}

void AddBookmarkNode(sync_pb::SyncEntity* entity, const SyncRecord* record) {
  DCHECK(entity);
  DCHECK(record);
  DCHECK(record->has_bookmark());
  DCHECK(!record->objectId.empty());

  auto bookmark_record = record->GetBookmark();

  sync_pb::EntitySpecifics specifics;
  AddDefaultFieldValue(BOOKMARKS, &specifics);
  entity->set_id_string(record->objectId);
  if (!bookmark_record.parentFolderObjectId.empty())
    entity->set_parent_id_string(bookmark_record.parentFolderObjectId);
  else
    entity->set_parent_id_string(std::string(kBookmarkBarFolderServerTag));
  entity->set_non_unique_name(bookmark_record.site.TryGetNonEmptyTitle());
  entity->set_folder(bookmark_record.isFolder);

  ExtractBookmarkMeta(entity, &specifics, bookmark_record);

  MigrateFromLegacySync(entity);

  if (record->action == SyncRecord::Action::A_DELETE)
    entity->set_deleted(true);
  else
    AddBookmarkSpecifics(&specifics, record);
  entity->mutable_specifics()->CopyFrom(specifics);
}

void ConstructUpdateResponse(sync_pb::GetUpdatesResponse* gu_response,
                             ModelTypeSet* request_types,
                             std::unique_ptr<RecordsList> records) {
  DCHECK(gu_response);
  DCHECK(request_types);
  for (ModelType type : *request_types) {
    sync_pb::DataTypeProgressMarker* marker =
        gu_response->add_new_progress_marker();
    marker->set_data_type_id(GetSpecificsFieldNumberFromModelType(type));
    marker->set_token("token");
    if (type == BOOKMARKS) {
      google::protobuf::RepeatedPtrField<sync_pb::SyncEntity> entities;
      AddRootForType(entities.Add(), BOOKMARKS);
      AddPermanentNode(entities.Add(), kBookmarkBarFolderName,
                       kBookmarkBarFolderServerTag);
      AddPermanentNode(entities.Add(), kOtherBookmarksFolderName,
                       kOtherBookmarksFolderServerTag);
      // required since 84f01c4c006cf89941138f3591db129a5b3cde54
      AddPermanentNode(entities.Add(), kSyncedBookmarksFolderName,
                       kSyncedBookmarksFolderServerTag);
      if (records) {
        for (const auto& record : *records.get()) {
          AddBookmarkNode(entities.Add(), record.get());
        }
      }
      std::copy(entities.begin(), entities.end(),
                RepeatedPtrFieldBackInserter(gu_response->mutable_entries()));
    } else if (type == NIGORI) {
      google::protobuf::RepeatedPtrField<sync_pb::SyncEntity> entities;
      sync_pb::EntitySpecifics specifics;
      AddDefaultFieldValue(NIGORI, &specifics);
      sync_pb::SyncEntity* entity = entities.Add();
      AddRootForType(entity, NIGORI);
      sync_pb::NigoriSpecifics* nigori = specifics.mutable_nigori();
      nigori->set_encrypt_everything(false);
      nigori->set_encrypt_bookmarks(false);
      syncer::DirectoryCryptographer cryptographer;
      KeyParams params = {KeyDerivationParams::CreateForPbkdf2(), "foobar"};
      syncer::KeyDerivationMethod method = params.derivation_params.method();
      bool add_key_result = cryptographer.AddKey(params);
      DCHECK(add_key_result);
      bool get_keys_result =
          cryptographer.GetKeys(nigori->mutable_encryption_keybag());
      DCHECK(get_keys_result);
      nigori->set_keybag_is_frozen(true);
      nigori->set_keystore_migration_time(1U);
      nigori->set_passphrase_type(sync_pb::NigoriSpecifics::CUSTOM_PASSPHRASE);
      nigori->set_custom_passphrase_key_derivation_method(
          EnumKeyDerivationMethodToProto(method));
      entity->mutable_specifics()->CopyFrom(specifics);

      std::copy(entities.begin(), entities.end(),
                RepeatedPtrFieldBackInserter(gu_response->mutable_entries()));
    }
  }
  gu_response->set_changes_remaining(0);
  gu_response->add_encryption_keys("dummy_encryption_key");
}

SyncerError ApplyBraveRecords(sync_pb::ClientToServerResponse* update_response,
                              ModelTypeSet* request_types,
                              std::unique_ptr<RecordsList> records) {
  DCHECK(update_response);
  DCHECK(request_types);
  sync_pb::GetUpdatesResponse* gu_response = new sync_pb::GetUpdatesResponse();
  ConstructUpdateResponse(gu_response, request_types, std::move(records));
  update_response->set_allocated_get_updates(gu_response);
  return SyncerError(SyncerError::SYNCER_OK);
}

}  // namespace

void GetUpdatesProcessor::AddBraveRecords(
    std::unique_ptr<RecordsList> records) {
  brave_records_ = std::move(records);
}

}  // namespace syncer
