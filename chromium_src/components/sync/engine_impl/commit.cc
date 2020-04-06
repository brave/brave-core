/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

namespace sync_pb {
class ClientToServerMessage;
class ClientToServerResponse;
}  // namespace sync_pb

namespace syncer {
class SyncCycle;
class SyncerError;
namespace {

SyncerError PostBraveCommit(sync_pb::ClientToServerMessage* message,
                            sync_pb::ClientToServerResponse* response,
                            SyncCycle* cycle);
}

}  // namespace syncer

#define BRAVE_COMMIT_POST_AND_PROCESS_RESPONSE \
  PostBraveCommit(&message_, &response, cycle);

#include "../../../../../components/sync/engine_impl/commit.cc"  // NOLINT
#undef BRAVE_COMMIT_POST_AND_PROCESS_RESPONSE

#include "base/base64.h"
#include "brave/components/brave_sync/jslib_const.h"
#include "brave/components/brave_sync/jslib_messages.h"
#include "brave/components/brave_sync/jslib_messages_fwd.h"
#include "brave/components/brave_sync/tools.h"
#include "components/sync/base/time.h"
#include "components/sync/base/unique_position.h"

namespace syncer {
namespace {
using brave_sync::jslib::MetaInfo;
using brave_sync::jslib::SyncRecord;
// from components/sync_bookmarks/bookmark_model_merger.cc
const char kOtherBookmarksTag[] = "other_bookmarks";

void CreateSuccessfulCommitResponse(
    const sync_pb::SyncEntity& entity,
    sync_pb::CommitResponse::EntryResponse* response,
    const std::string& new_object_id) {
  response->set_response_type(sync_pb::CommitResponse::SUCCESS);
  response->set_non_unique_name(entity.name());
  response->set_version(entity.version() + 1);
  response->set_parent_id_string(entity.parent_id_string());

  if (new_object_id.empty())
    response->set_id_string(entity.id_string());
  else
    response->set_id_string(new_object_id);
}

std::unique_ptr<SyncRecord> CreateOtherBookmarksRecord(SyncRecord* child) {
  auto record = std::make_unique<SyncRecord>();
  record->objectData = brave_sync::jslib_const::SyncObjectData_BOOKMARK;
  auto bookmark = std::make_unique<brave_sync::jslib::Bookmark>();
  bookmark->site.title = brave_sync::tools::kOtherNodeName;
  bookmark->site.customTitle = brave_sync::tools::kOtherNodeName;
  // Special order reserved for "Other Bookmarks" folder, it only has effect on
  // mobile. On desktop, it is used to distinguish "Other Bookmarks" from normal
  // same name folder
  bookmark->order = brave_sync::tools::kOtherNodeOrder;
  bookmark->site.creationTime = child->GetBookmark().site.creationTime;
  bookmark->isFolder = true;

  DCHECK(child->has_bookmark());
  record->objectId = child->GetBookmark().parentFolderObjectId;
  record->action = brave_sync::jslib::SyncRecord::Action::A_CREATE;
  record->syncTimestamp = child->syncTimestamp;
  record->SetBookmark(std::move(bookmark));

  return record;
}

brave_sync::RecordsListPtr ConvertCommitsToBraveRecords(
    sync_pb::ClientToServerMessage* message,
    sync_pb::ClientToServerResponse* response) {
  brave_sync::RecordsListPtr record_list =
      std::make_unique<brave_sync::RecordsList>();
  const sync_pb::CommitMessage& commit_message = message->commit();
  const std::string cache_guid = commit_message.cache_guid();
  bool other_bookmarks_record_created = false;
  for (int i = 0; i < commit_message.entries_size(); ++i) {
    sync_pb::SyncEntity entity = commit_message.entries(i);
    std::string new_object_id;
    if (entity.specifics().has_bookmark()) {
      const sync_pb::BookmarkSpecifics& bm_specifics =
          entity.specifics().bookmark();

      auto record = std::make_unique<SyncRecord>();
      record->objectData = brave_sync::jslib_const::SyncObjectData_BOOKMARK;

      auto bookmark = std::make_unique<brave_sync::jslib::Bookmark>();
      bookmark->site.location = bm_specifics.url();
      bookmark->site.title = bm_specifics.legacy_canonicalized_title();
      bookmark->site.customTitle = bm_specifics.legacy_canonicalized_title();
      // bookmark->site.lastAccessedTime - ignored
      bookmark->site.creationTime =
          ProtoTimeToTime(bm_specifics.creation_time_us());
      bookmark->site.favicon = bm_specifics.icon_url();
      bookmark->isFolder = entity.folder();
      // only matters for direct children of permanent nodes
      bookmark->hideInToolbar = entity.parent_id_string() == kOtherBookmarksTag;

      bool skip_record = false;
      for (int i = 0; i < bm_specifics.meta_info_size(); ++i) {
        if (bm_specifics.meta_info(i).key() == "order") {
          bookmark->order = bm_specifics.meta_info(i).value();
        } else if (bm_specifics.meta_info(i).key() == "object_id") {
          new_object_id = bm_specifics.meta_info(i).value();
        } else if (bm_specifics.meta_info(i).key() == "parent_object_id") {
          bookmark->parentFolderObjectId = bm_specifics.meta_info(i).value();
        } else if (bm_specifics.meta_info(i).key() == "sync_timestamp") {
          record->syncTimestamp = base::Time::FromJsTime(
              std::stod(bm_specifics.meta_info(i).value()));
        } else if (bm_specifics.meta_info(i).key() == "last_send_time" &&
                   entity.version() == 0) {
          // Upgrade from legacy code, we need to prevent sending duplicate
          // records which are already on sync chain
          skip_record = true;
        }
      }

      int64_t version = entity.version();
      if (entity.version() == 0) {
        record->objectId = new_object_id;
        record->action = brave_sync::jslib::SyncRecord::Action::A_CREATE;
      } else {
        record->objectId = entity.id_string();
        if (entity.deleted())
          record->action = brave_sync::jslib::SyncRecord::Action::A_DELETE;
        else
          record->action = brave_sync::jslib::SyncRecord::Action::A_UPDATE;
      }

      if (entity.deleted() && entity.version() == 0 &&
        record->objectId.empty()) {
        // This is for recover profile after crash with duplicated object ids
        // When doing delete of duplicated bookmark, pretend it has a new
        // object id to got through nudge/pull cycle
        // We are ok, if other devices would get this record, because they have
        // nothing to delete in fact
        record->objectId = brave_sync::tools::GenerateObjectId();
        record->action = brave_sync::jslib::SyncRecord::Action::A_DELETE;
      }

      DCHECK(!record->objectId.empty());

      MetaInfo metaInfo;

      metaInfo.key = "version";
      metaInfo.value = std::to_string(version);
      bookmark->metaInfo.push_back(metaInfo);

      metaInfo.key = "position_in_parent";
      metaInfo.value = std::to_string(entity.position_in_parent());
      bookmark->metaInfo.push_back(metaInfo);

      record->SetBookmark(std::move(bookmark));

      if (!other_bookmarks_record_created &&
          entity.parent_id_string() == kOtherBookmarksTag) {
        record_list->push_back(CreateOtherBookmarksRecord(record.get()));
        other_bookmarks_record_created = true;
      }
      if (!skip_record)
        record_list->push_back(std::move(record));
    }
    sync_pb::CommitResponse_EntryResponse* entry_response =
        response->mutable_commit()->add_entryresponse();
    CreateSuccessfulCommitResponse(entity, entry_response, new_object_id);
  }
  return record_list;
}

SyncerError PostBraveCommit(sync_pb::ClientToServerMessage* message,
                            sync_pb::ClientToServerResponse* response,
                            SyncCycle* cycle) {
  brave_sync::RecordsListPtr records_list =
      ConvertCommitsToBraveRecords(message, response);
  cycle->delegate()->OnNudgeSyncCycle(std::move(records_list));

  return SyncerError(SyncerError::SYNCER_OK);
}

}  // namespace
}  // namespace syncer
