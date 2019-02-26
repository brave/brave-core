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

}   // namespace syncer

#include "../../../../../components/sync/engine_impl/commit.cc"
#include "brave/components/brave_sync/jslib_const.h"
#include "brave/components/brave_sync/jslib_messages_fwd.h"
#include "brave/components/brave_sync/jslib_messages.h"
#include "components/sync/base/time.h"
#include "components/sync/base/unique_position.h"

namespace syncer {
namespace {
using brave_sync::jslib::SyncRecord;
using brave_sync::jslib::MetaInfo;
const char kBookmarkBarTag[] = "bookmark_bar";

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

brave_sync::RecordsListPtr
ConvertCommitsToBraveRecords(sync_pb::ClientToServerMessage* message,
                             sync_pb::ClientToServerResponse* response) {
  brave_sync::RecordsListPtr record_list =
    std::make_unique<brave_sync::RecordsList>();
  const sync_pb::CommitMessage& commit_message = message->commit();
  const std::string cache_guid =  commit_message.cache_guid();
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
      bookmark->site.title = bm_specifics.title();
      bookmark->site.customTitle = bm_specifics.title();
      // bookmark->site.lastAccessedTime - ignored
      bookmark->site.creationTime =
        ProtoTimeToTime(bm_specifics.creation_time_us());
      bookmark->site.favicon = bm_specifics.icon_url();
      bookmark->isFolder = entity.folder();
      // only mattters for direct children of permanent nodes
      bookmark->hideInToolbar = entity.parent_id_string() != kBookmarkBarTag;

      std::string originator_cache_guid;
      std::string originator_client_item_id;
      for (int i = 0; i < bm_specifics.meta_info_size(); ++i) {
        if (bm_specifics.meta_info(i).key() == "order") {
          bookmark->order = bm_specifics.meta_info(i).value();
        } else if (bm_specifics.meta_info(i).key() == "prev_object_id") {
          bookmark->prevObjectId = bm_specifics.meta_info(i).value();
        } else if (bm_specifics.meta_info(i).key() == "prev_order") {
          bookmark->prevOrder = bm_specifics.meta_info(i).value();
        } else if (bm_specifics.meta_info(i).key() == "next_order") {
          bookmark->nextOrder = bm_specifics.meta_info(i).value();
        } else if (bm_specifics.meta_info(i).key() == "parent_order") {
          bookmark->parentOrder = bm_specifics.meta_info(i).value();
        } else if (bm_specifics.meta_info(i).key() == "object_id") {
          new_object_id = bm_specifics.meta_info(i).value();
        } else if (bm_specifics.meta_info(i).key() == "parent_object_id") {
          bookmark->parentFolderObjectId = bm_specifics.meta_info(i).value();
        } else if (bm_specifics.meta_info(i).key() == "sync_timestamp") {
          record->syncTimestamp = base::Time::FromJsTime(
                                    std::stod(
                                      bm_specifics.meta_info(i).value()));
        } else if (bm_specifics.meta_info(i).key() == "originator_cache_guid") {
          originator_cache_guid = bm_specifics.meta_info(i).value();
        } else if (bm_specifics.meta_info(i).key() ==
                   "originator_client_item_id") {
          originator_client_item_id = bm_specifics.meta_info(i).value();
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

      DCHECK(!record->objectId.empty());

      MetaInfo metaInfo;
      metaInfo.key = "originator_cache_guid";
      if (originator_cache_guid.empty()) {
        originator_cache_guid = cache_guid;
      }
      metaInfo.value = originator_cache_guid;
      bookmark->metaInfo.push_back(metaInfo);

      metaInfo.key = "originator_client_item_id";
      if (originator_client_item_id.empty()) {
        originator_client_item_id = entity.id_string();
      }
      metaInfo.value = originator_client_item_id;
      bookmark->metaInfo.push_back(metaInfo);

      metaInfo.key = "version";
      metaInfo.value = std::to_string(version);
      bookmark->metaInfo.push_back(metaInfo);

      metaInfo.key = "mtime";
      metaInfo.value = std::to_string(entity.mtime());
      bookmark->metaInfo.push_back(metaInfo);

      metaInfo.key = "ctime";
      metaInfo.value = std::to_string(entity.ctime());
      bookmark->metaInfo.push_back(metaInfo);

      record->SetBookmark(std::move(bookmark));
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

}
}   // namespace syncer
