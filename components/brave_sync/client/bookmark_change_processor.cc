/* Copyright 2016 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_sync/client/bookmark_change_processor.h"

#include <string>
#include <tuple>
#include <memory>
#include <utility>
#include <vector>

#include "base/strings/utf_string_conversions.h"
#include "brave/components/brave_sync/bookmark_order_util.h"
#include "brave/components/brave_sync/client/bookmark_node.h"
#include "brave/components/brave_sync/jslib_const.h"
#include "brave/components/brave_sync/jslib_messages.h"
#include "brave/components/brave_sync/tools.h"
#include "chrome/browser/bookmarks/bookmark_model_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/bookmarks/browser/bookmark_storage.h"
#include "components/bookmarks/browser/bookmark_utils.h"
#include "components/bookmarks/common/bookmark_pref_names.h"
#include "components/prefs/pref_service.h"
#include "ui/base/models/tree_node_iterator.h"

using bookmarks::BookmarkNode;
using bookmarks::BookmarkModel;

namespace {

class ScopedPauseObserver {
 public:
  explicit ScopedPauseObserver(brave_sync::BookmarkChangeProcessor* processor) :
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

const char kDeletedBookmarksTitle[] = "Deleted Bookmarks";
const char kPendingBookmarksTitle[] = "Pending Bookmarks";

std::unique_ptr<brave_sync::BraveBookmarkPermanentNode>
    MakePermanentNode(const std::string& title, int64_t* next_node_id) {
  using brave_sync::BraveBookmarkPermanentNode;
  auto node = std::make_unique<BraveBookmarkPermanentNode>(*next_node_id);
  (*next_node_id)++;
  node->set_type(bookmarks::BookmarkNode::FOLDER);
  node->set_visible(false);
  node->SetTitle(base::UTF8ToUTF16(title));

  return node;
}

}  // namespace

namespace brave_sync {

bool IsSyncManagedNodeDeleted(const bookmarks::BookmarkPermanentNode* node) {
  return node->GetTitledUrlNodeTitle() ==
      base::UTF8ToUTF16(kDeletedBookmarksTitle);
}

bool IsSyncManagedNodePending(const bookmarks::BookmarkPermanentNode* node) {
  return node->GetTitledUrlNodeTitle() ==
      base::UTF8ToUTF16(kPendingBookmarksTitle);
}

bool IsSyncManagedNode(const bookmarks::BookmarkPermanentNode* node) {
  return IsSyncManagedNodeDeleted(node) || IsSyncManagedNodePending(node);
}

bookmarks::BookmarkPermanentNodeList
LoadExtraNodes(bookmarks::LoadExtraCallback callback,
               int64_t* next_node_id) {
  // TODO(bridiver) - deleted node should not be visible
  bookmarks::BookmarkPermanentNodeList extra_nodes;
  if (callback)
    extra_nodes = std::move(callback).Run(next_node_id);

  auto deleted_node = MakePermanentNode(kDeletedBookmarksTitle, next_node_id);
  extra_nodes.push_back(std::move(deleted_node));

  auto pending_node = MakePermanentNode(kPendingBookmarksTitle, next_node_id);
  extra_nodes.push_back(std::move(pending_node));

  return extra_nodes;
}

namespace {

void GetOrder(const bookmarks::BookmarkNode* parent,
              int index,
              std::string* prev_order,
              std::string* next_order,
              std::string* parent_order) {
  DCHECK_GE(index, 0);
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
  DCHECK_GE(index, 0);
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

uint64_t GetIndexByOrder(const bookmarks::BookmarkNode* root_node,
                  const std::string& record_order) {
  int index = 0;
  while (index < root_node->child_count()) {
    const bookmarks::BookmarkNode* node = root_node->GetChild(index);
    std::string node_order;
    node->GetMetaInfo("order", &node_order);

    if (!node_order.empty() &&
        brave_sync::CompareOrder(record_order, node_order))
      return index;

    ++index;
  }
  return index;
}

uint64_t GetIndex(const bookmarks::BookmarkNode* root_node,
                  const jslib::Bookmark& record) {
  return GetIndexByOrder(root_node, record.order);
}

// this should only be called for resolved records we get from the server
void UpdateNode(bookmarks::BookmarkModel* model,
                const bookmarks::BookmarkNode* node,
                const jslib::SyncRecord* record,
                const bookmarks::BookmarkNode* pending_node_root = nullptr) {
  auto bookmark = record->GetBookmark();
  if (bookmark.isFolder) {
    // SetDateFolderModified
  } else {
    model->SetURL(node, GURL(bookmark.site.location));
    // TODO(alexeyb): apply these:
    // sync_bookmark.site.customTitle
    // sync_bookmark.site.lastAccessedTime
    // sync_bookmark.site.favicon
  }

  const auto& title = !bookmark.site.title.empty() ?
      bookmark.site.title : bookmark.site.customTitle;
  model->SetTitle(node,
      base::UTF8ToUTF16(title));
  model->SetDateAdded(node, bookmark.site.creationTime);
  model->SetNodeMetaInfo(node, "object_id", record->objectId);
  model->SetNodeMetaInfo(node, "order", bookmark.order);

  // updating the sync_timestamp marks this record as synced
  model->SetNodeMetaInfo(node,
      "sync_timestamp",
      std::to_string(record->syncTimestamp.ToJsTime()));

  if (pending_node_root && node->parent() == pending_node_root) {
    model->SetNodeMetaInfo(node, "parent_object_id",
        bookmark.parentFolderObjectId);
  }
}

const bookmarks::BookmarkNode* FindParent(bookmarks::BookmarkModel* model,
                                          const jslib::Bookmark& bookmark,
                                          bookmarks::BookmarkNode*
                                                            pending_node_root) {
  auto* parent_node = FindByObjectId(model, bookmark.parentFolderObjectId);

  if (!parent_node) {
    if (!bookmark.parentFolderObjectId.empty()) {
      return pending_node_root;
    }
    if (
        // this flag is a bit odd, but if the node doesn't have a parent and
        // hideInToolbar is false, then this bookmark should go in the
        // toolbar root. We don't care about this flag for records with
        // a parent id because they will be inserted into the correct
        // parent folder
        !bookmark.hideInToolbar ||
        // mobile generated bookmarks go also in bookmark bar
        (!bookmark.order.empty() && bookmark.order.at(0) == '2')) {
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
      profile_(profile),
      bookmark_model_(BookmarkModelFactory::GetForBrowserContext(
          Profile::FromBrowserContext(profile))),
      deleted_node_root_(nullptr),
      pending_node_root_(nullptr) {
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
  // This may be invoked after bookmarks import
  VLOG(1) << __func__;
}

void BookmarkChangeProcessor::BookmarkModelBeingDeleted(
    bookmarks::BookmarkModel* model) {
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

  // The node which has not yet been sent, should not be cloned into removed.
  std::string node_object_id;
  node->GetMetaInfo("object_id", &node_object_id);
  if (node_object_id.empty()) {
    return;
  }

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
  // Ignore metadata changes.
  // These are:
  // Brave managed: "object_id", "order", "sync_timestamp",
  //      "last_send_time", "last_updated_time"
  // Chromium managed: kBookmarkLastVisitDateOnMobileKey,
  //      kBookmarkLastVisitDateOnDesktopKey, kBookmarkDismissedFromNTP,
  //      submitted by private JS API
  // Not interested in any of these.
}

void BookmarkChangeProcessor::BookmarkNodeMoved(BookmarkModel* model,
      const BookmarkNode* old_parent, int old_index,
      const BookmarkNode* new_parent, int new_index) {
  auto* node = new_parent->GetChild(new_index);
  model->DeleteNodeMetaInfo(node, "order");
  BookmarkNodeChanged(model, node);
  // TODO(darkdh): handle old_parent == new_parent to avoid duplicate order
  // clearing. Also https://github.com/brave/sync/issues/231 blocks update to
  // another devices
  for (int i = old_index; i < old_parent->child_count(); ++i) {
    auto* shifted_node = old_parent->GetChild(i);
    model->DeleteNodeMetaInfo(shifted_node, "order");
    BookmarkNodeChanged(model, shifted_node);
  }
  for (int i = new_index; i < new_parent->child_count(); ++i) {
    auto* shifted_node = new_parent->GetChild(i);
    model->DeleteNodeMetaInfo(shifted_node, "order");
    BookmarkNodeChanged(model, shifted_node);
  }
}

void BookmarkChangeProcessor::BookmarkNodeFaviconChanged(
    BookmarkModel* model,
    const BookmarkNode* node) {
  // TODO(darkdh): This will be triggered right after apply sync CREATE records
  // So the node applied from sync record will be put into unsync list
  // BookmarkNodeChanged(model, node);
}

void BookmarkChangeProcessor::BookmarkNodeChildrenReordered(
    BookmarkModel* model, const BookmarkNode* node) {
  // this should be safe to ignore as it's only called for managed bookmarks
}

void BookmarkChangeProcessor::Reset(bool clear_meta_info) {
  ui::TreeNodeIterator<const bookmarks::BookmarkNode>
      iterator(bookmark_model_->root_node());
  bookmark_model_->BeginExtensiveChanges();

  if (clear_meta_info) {
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
  auto* pending_node = GetPendingNodeRoot();
  CHECK(pending_node);
  pending_node->DeleteAll();
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

void ValidateFolderOrders(const bookmarks::BookmarkNode* folder_node) {
  DCHECK(folder_node);

  // Validate direct children order
  std::string left_order;
  std::string right_order;
  for (auto i = 0; i < folder_node->child_count(); ++i) {
    const auto* node = folder_node->GetChild(i);
    std::string order;
    node->GetMetaInfo("order", &order);
    if (order.empty()) {
      continue;
    }

    if (left_order.empty()) {
      left_order = order;
      continue;
    }

    if (right_order.empty()) {
      right_order = order;
    } else {
      left_order = right_order;
      right_order = order;
    }

    DCHECK(!left_order.empty());
    DCHECK(!right_order.empty());

    bool compare_result = CompareOrder(left_order, right_order);
    if (!compare_result) {
      DLOG(ERROR) << "ValidateFolderOrders failed";
      DLOG(ERROR) << "folder_node=" << folder_node->GetTitle();
      DLOG(ERROR) << "folder_node->child_count()=" <<
                                                  folder_node->child_count();
      DLOG(ERROR) << "i=" << i;
      DLOG(ERROR) << "left_order=" << left_order;
      DLOG(ERROR) << "right_order=" << right_order;
      DLOG(ERROR) << "Unexpected situation of invalid order";
      return;
    }
  }
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

    if (node && sync_record->action == jslib::SyncRecord::Action::A_UPDATE) {
      int64_t old_parent_local_id = node->parent()->id();
      const bookmarks::BookmarkNode* old_parent_node =
          bookmarks::GetBookmarkNodeByID(bookmark_model_, old_parent_local_id);

      std::string old_parent_object_id;
      if (old_parent_node) {
        old_parent_node->GetMetaInfo("object_id", &old_parent_object_id);
      }

      const bookmarks::BookmarkNode* new_parent_node = nullptr;
      if (bookmark_record.parentFolderObjectId != old_parent_object_id) {
        new_parent_node = FindParent(bookmark_model_, bookmark_record,
            GetPendingNodeRoot());
      }

      if (new_parent_node) {
        DCHECK(!bookmark_record.order.empty());
        int64_t index = GetIndex(new_parent_node, bookmark_record);
        bookmark_model_->Move(node, new_parent_node, index);
      } else if (!bookmark_record.order.empty()) {
        std::string order;
        node->GetMetaInfo("order", &order);
        DCHECK(!order.empty());
        if (bookmark_record.order != order) {
          int64_t index = GetIndex(node->parent(), bookmark_record);
          bookmark_model_->Move(node, node->parent(), index);
        }
      }
      UpdateNode(bookmark_model_, node, sync_record.get());
    } else if (node &&
               sync_record->action == jslib::SyncRecord::Action::A_DELETE) {
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
    } else if (sync_record->action == jslib::SyncRecord::Action::A_CREATE) {
      bool folder_was_created = false;
      const bookmarks::BookmarkNode* parent_node = nullptr;
      if (!node) {
        // TODO(bridiver) make sure there isn't an existing record for objectId
        parent_node =
            FindParent(bookmark_model_, bookmark_record, GetPendingNodeRoot());

        const BookmarkNode* bookmark_bar = bookmark_model_->bookmark_bar_node();
        bool bookmark_bar_was_empty = bookmark_bar->empty();

        if (bookmark_record.isFolder) {
          node = bookmark_model_->AddFolder(
                          parent_node,
                          GetIndex(parent_node, bookmark_record),
                          base::UTF8ToUTF16(bookmark_record.site.title));
          folder_was_created = true;
        } else {
          node = bookmark_model_->AddURL(parent_node,
                          GetIndex(parent_node, bookmark_record),
                          base::UTF8ToUTF16(bookmark_record.site.title),
                          GURL(bookmark_record.site.location));
        }
        if (bookmark_bar_was_empty)
          profile_->GetPrefs()->SetBoolean(bookmarks::prefs::kShowBookmarkBar,
                                          true);
      }
      UpdateNode(bookmark_model_, node, sync_record.get(),
          GetPendingNodeRoot());

#ifndef NDEBUG
      if (parent_node) {
        ValidateFolderOrders(parent_node);
      }
#endif

      if (folder_was_created) {
        CompletePendingNodesMove(node, sync_record->objectId);
      }
    }
  }
  bookmark_model_->EndExtensiveChanges();
}

void BookmarkChangeProcessor::CompletePendingNodesMove(
    const bookmarks::BookmarkNode* created_folder_node,
    const std::string& created_folder_object_id) {
  DCHECK(GetPendingNodeRoot());

  // node, node_order
  using MoveInfo = std::tuple<bookmarks::BookmarkNode*, const std::string>;
  std::vector<MoveInfo> move_infos;

  bookmarks::BookmarkNode* pending_node_root = GetPendingNodeRoot();
  for (int i = 0; i < pending_node_root->child_count(); ++i) {
    bookmarks::BookmarkNode* node = pending_node_root->GetChild(i);

    std::string parent_object_id;
    node->GetMetaInfo("parent_object_id", &parent_object_id);
    if (parent_object_id.empty()) {
      // The node has been attached to folder which is still in Pending nodes
      continue;
    }

    if (created_folder_object_id != parent_object_id) {
      // Node is still pending, because waits for another parent
      continue;
    }

    std::string order;
    node->GetMetaInfo("order", &order);

    DCHECK(!order.empty());
    move_infos.push_back(std::make_tuple(node, order));
  }

  for (auto& move_info : move_infos) {
    auto* node = std::get<0>(move_info);
    const auto& order = std::get<1>(move_info);
    int64_t index = GetIndexByOrder(created_folder_node, order);

    bookmark_model_->Move(node, created_folder_node, index);
    // Now we dont need "parent_object_id" metainfo on node, because node
    // is attached to proper parent. Note that parent can still be a child
    // of "Pending Bookmarks" note.
    node->DeleteMetaInfo("parent_object_id");
#ifndef NDEBUG
    ValidateFolderOrders(created_folder_node);
#endif
  }
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
  // bookmark->site.lastAccessedTime - ignored
  bookmark->site.creationTime = node->date_added();
  bookmark->site.favicon = node->icon_url() ? node->icon_url()->spec() : "";
  // Url may have type OTHER_NODE if it is in Deleted Bookmarks
  bookmark->isFolder = (node->type() != bookmarks::BookmarkNode::URL &&
                           node->type() != bookmarks::BookmarkNode::OTHER_NODE);
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
  std::string sync_timestamp;
  node->GetMetaInfo("sync_timestamp", &sync_timestamp);

  if (!sync_timestamp.empty()) {
    record->syncTimestamp = base::Time::FromJsTime(std::stod(sync_timestamp));
  } else {
    record->syncTimestamp = base::Time::Now();
  }

  // Situation below means the node was created and then deleted before send
  // Should be ignored
  if (record->objectId.empty() && node->HasAncestor(deleted_node)) {
    return nullptr;
  }

  if (record->objectId.empty()) {
    record->objectId = tools::GenerateObjectId();
    record->action = jslib::SyncRecord::Action::A_CREATE;
    bookmark_model_->SetNodeMetaInfo(node, "object_id", record->objectId);
  } else if (node->HasAncestor(deleted_node)) {
    record->action = jslib::SyncRecord::Action::A_DELETE;
  } else {
    record->action = jslib::SyncRecord::Action::A_UPDATE;
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
          IsSyncManagedNodeDeleted(
              static_cast<const bookmarks::BookmarkPermanentNode*>(node))) {
        deleted_node_root_ = const_cast<bookmarks::BookmarkNode*>(node);
        return deleted_node_root_;
      }
    }
  }
  DCHECK(deleted_node_root_);
  return deleted_node_root_;
}

bookmarks::BookmarkNode* BookmarkChangeProcessor::GetPendingNodeRoot() {
  if (!pending_node_root_) {
    ui::TreeNodeIterator<const bookmarks::BookmarkNode>
        iterator(bookmark_model_->root_node());
    while (iterator.has_next()) {
      const bookmarks::BookmarkNode* node = iterator.Next();
      if (node->is_permanent_node() &&
          IsSyncManagedNodePending(
              static_cast<const bookmarks::BookmarkPermanentNode*>(node))) {
        pending_node_root_ = const_cast<bookmarks::BookmarkNode*>(node);
        return pending_node_root_;
      }
    }
  }
  DCHECK(pending_node_root_);
  return pending_node_root_;
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

      bookmark_model_->SetNodeMetaInfo(node,
          "last_send_time", std::to_string(base::Time::Now().ToJsTime()));

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
  sync_client_->ClearOrderMap();
}

void BookmarkChangeProcessor::InitialSync() {}

}  // namespace brave_sync
