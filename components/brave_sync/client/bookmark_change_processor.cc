/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_sync/client/bookmark_change_processor.h"

#include "base/strings/utf_string_conversions.h"
#include "brave/components/brave_sync/bookmark_order_util.h"
#include "brave/components/brave_sync/jslib_const.h"
#include "brave/components/brave_sync/jslib_messages.h"
#include "brave/components/brave_sync/tools.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "chrome/browser/bookmarks/bookmark_model_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "components/bookmarks/browser/bookmark_storage.h"
#include "components/bookmarks/browser/bookmark_utils.h"
#include "ui/base/models/tree_node_iterator.h"

using bookmarks::BookmarkNode;
using bookmarks::BookmarkModel;

namespace {

class ScopedPauseObserver {
 public:
  ScopedPauseObserver(brave_sync::BookmarkChangeProcessor* processor) :
      processor_(processor) {
    DCHECK_NE(processor_, nullptr);
    processor_->Stop();
  }
  ~ScopedPauseObserver() {
    processor_->Start();
  }

 private:
  brave_sync::BookmarkChangeProcessor* processor_;  // Not owned
};

}  // namespace

namespace brave_sync {

int64_t deleted_node_id = -1;
bookmarks::BookmarkNode* deleted_node_root;

bool IsSyncManagedNode(const bookmarks::BookmarkPermanentNode* node) {
  return node->id() == deleted_node_id;
}

bookmarks::BookmarkPermanentNodeList
LoadExtraNodes(bookmarks::LoadExtraCallback callback,
               int64_t* next_node_id) {
  // TODO(bridiver) - deleted node should not be visible
  bookmarks::BookmarkPermanentNodeList extra_nodes;
  if (callback)
    extra_nodes = std::move(callback).Run(next_node_id);

  auto node = std::make_unique<bookmarks::BookmarkPermanentNode>(*next_node_id);
  deleted_node_id = *next_node_id;
  *next_node_id = deleted_node_id + 1;
  node->set_type(bookmarks::BookmarkNode::FOLDER);
  node->set_visible(false);
  node->SetTitle(base::UTF8ToUTF16("Deleted Bookmarks"));

  extra_nodes.push_back(std::move(node));

  return extra_nodes;
}

namespace {

void GetOrder(const bookmarks::BookmarkNode* parent,
              int index,
              std::string* prev_order,
              std::string* next_order,
              std::string* parent_order) {
  auto* prev_node = index == 0 ?
    nullptr :
    parent->GetChild(index - 1);
  auto* next_node = index == parent->child_count() - 1 ?
    nullptr :
    parent->GetChild(index + 1);

  if (prev_node)
    prev_node->GetMetaInfo("order", prev_order);

  if (next_node)
    next_node->GetMetaInfo("order", next_order);

  parent->GetMetaInfo("order", parent_order);
}

const bookmarks::BookmarkNode* FindByObjectId(bookmarks::BookmarkModel* model,
                                        const std::string& object_id) {
  ui::TreeNodeIterator<const bookmarks::BookmarkNode>
      iterator(model->root_node());
  while (iterator.has_next()) {
    const bookmarks::BookmarkNode* node = iterator.Next();
    std::string node_object_id;
    node->GetMetaInfo("object_id", &node_object_id);

    if (!node_object_id.empty() && object_id == node_object_id)
      return node;
  }
  return nullptr;
}

uint64_t GetIndex(const bookmarks::BookmarkNode* root_node,
                  const jslib::Bookmark& record) {
  uint64_t index = 0;
  ui::TreeNodeIterator<const bookmarks::BookmarkNode> iterator(root_node);
  while (iterator.has_next()) {
    const bookmarks::BookmarkNode* node = iterator.Next();

    if (node->parent() != root_node)
      return index;

    std::string node_order;
    node->GetMetaInfo("order", &node_order);

    if (!node_order.empty() &&
        brave_sync::CompareOrder(record.order, node_order))
      return index;

    ++index;
  }
  return index;
}

// this should only be called for resolved records we get from the server
void UpdateNode(bookmarks::BookmarkModel* model,
                const bookmarks::BookmarkNode* node,
                const jslib::SyncRecord* record) {
  auto bookmark = record->GetBookmark();
  if (bookmark.isFolder) {
    // SetDateFolderModified
  } else {
    model->SetURL(node, GURL(bookmark.site.location));
    // TODO, AB: apply these:
    // sync_bookmark.site.customTitle
    // sync_bookmark.site.lastAccessedTime
    // sync_bookmark.site.favicon
  }

  model->SetTitle(node,
      base::UTF8ToUTF16(bookmark.site.title));
  model->SetDateAdded(node, bookmark.site.creationTime);
  model->SetNodeMetaInfo(node, "object_id", record->objectId);
  model->SetNodeMetaInfo(node, "order", bookmark.order);

  // updating the sync_timestamp marks this record as synced
  if (!tools::IsTimeEmpty(record->syncTimestamp)) {
    model->SetNodeMetaInfo(node,
        "sync_timestamp",
        std::to_string(record->syncTimestamp.ToJsTime()));
    model->DeleteNodeMetaInfo(node, "last_send_time");

    std::string last_updated_time;
    node->GetMetaInfo("last_updated_time", &last_updated_time);
    if (std::stod(last_updated_time) < record->syncTimestamp.ToJsTime()) {
      LOG(ERROR) << "update last updated time";
      model->SetNodeMetaInfo(node, "last_updated_time",
          std::to_string(record->syncTimestamp.ToJsTime()));
    }
  }
}

const bookmarks::BookmarkNode* FindParent(bookmarks::BookmarkModel* model,
                                          const jslib::Bookmark& bookmark) {
  auto* parent_node = FindByObjectId(model, bookmark.parentFolderObjectId);

  if (!parent_node) {
    if (!bookmark.order.empty() &&
        bookmark.order.at(0) == '2') {
      // mobile generated bookmarks go in the mobile folder so they don't
      // get so we don't get m.xxx.xxx domains in the normal bookmarks
      parent_node = model->mobile_node();
    } else if (!bookmark.hideInToolbar) {
      // this flag is a bit odd, but if the node doesn't have a parent and
      // hideInToolbar is false, then this bookmark should go in the
      // toolbar root. We don't care about this flag for records with
      // a parent id because they will be inserted into the correct
      // parent folder
      parent_node = model->bookmark_bar_node();
    } else {
      parent_node = model->other_node();
    }
  }

  return parent_node;
}

}  // namespace

// static
BookmarkChangeProcessor* BookmarkChangeProcessor::Create(
    Profile* profile,
    BraveSyncClient* sync_client,
    prefs::Prefs* sync_prefs) {
  return new BookmarkChangeProcessor(profile, sync_client, sync_prefs);
}

BookmarkChangeProcessor::BookmarkChangeProcessor(
    Profile* profile,
    BraveSyncClient* sync_client,
    prefs::Prefs* sync_prefs)
    : sync_client_(sync_client),
      sync_prefs_(sync_prefs),
      bookmark_model_(BookmarkModelFactory::GetForBrowserContext(
          Profile::FromBrowserContext(profile))) {
  DCHECK(sync_client_);
  DCHECK(sync_prefs);
  DCHECK(bookmark_model_);
}

BookmarkChangeProcessor::~BookmarkChangeProcessor() {
  Stop();
}

void BookmarkChangeProcessor::Start() {
  bookmark_model_->AddObserver(this);
}

void BookmarkChangeProcessor::Stop() {
  if (bookmark_model_)
    bookmark_model_->RemoveObserver(this);
}

void BookmarkChangeProcessor::BookmarkModelLoaded(BookmarkModel* model,
                                                  bool ids_reassigned) {
  NOTREACHED();
}

void BookmarkChangeProcessor::BookmarkModelBeingDeleted(bookmarks::BookmarkModel* model) {
  NOTREACHED();
  bookmark_model_ = nullptr;
}

void BookmarkChangeProcessor::BookmarkNodeAdded(BookmarkModel* model,
                                                const BookmarkNode* parent,
                                                int index) {
  auto* node = parent->GetChild(index);

  std::string prev_node_order;
  std::string next_node_order;
  std::string parent_node_order;
  GetOrder(parent, index,
           &prev_node_order, &next_node_order, &parent_node_order);

  // this is a giant hack and have an empty value for all 3 should
  // be handled in the sync js lib
  if (parent_node_order.empty())
    parent_node_order =
        sync_prefs_->GetBookmarksBaseOrder() + std::to_string(index);

  PushRRContext(
      prev_node_order, next_node_order, parent_node_order,
      node->id(), jslib_const::kActionCreate);
  sync_client_->SendGetBookmarkOrder(
      prev_node_order, next_node_order, parent_node_order);
  // responds in OnGetBookmarkOrder
}

void BookmarkChangeProcessor::OnWillRemoveBookmarks(BookmarkModel* model,
                                                    const BookmarkNode* parent,
                                                    int old_index,
                                                    const BookmarkNode* node) {
}


void BookmarkChangeProcessor::CloneBookmarkNodeForDeleteImpl(
    const bookmarks::BookmarkNodeData::Element& element,
    bookmarks::BookmarkNode* parent,
    int index) {
  auto cloned_node =
      std::make_unique<bookmarks::BookmarkNode>(element.id(), element.url);
  if (!element.is_url) {
    cloned_node->set_type(bookmarks::BookmarkNode::FOLDER);
    for (int i = 0; i < static_cast<int>(element.children.size()); ++i)
      CloneBookmarkNodeForDeleteImpl(element.children[i], cloned_node.get(), i);
  }
  cloned_node->SetTitle(element.title);

  // clear sync timestsamp so this sends in unsynced records
  bookmarks::BookmarkNode::MetaInfoMap meta_info_map = element.meta_info_map;
  meta_info_map.erase("sync_timestamp");
  cloned_node->SetMetaInfoMap(meta_info_map);

  auto* cloned_node_ptr = cloned_node.get();
  parent->Add(std::move(cloned_node), index);
  // we call `Changed` here because we don't want to update the order
  BookmarkNodeChanged(bookmark_model_, cloned_node_ptr);
}

void BookmarkChangeProcessor::CloneBookmarkNodeForDelete(
    const std::vector<bookmarks::BookmarkNodeData::Element>& elements,
    bookmarks::BookmarkNode* parent,
    int index) {
  for (size_t i = 0; i < elements.size(); ++i) {
    CloneBookmarkNodeForDeleteImpl(
        elements[i], parent, index + static_cast<int>(i));
  }
}

void BookmarkChangeProcessor::BookmarkNodeRemoved(
    BookmarkModel* model,
    const BookmarkNode* parent,
    int old_index,
    const BookmarkNode* node,
    const std::set<GURL>& no_longer_bookmarked) {
  // TODO(bridiver) - should this be in OnWillRemoveBookmarks?
  // copy into the deleted node tree without firing any events
  auto* deleted_node = GetDeletedNodeRoot();
  CHECK(deleted_node);
  bookmarks::BookmarkNodeData data(node);
  CloneBookmarkNodeForDelete(
      data.elements, deleted_node, deleted_node->child_count());
}

void BookmarkChangeProcessor::BookmarkAllUserNodesRemoved(
    BookmarkModel* model,
    const std::set<GURL>& removed_urls) {
  // this only happens on profile deletion and we don't want
  // to wipe out the remote store when that happens
}

void BookmarkChangeProcessor::BookmarkNodeChanged(BookmarkModel* model,
                                                  const BookmarkNode* node) {
  ScopedPauseObserver pause(this);
  // clearing the sync_timestamp will put the record back in the `Unsynced` list
  model->DeleteNodeMetaInfo(node, "sync_timestamp");
  // also clear the last send time because this is a new change
  model->DeleteNodeMetaInfo(node, "last_send_time");

  model->SetNodeMetaInfo(node,
      "last_updated_time",
      std::to_string(base::Time::Now().ToJsTime()));
}

void BookmarkChangeProcessor::BookmarkMetaInfoChanged(
    BookmarkModel* model, const BookmarkNode* node) {
  BookmarkNodeChanged(model, node);
}

void BookmarkChangeProcessor::BookmarkNodeMoved(BookmarkModel* model,
      const BookmarkNode* old_parent, int old_index,
      const BookmarkNode* new_parent, int new_index) {
  auto* node = new_parent->GetChild(new_index);

  std::string prev_node_order;
  std::string next_node_order;
  std::string parent_node_order;
  GetOrder(new_parent, new_index,
           &prev_node_order, &next_node_order, &parent_node_order);

  PushRRContext(
      prev_node_order, next_node_order, parent_node_order,
      node->id(), jslib_const::kActionUpdate);
  sync_client_->SendGetBookmarkOrder(
      prev_node_order, next_node_order, parent_node_order);
  // responds in OnGetBookmarkOrder
}

void BookmarkChangeProcessor::BookmarkNodeFaviconChanged(
    BookmarkModel* model,
    const BookmarkNode* node) {
  BookmarkNodeChanged(model, node);
}

void BookmarkChangeProcessor::BookmarkNodeChildrenReordered(
    BookmarkModel* model, const BookmarkNode* node) {
  // this should be safe to ignore as it's only called for managed bookmarks
}

void BookmarkChangeProcessor::Reset() {
  ui::TreeNodeIterator<const bookmarks::BookmarkNode>
      iterator(bookmark_model_->root_node());
  bookmark_model_->BeginExtensiveChanges();

  {
    ScopedPauseObserver pause(this);
    while (iterator.has_next()) {
      const bookmarks::BookmarkNode* node = iterator.Next();
      bookmark_model_->DeleteNodeMetaInfo(node, "object_id");
      bookmark_model_->DeleteNodeMetaInfo(node, "order");
      bookmark_model_->DeleteNodeMetaInfo(node, "sync_timestamp");
      bookmark_model_->DeleteNodeMetaInfo(node, "last_send_time");
      bookmark_model_->DeleteNodeMetaInfo(node, "last_updated_time");
    }
  }

  auto* deleted_node = GetDeletedNodeRoot();
  CHECK(deleted_node);
  deleted_node->DeleteAll();
  bookmark_model_->EndExtensiveChanges();
}

void BookmarkChangeProcessor::ApplyChangesFromSyncModel(
    const RecordsList &records) {
  ScopedPauseObserver pause(this);
  bookmark_model_->BeginExtensiveChanges();
  for (const auto& sync_record : records) {
    DCHECK(sync_record->has_bookmark());
    DCHECK(!sync_record->objectId.empty());

    auto* node = FindByObjectId(bookmark_model_, sync_record->objectId);
    auto bookmark_record = sync_record->GetBookmark();

    if (node && sync_record->action == jslib::SyncRecord::Action::UPDATE) {
      UpdateNode(bookmark_model_, node, sync_record.get());

      int64_t old_parent_local_id = node->parent()->id();
      const bookmarks::BookmarkNode* old_parent_node =
          bookmarks::GetBookmarkNodeByID(bookmark_model_, old_parent_local_id);

      std::string old_parent_object_id;
      if (old_parent_node) {
        old_parent_node->GetMetaInfo("object_id", &old_parent_object_id);
      }

      const bookmarks::BookmarkNode* new_parent_node = nullptr;
      if (bookmark_record.parentFolderObjectId != old_parent_object_id) {
        new_parent_node = FindParent(bookmark_model_, bookmark_record);
      }

      if (new_parent_node) {
        DCHECK(!bookmark_record.order.empty());
        int64_t index = GetIndex(new_parent_node, bookmark_record);
        bookmark_model_->Move(node, new_parent_node, index);
      }
    } else if (node &&
               sync_record->action == jslib::SyncRecord::Action::DELETE) {
      if (node->parent() == GetDeletedNodeRoot()) {
        // this is a deleted node so remove without firing events
        int index = GetDeletedNodeRoot()->GetIndexOf(node);
        GetDeletedNodeRoot()->Remove(index);
      } else {
        // normal remove
        bookmark_model_->Remove(node);
      }
    } else if (!node) {
      // TODO(bridiver) (make sure there isn't an existing record for objectId)
      const bookmarks::BookmarkNode* parent_node =
          FindParent(bookmark_model_, bookmark_record);

      if (bookmark_record.isFolder) {
        node = bookmark_model_->AddFolder(
                        parent_node,
                        GetIndex(parent_node, bookmark_record),
                        base::UTF8ToUTF16(bookmark_record.site.title));
      } else {
        node = bookmark_model_->AddURL(parent_node,
                              GetIndex(parent_node, bookmark_record),
                              base::UTF8ToUTF16(bookmark_record.site.title),
                              GURL(bookmark_record.site.location));
      }
      UpdateNode(bookmark_model_, node, sync_record.get());
    }
  }
  bookmark_model_->EndExtensiveChanges();
}

void BookmarkChangeProcessor::OnGetBookmarkOrder(
    const std::string& order,
    const std::string& prev_order,
    const std::string& next_order,
    const std::string& parent_order) {
  DCHECK(!order.empty());

  int64_t between_order_rr_context_node_id = -1;
  int action = -1;

  PopRRContext(prev_order, next_order, parent_order,
      between_order_rr_context_node_id, &action);

  DCHECK(between_order_rr_context_node_id != -1);
  DCHECK(action != -1);

  auto* bookmark_node = bookmarks::GetBookmarkNodeByID(
      bookmark_model_, between_order_rr_context_node_id);

  if (bookmark_node) {
    bookmark_model_->SetNodeMetaInfo(bookmark_node, "order", order);
  }
}

void BookmarkChangeProcessor::PushRRContext(const std::string& prev_order,
                                         const std::string& next_order,
                                         const std::string& parent_order,
                                         const int64_t& node_id,
                                         const int action) {
  std::string key(prev_order + "-" + next_order + "-" + parent_order);
  DCHECK(rr_map_.find(key) == rr_map_.end());
  rr_map_[key] = std::make_tuple(node_id, action);
}

void BookmarkChangeProcessor::PopRRContext(const std::string& prev_order,
                                        const std::string& next_order,
                                        const std::string& parent_order,
                                        int64_t& node_id,
                                        int* action) {
  std::string key(prev_order + "-" + next_order + "-" + parent_order);
  auto it = rr_map_.find(key);
  DCHECK(it != rr_map_.end());
  node_id = std::get<0>(it->second);
  *action = std::get<1>(it->second);
  rr_map_.erase(it);
}

std::unique_ptr<jslib::SyncRecord>
BookmarkChangeProcessor::BookmarkNodeToSyncBookmark(
    const bookmarks::BookmarkNode* node) {
  if (node->is_permanent_node() || !node->parent())
    return std::unique_ptr<jslib::SyncRecord>();

  auto record = std::make_unique<jslib::SyncRecord>();
  record->deviceId = sync_prefs_->GetThisDeviceId();
  record->objectData = jslib_const::SyncObjectData_BOOKMARK;

  auto bookmark = std::make_unique<jslib::Bookmark>();
  bookmark->site.location = node->url().spec();
  bookmark->site.title = base::UTF16ToUTF8(node->GetTitledUrlNodeTitle());
  bookmark->site.customTitle = base::UTF16ToUTF8(node->GetTitle());
  //bookmark->site.lastAccessedTime - ignored
  bookmark->site.creationTime = node->date_added();
  bookmark->site.favicon = node->icon_url() ? node->icon_url()->spec() : "";
  bookmark->isFolder = node->is_folder();
  bookmark->hideInToolbar =
      !node->HasAncestor(bookmark_model_->bookmark_bar_node());

  // these will be empty for unsynced nodes
  std::string sync_timestamp;
  node->GetMetaInfo("sync_timestamp", &sync_timestamp);
  if (!sync_timestamp.empty())
    record->syncTimestamp = base::Time::FromJsTime(std::stod(sync_timestamp));

  std::string object_id;
  node->GetMetaInfo("object_id", &object_id);
  record->objectId = object_id;

  std::string parent_object_id;
  node->parent()->GetMetaInfo("object_id", &parent_object_id);
  bookmark->parentFolderObjectId = parent_object_id;

  std::string order;
  node->GetMetaInfo("order", &order);
  bookmark->order = order;
  DCHECK(!order.empty());

  auto* deleted_node = GetDeletedNodeRoot();
  CHECK(deleted_node);

  if (record->objectId.empty()) {
    ScopedPauseObserver pause(this);
    record->objectId = tools::GenerateObjectId();
    record->action = jslib::SyncRecord::Action::CREATE;
    bookmark_model_->SetNodeMetaInfo(node, "object_id", record->objectId);
  } else if (node->HasAncestor(deleted_node)) {
   record->action = jslib::SyncRecord::Action::DELETE;
  } else {
    record->action = jslib::SyncRecord::Action::UPDATE;
    DCHECK(!bookmark->order.empty());
    DCHECK(!record->objectId.empty());
  }

  record->SetBookmark(std::move(bookmark));

  return record;
}

bool IsUnsynced(const bookmarks::BookmarkNode* node) {
  std::string sync_timestamp;
  node->GetMetaInfo("sync_timestamp", &sync_timestamp);

  if (sync_timestamp.empty())
    return true;

  std::string last_updated_time;
  node->GetMetaInfo("last_updated_time", &last_updated_time);

  return !last_updated_time.empty() &&
      base::Time::FromJsTime(std::stod(last_updated_time)) >
      base::Time::FromJsTime(std::stod(sync_timestamp));
}

void BookmarkChangeProcessor::GetAllSyncData(
    const std::vector<std::unique_ptr<jslib::SyncRecord>>& records,
    SyncRecordAndExistingList* records_and_existing_objects) {
  for (const auto& record : records) {
    auto resolved_record = std::make_unique<SyncRecordAndExisting>();
    resolved_record->first = jslib::SyncRecord::Clone(*record);
    auto* node = FindByObjectId(bookmark_model_, record->objectId);
    if (node) {
      std::string sync_timestamp;
      node->GetMetaInfo("sync_timestamp", &sync_timestamp);

      // only match unsynced nodes so we don't accidentally overwrite
      // changes from another client with our local changes
      if (IsUnsynced(node)) {
        resolved_record->second = BookmarkNodeToSyncBookmark(node);
      }
    }

    records_and_existing_objects->push_back(std::move(resolved_record));
  }
}

bookmarks::BookmarkNode* BookmarkChangeProcessor::GetDeletedNodeRoot() {
  if (!deleted_node_root)
    deleted_node_root = const_cast<bookmarks::BookmarkNode*>(
        bookmarks::GetBookmarkNodeByID(bookmark_model_, deleted_node_id));

  return deleted_node_root;
}

void BookmarkChangeProcessor::SendUnsynced(
    base::TimeDelta unsynced_send_interval) {
  std::vector<std::unique_ptr<jslib::SyncRecord>> records;

  auto* deleted_node = GetDeletedNodeRoot();
  CHECK(deleted_node);
  std::vector<const bookmarks::BookmarkNode*> root_nodes = {
    bookmark_model_->other_node(),
    bookmark_model_->bookmark_bar_node(),
    deleted_node
  };

  for (const auto* root_node : root_nodes) {
    ui::TreeNodeIterator<const bookmarks::BookmarkNode>
        iterator(root_node);
    while (iterator.has_next()) {
      const bookmarks::BookmarkNode* node = iterator.Next();

      // only send unsynced records
      if (!IsUnsynced(node))
        continue;

      std::string last_send_time;
      node->GetMetaInfo("last_send_time", &last_send_time);
      if (!last_send_time.empty() &&
          // don't send more often than unsynced_send_interval_
          (base::Time::Now() -
              base::Time::FromJsTime(std::stod(last_send_time))) <
          unsynced_send_interval)
        continue;

      {
        ScopedPauseObserver pause(this);
        bookmark_model_->SetNodeMetaInfo(node,
            "last_send_time", std::to_string(base::Time::Now().ToJsTime()));
      }
      auto record = BookmarkNodeToSyncBookmark(node);
      if (record)
        records.push_back(std::move(record));

      if (records.size() == 1000) {
        sync_client_->SendSyncRecords(
            jslib_const::SyncRecordType_BOOKMARKS, records);
        records.clear();
      }
    }
  }
  if (!records.empty()) {
    sync_client_->SendSyncRecords(
      jslib_const::SyncRecordType_BOOKMARKS, records);
    records.clear();
  }
}

uint64_t BookmarkChangeProcessor::InitialSync() {
    auto* deleted_node = GetDeletedNodeRoot();
    CHECK(deleted_node);
    std::vector<const bookmarks::BookmarkNode*> root_nodes = {
      bookmark_model_->other_node(),
      bookmark_model_->bookmark_bar_node(),
      deleted_node
    };

    std::vector<const bookmarks::BookmarkNode*> sync_nodes;
    for (const auto* root_node : root_nodes) {
      ui::TreeNodeIterator<const bookmarks::BookmarkNode>
          iterator(root_node);
      while (iterator.has_next())
        sync_nodes.push_back(iterator.Next());
    }

    uint64_t initial_sync_record_count = sync_nodes.size();

    for (auto* node : sync_nodes)
      BookmarkNodeAdded(bookmark_model_,
                        node->parent(),
                        node->parent()->GetIndexOf(node));

    return initial_sync_record_count;
}

}  // namespace brave_sync
