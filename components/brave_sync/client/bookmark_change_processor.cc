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

bool IsSyncManagedNode(const bookmarks::BookmarkPermanentNode* node) {
  return node->GetTitledUrlNodeTitle() ==
      base::UTF8ToUTF16("Deleted Bookmarks");
}

bookmarks::BookmarkPermanentNodeList
LoadExtraNodes(bookmarks::LoadExtraCallback callback,
               int64_t* next_node_id) {
  // TODO(bridiver) - deleted node should not be visible
  bookmarks::BookmarkPermanentNodeList extra_nodes;
  if (callback)
    extra_nodes = std::move(callback).Run(next_node_id);

  auto node = std::make_unique<bookmarks::BookmarkPermanentNode>(*next_node_id);
  (*next_node_id)++;
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
  DCHECK(index >= 0);
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

void GetPrevObjectId(const bookmarks::BookmarkNode* parent,
                     int index,
                     std::string* prev_object_id) {
  DCHECK(index >= 0);
  auto* prev_node = index == 0 ?
    nullptr :
    parent->GetChild(index - 1);

  if (prev_node)
    prev_node->GetMetaInfo("object_id", prev_object_id);
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
  model->SetNodeMetaInfo(node,
      "sync_timestamp",
      std::to_string(record->syncTimestamp.ToJsTime()));
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
          Profile::FromBrowserContext(profile))),
      deleted_node_root_(nullptr) {
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
  } else {
    // default type is URL and we will hit
    // [url_index.cc(122)] "Check failed: i != nodes_ordered_by_url_set_.end()."
    // However, clone nodes should be dummy nodes which only need
    // object_id meta info.
    cloned_node->set_type(bookmarks::BookmarkNode::OTHER_NODE);
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
  model->DeleteNodeMetaInfo(node, "order");
  // TODO(darkdh): handle old_parent == new_parent to avoid duplicate order
  // clearing. Also https://github.com/brave/sync/issues/231 blocks update to
  // another devices
  for (int i = old_index; i < old_parent->child_count(); ++i) {
    auto* shifted_node = old_parent->GetChild(i);
    model->DeleteNodeMetaInfo(shifted_node, "order");
  }
  for (int i = new_index; i < new_parent->child_count(); ++i) {
    auto* shifted_node = new_parent->GetChild(i);
    model->DeleteNodeMetaInfo(shifted_node, "order");
  }
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

void BookmarkChangeProcessor::DeleteSelfAndChildren(
    const bookmarks::BookmarkNode* node) {
  DCHECK(node->is_folder());
  for (int i = 0; i < node->child_count(); ++i) {
    if (node->GetChild(i)->is_folder()) {
      DeleteSelfAndChildren(node->GetChild(i));
    } else {
      bookmark_model_->Remove(node->GetChild(i));
    }
  }
  bookmark_model_->Remove(node);
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
      UpdateNode(bookmark_model_, node, sync_record.get());
    } else if (node &&
               sync_record->action == jslib::SyncRecord::Action::DELETE) {
      if (node->parent() == GetDeletedNodeRoot()) {
        // this is a deleted node so remove without firing events
        int index = GetDeletedNodeRoot()->GetIndexOf(node);
        GetDeletedNodeRoot()->Remove(index);
      } else {
        // normal remove
        if (node->is_folder()) {
          DeleteSelfAndChildren(node);
        } else {
          bookmark_model_->Remove(node);
        }
      }
    } else if (sync_record->action == jslib::SyncRecord::Action::CREATE) {
      if (!node) {
        // TODO(bridiver) make sure there isn't an existing record for objectId
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
      }
      UpdateNode(bookmark_model_, node, sync_record.get());
    }
  }
  bookmark_model_->EndExtensiveChanges();
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

  std::string object_id;
  node->GetMetaInfo("object_id", &object_id);
  record->objectId = object_id;

  std::string parent_object_id;
  node->parent()->GetMetaInfo("object_id", &parent_object_id);
  bookmark->parentFolderObjectId = parent_object_id;

  std::string order;
  node->GetMetaInfo("order", &order);
  bookmark->order = order;

  int index = node->parent()->GetIndexOf(node);
  std::string prev_object_id;
  GetPrevObjectId(node->parent(), index, &prev_object_id);
  bookmark->prevObjectId = prev_object_id;

  std::string prev_order, next_order, parent_order;
  GetOrder(node->parent(), index, &prev_order, &next_order, &parent_order);
  if (parent_order.empty() && node->parent()->is_permanent_node())
    parent_order =
        sync_prefs_->GetBookmarksBaseOrder() + std::to_string(index);
  bookmark->prevOrder = prev_order;
  bookmark->nextOrder = next_order;
  bookmark->parentOrder = parent_order;

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
      resolved_record->second = BookmarkNodeToSyncBookmark(node);
    }

    records_and_existing_objects->push_back(std::move(resolved_record));
  }
}

bookmarks::BookmarkNode* BookmarkChangeProcessor::GetDeletedNodeRoot() {
  if (!deleted_node_root_) {
    ui::TreeNodeIterator<const bookmarks::BookmarkNode>
        iterator(bookmark_model_->root_node());
    while (iterator.has_next()) {
      const bookmarks::BookmarkNode* node = iterator.Next();
      if (node->is_permanent_node() &&
          IsSyncManagedNode(
              static_cast<const bookmarks::BookmarkPermanentNode*>(node))) {
        deleted_node_root_ = const_cast<bookmarks::BookmarkNode*>(node);
        return deleted_node_root_;
      }
    }
  }
  DCHECK(deleted_node_root_);
  return deleted_node_root_;
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

void BookmarkChangeProcessor::InitialSync() {}

}  // namespace brave_sync
