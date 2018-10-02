/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "brave/components/brave_sync/bookmarks.h"

#include <map>
#include <memory>
#include <string>
#include <tuple>

#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/components/brave_sync/bookmark_order_util.h"
#include "brave/components/brave_sync/cansendbookmarks.h"
#include "brave/components/brave_sync/client/client.h"
#include "brave/components/brave_sync/jslib_const.h"
#include "brave/components/brave_sync/jslib_messages.h"
#include "brave/components/brave_sync/object_map.h"
#include "brave/components/brave_sync/tools.h"
#include "brave/components/brave_sync/debug.h"
#include "brave/components/brave_sync/values_conv.h"
#include "chrome/browser/bookmarks/bookmark_model_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/bookmarks/browser/bookmark_utils.h"
#include "components/bookmarks/browser/bookmark_model_observer.h"
#include "content/public/browser/browser_thread.h"
#include "ui/base/models/tree_node_iterator.h"

namespace brave_sync {

Bookmarks::Bookmarks(ControllerForBookmarksExports *controller_exports) :
  profile_(nullptr), model_(nullptr), sync_obj_map_(nullptr),
  observer_is_set_(false), controller_exports_(controller_exports) {
  LOG(ERROR) << "TAGAB brave_sync::Bookmarks::Bookmarks CTOR";
}

Bookmarks::~Bookmarks() {
  LOG(ERROR) << "TAGAB brave_sync::Bookmarks::~Bookmarks DTOR";
  if (model_ && observer_is_set_) {
    model_->RemoveObserver(this);
  }
}

void Bookmarks::SetProfile(Profile *profile) {
  LOG(ERROR) << "TAGAB brave_sync::Bookmarks::SetProfile profile="<<profile;
  DCHECK(profile != nullptr);
  if (profile_ == nullptr) {
    model_ = BookmarkModelFactory::GetForBrowserContext(profile);
    LOG(ERROR) << "TAGAB brave_sync::Bookmarks::SetProfile model_="<<model_;
    DCHECK(observer_is_set_ == false);
    model_->AddObserver(this);
    observer_is_set_ = true;
    // per profile
    profile_ = profile;
  } else {
    LOG(ERROR) << "TAGAB brave_sync::Bookmarks::SetProfile already set profile_="<<profile_;
    DCHECK(false);
  }
}

void Bookmarks::SetThisDeviceId(const std::string &device_id) {
  DCHECK(device_id_.empty());
  DCHECK(!device_id.empty());
  device_id_ = device_id;
}

void Bookmarks::SetObjectMap(storage::ObjectMap* sync_obj_map) {
  DCHECK(sync_obj_map != nullptr);
  DCHECK(sync_obj_map_ == nullptr);
  sync_obj_map_ = sync_obj_map;
}

const bookmarks::BookmarkNode* Bookmarks::GetNodeById(const int64_t &bookmark_local_id) {
  LOG(ERROR) << "TAGAB brave_sync::Bookmarks::GetNodeById";
  DCHECK(model_);
  const bookmarks::BookmarkNode* node = bookmarks::GetBookmarkNodeByID(model_, bookmark_local_id);
  LOG(ERROR) << "TAGAB bookmark_local_id=" << bookmark_local_id;
  LOG(ERROR) << "TAGAB node=" << node;
  return node;
}

std::unique_ptr<jslib::Bookmark> Bookmarks::GetFromNode(const bookmarks::BookmarkNode* node,
  const std::string &node_order,
  const std::string &parent_order) {
  auto bookmark = std::make_unique<jslib::Bookmark>();

  int64_t parent_folder_id = node->parent() ? node->parent()->id() : 0;
  std::string parent_folder_object_sync_id;
  if (parent_folder_id) {
    parent_folder_object_sync_id = GetOrCreateObjectByLocalId(parent_folder_id, parent_order/* dont need it here*/);
  }

  bookmark->site.location = node->url().spec();
  bookmark->site.title = base::UTF16ToUTF8(node->GetTitledUrlNodeTitle());
  bookmark->site.customTitle = base::UTF16ToUTF8(node->GetTitle());
  //bookmark->site.lastAccessedTime = ; ??
  bookmark->site.creationTime = node->date_added();
  bookmark->site.favicon = node->icon_url() ? node->icon_url()->spec() : "";

  bookmark->isFolder = node->is_folder();
  bookmark->parentFolderObjectId = parent_folder_object_sync_id; // bytes

  bookmark->hideInToolbar = !node->HasAncestor(model_->bookmark_bar_node());
  bookmark->order = node_order; // order should be taken from obj map

  return bookmark;
}

std::unique_ptr<jslib::SyncRecord> Bookmarks::GetResolvedBookmarkValue(
  const std::string &object_id, const jslib::SyncRecord::Action &action) {

  LOG(ERROR) << "TAGAB brave_sync::Bookmarks::GetResolvedBookmarkValue object_id=<"<<object_id<<">";

  std::string local_object_id = sync_obj_map_->GetLocalIdByObjectId(storage::ObjectMap::Type::Bookmark, object_id);
  LOG(ERROR) << "TAGAB brave_sync::Bookmarks::GetResolvedBookmarkValue local_object_id=<"<<local_object_id<<">";
  if(local_object_id.empty()) {
    return nullptr;
  }
  CHECK(model_);

  int64_t id = 0;
  bool convert_result = base::StringToInt64(local_object_id, &id);
  DCHECK(convert_result);
  if (!convert_result) {
    return nullptr;
  }

  const bookmarks::BookmarkNode* node = bookmarks::GetBookmarkNodeByID(model_, id);
  if (node == nullptr) {
    LOG(ERROR) << "TAGAB brave_sync::Bookmarks::GetResolvedBookmarkValue node not found for local_object_id=<"<<local_object_id<<">";
    // Node was removed
    // NOTREACHED() << "means we had not removed (object_id => local_id) pair from objects map";
    // Something gone wrong previously, no obvious way to fix

    return nullptr;
  }

  auto record = std::make_unique<jslib::SyncRecord>();
  record->action = action;
  record->deviceId = device_id_;
  record->objectId = object_id;
  record->objectData = "bookmark";

  std::string node_order = sync_obj_map_->GetOrderByLocalObjectId(storage::ObjectMap::Type::Bookmark, local_object_id);
  std::string parent_local_object_id;
  if (node->parent()) {
    parent_local_object_id = std::to_string(node->parent()->id());
  }
  std::string parent_order = sync_obj_map_->GetOrderByLocalObjectId(storage::ObjectMap::Type::Bookmark, parent_local_object_id);
  DCHECK(!node_order.empty());
  //DCHECK(!parent_order.empty()); Parent order can be empty when node is a top-level folder

  std::unique_ptr<jslib::Bookmark> bookmark = GetFromNode(node, node_order, parent_order);
  record->SetBookmark(std::move(bookmark));

//it appears it should not include SyncRecord, but only data, js/state/syncUtil.js

  return record;
}

std::string Bookmarks::GetOrCreateObjectByLocalId(const int64_t &local_id, const std::string &order) {
  CHECK(sync_obj_map_);
  const std::string s_local_id = base::Int64ToString(local_id);
  std::string object_id = sync_obj_map_->GetObjectIdByLocalId(storage::ObjectMap::Type::Bookmark, s_local_id);
  if (!object_id.empty()) {
    return object_id;
  }

  object_id = tools::GenerateObjectId(); // TODO, AB: pack 8 bytes from s_local_id?
  sync_obj_map_->SaveObjectIdAndOrder(
        storage::ObjectMap::Type::Bookmark,
        s_local_id,
        object_id,
        order);

  return object_id;
}

void Bookmarks::SaveIdMap(const int64_t &local_id, const std::string &order, const std::string &sync_object_id) {
  CHECK(sync_obj_map_);
  const std::string s_local_id = base::Int64ToString(local_id);
  sync_obj_map_->SaveObjectIdAndOrder(
        storage::ObjectMap::Type::Bookmark,
        s_local_id,
        sync_object_id,
        order);
  LOG(ERROR) << "TAGAB brave_sync::Bookmarks::SaveIdMap <"<<s_local_id<<"> ==> <"<<sync_object_id<<">, <"<<order<<">";
}

void Bookmarks::AddBookmark(const jslib::SyncRecord &sync_record) {
  const jslib::Bookmark &sync_bookmark = sync_record.GetBookmark();
  LOG(ERROR) << "TAGAB brave_sync::Bookmarks::AddBookmark location="<<sync_bookmark.site.location;
  LOG(ERROR) << "TAGAB brave_sync::Bookmarks::AddBookmark title="<<sync_bookmark.site.title;
  LOG(ERROR) << "TAGAB brave_sync::Bookmarks::AddBookmark order="<<sync_bookmark.order;
  LOG(ERROR) << "TAGAB brave_sync::Bookmarks::AddBookmark parentFolderObjectId="<<sync_bookmark.parentFolderObjectId;
  DCHECK(model_);
  if (model_ == nullptr) {
    return;
  }

  auto sync_record_ptr = jslib::SyncRecord::Clone(sync_record);
  std::string s_parent_local_object_id;
  if (!sync_bookmark.parentFolderObjectId.empty()) {
    s_parent_local_object_id = sync_obj_map_->GetLocalIdByObjectId(storage::ObjectMap::Type::Bookmark,
      sync_bookmark.parentFolderObjectId);
  } else {
    // We don't send parentFolderObjectId if the parent is permanent node
    // "bookmarks bar", "other" or "mobile" because it is impossible to create
    // a node direct child of the root node
  }

  content::BrowserThread::GetTaskRunnerForThread(content::BrowserThread::UI)->PostTask(
    FROM_HERE, base::Bind(&Bookmarks::AddBookmarkUiWork,
         base::Unretained(this), base::Passed(std::move(sync_record_ptr)), s_parent_local_object_id ));
}

void Bookmarks::AddBookmarkUiWork(std::unique_ptr<jslib::SyncRecord> sync_record, const std::string &s_parent_local_object_id) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DCHECK(controller_exports_);
  DCHECK(controller_exports_->GetTaskRunner());

  const jslib::Bookmark &sync_bookmark = sync_record->GetBookmark();
  LOG(ERROR) << "TAGAB brave_sync::Bookmarks::AddBookmarkUiWork objectId="<<sync_record->objectId;
  LOG(ERROR) << "TAGAB brave_sync::Bookmarks::AddBookmarkUiWork location="<<sync_bookmark.site.location;
  LOG(ERROR) << "TAGAB brave_sync::Bookmarks::AddBookmarkUiWork title="<<sync_bookmark.site.title;
  LOG(ERROR) << "TAGAB brave_sync::Bookmarks::AddBookmarkUiWork order="<<sync_bookmark.order;
  LOG(ERROR) << "TAGAB brave_sync::Bookmarks::AddBookmarkUiWork s_parent_local_object_id="<<s_parent_local_object_id;

  int64_t parent_local_object_id = -1;
  const bookmarks::BookmarkNode* parent_node = nullptr;
  if (!s_parent_local_object_id.empty() && base::StringToInt64(s_parent_local_object_id, &parent_local_object_id) && parent_local_object_id != -1) {
    LOG(ERROR) << "TAGAB brave_sync::Bookmarks::AddBookmarkUiWork parent_local_object_id=" << parent_local_object_id;
    parent_node = bookmarks::GetBookmarkNodeByID(model_, parent_local_object_id);
  } else {
    LOG(ERROR) << "TAGAB brave_sync::Bookmarks::AddBookmarkUiWork StringToInt64 failed for <s_parent_local_object_id" << s_parent_local_object_id << ">";
  }

  LOG(ERROR) << "TAGAB brave_sync::Bookmarks::AddBookmarkUiWork (GetBookmarkNodeByID) parent_node="<<parent_node;
  if (!parent_node) {
    //parent_node = bookmarks::GetParentForNewNodes(model_);
    //LOG(ERROR) << "TAGAB brave_sync::Bookmarks::AddBookmarkUiWork (GetParentForNewNodes) parent_node="<<parent_node;

    // Here are 3 options why we cannot find parent
    // 1) Parent is one of permanent nodes 'bookmark bar', 'other' or 'mobile',
    //    we do not send it on sync because we cannot create it
    // 2) Parent is 'root' node - something is wrong
    // 3) Parent is not a permanent node but was not yet synced

    if (!sync_record->GetBookmark().hideInToolbar) {
      parent_node = model_->bookmark_bar_node();
      LOG(ERROR) << "TAGAB brave_sync::Bookmarks::AddBookmarkUiWork use bookmark_bar_node";
    } else if (!sync_record->GetBookmark().order.empty() && sync_record->GetBookmark().order.at(0) == '2') {
      parent_node = model_->mobile_node();
      LOG(ERROR) << "TAGAB brave_sync::Bookmarks::AddBookmarkUiWork use mobile_node";
    } else {
      parent_node = model_->other_node();
      LOG(ERROR) << "TAGAB brave_sync::Bookmarks::AddBookmarkUiWork use other";
    }
  }

  PauseObserver();

  // std::unique_ptr<BookmarkNode> new_node =
  //     std::make_unique<BookmarkNode>(generate_next_node_id(), url);
  // new_node->SetTitle(title);
  // new_node->set_date_added(creation_time);
  // new_node->set_type(BookmarkNode::URL);
  // cannot use, ^-- are private

  base::string16 title16 = base::UTF8ToUTF16(sync_bookmark.site.title);
  const bookmarks::BookmarkNode* added_node;
  if (sync_bookmark.isFolder) {
    added_node = model_->AddFolder(
      parent_node,
      parent_node->child_count(),//int index,
      title16);
  } else {
    added_node = model_->AddURLWithCreationTimeAndMetaInfo(
        parent_node,
        parent_node->child_count(),//int index,
        title16,
        GURL(sync_bookmark.site.location),
        sync_bookmark.site.creationTime, //const base::Time& creation_time,
        nullptr//const BookmarkNode::MetaInfoMap* meta_info
      );
  }
  LOG(ERROR) << "TAGAB brave_sync::Bookmarks::AddBookmarkUiWork added_node="<<added_node;
  LOG(ERROR) << "TAGAB brave_sync::Bookmarks::AddBookmarkUiWork added_node->id()="<<added_node->id();


  // TODO, AB: apply these:
  //          sync_bookmark.site.customTitle
  //          sync_bookmark.site.lastAccessedTime
  //          sync_bookmark.site.favicon
  //          sync_bookmark.isFolder
  //          sync_bookmark.parentFolderObjectId
  //          sync_bookmark.hideInToolbar
  //          sync_bookmark.order => `int index`

  //  What is const BookmarkNode::MetaInfoMap* meta_info ?
  //      just a map string=>string to store some data
  //      examples:
  //        "last_visited_desktop" => "13177516524293958"
  //        const char kBookmarkLastVisitDateOnMobileKey[] = "last_visited";
  //        const char kBookmarkLastVisitDateOnDesktopKey[] = "last_visited_desktop";
  //        const char kBookmarkDismissedFromNTP[] = "dismissed_from_ntp";

  // There are no UpdateBookmark
  // I cannot update bookmark neither with bookmark_utils.h nor with bookmark_model

  // How to update custom title?
  // It happens in  BookmarkModel::SetTitle:
  // if (node->is_url())
  //   index_->Remove(node);
  // AsMutable(node)->SetTitle(title);
  // if (node->is_url())
  //   index_->Add(node);
  // store_->ScheduleSave();
  // all these are private
  // No matter we want ChromiumSync and Brave backend.

  ResumeObserver();

  DCHECK(!sync_bookmark.order.empty()); // What if I recieve empty order from old browser-laptop, maybe we should generate it?

  controller_exports_->GetTaskRunner()->PostTask(
    FROM_HERE,
    base::Bind(&Bookmarks::AddOrUpdateBookmarkPostUiFileWork, base::Unretained(this), parent_node->id(), added_node->id(), sync_bookmark.order, sync_record->objectId)
  );

}

void Bookmarks::AddOrUpdateBookmarkPostUiFileWork(const int64_t &folder_id, const int64_t &added_node_id, const std::string &order, const std::string &sync_record_object_id) {
  LOG(ERROR) << "TAGAB brave_sync::Bookmarks::AddOrUpdateBookmarkPostUiFileWork";
  LOG(ERROR) << "TAGAB added_node_id="<<added_node_id;
  LOG(ERROR) << "TAGAB order="<<order;
  LOG(ERROR) << "TAGAB sync_record_object_id="<<sync_record_object_id;

  SaveIdMap(added_node_id, order, sync_record_object_id);

  content::BrowserThread::GetTaskRunnerForThread(content::BrowserThread::UI)->PostTask(
    FROM_HERE, base::Bind(&Bookmarks::ReorderFolderUiWorkCollectChildren,
         base::Unretained(this), folder_id));
}

void Bookmarks::ReorderFolderUiWorkCollectChildren(const int64_t &folder_id) {
  LOG(ERROR) << "TAGAB ReorderFolderUiWorkCollectChildren";
  LOG(ERROR) << "TAGAB folder_id=" << folder_id;

  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  const bookmarks::BookmarkNode* this_parent_node = bookmarks::GetBookmarkNodeByID(model_, folder_id);
  if (!this_parent_node) {
    return;
  }

  std::vector<int64_t> children_local_ids;

  for (int i = 0; i < this_parent_node->child_count(); ++i) {
    const bookmarks::BookmarkNode* node = this_parent_node->GetChild(i);
    children_local_ids.push_back(node->id());
  }

  // Jump to file thread
  controller_exports_->GetTaskRunner()->PostTask(
    FROM_HERE,
    base::Bind(&Bookmarks::ReorderFolderFileWorkCalculateSortedIndexes,
      base::Unretained(this), folder_id, children_local_ids)
  );
}

void Bookmarks::ReorderFolderFileWorkCalculateSortedIndexes(
  const int64_t &folder_id,
  const std::vector<int64_t> &children_local_ids) {
  LOG(ERROR) << "TAGAB ReorderFolderFileWorkCalculateSortedIndexes";
  LOG(ERROR) << "TAGAB folder_id=" << folder_id;
  LOG(ERROR) << "TAGAB children_local_ids.size()=" << children_local_ids.size();

  if (children_local_ids.empty()) {
    return;
  }

  std::vector<std::tuple<int64_t, std::string>> id_and_order_list;

  for (size_t i = 0; i < children_local_ids.size(); ++i) {
    int64_t current_local_id = children_local_ids.at(i);
    std::string current_order = sync_obj_map_->GetOrderByLocalObjectId(storage::ObjectMap::Bookmark, std::to_string(current_local_id));
    id_and_order_list.push_back(std::make_tuple(current_local_id, current_order));
  }

  LOG(ERROR) << "TAGAB raw ids";
  for (size_t i = 0; i < id_and_order_list.size(); ++i) {
    LOG(ERROR) << "TAGAB id=" << std::get<0>(id_and_order_list.at(i));
    LOG(ERROR) << "TAGAB order=" << std::get<1>(id_and_order_list.at(i));
  }

  std::sort(id_and_order_list.begin(), id_and_order_list.end(),
    [](std::tuple<int64_t, std::string> &left, std::tuple<int64_t, std::string> &right) {
      return CompareOrder(std::get<1>(left), std::get<1>(right));
    }
  );

  LOG(ERROR) << "TAGAB sorted ids";
  for (size_t i = 0; i < id_and_order_list.size(); ++i) {
    LOG(ERROR) << "TAGAB id=" << std::get<0>(id_and_order_list.at(i));
    LOG(ERROR) << "TAGAB order=" << std::get<1>(id_and_order_list.at(i));
  }

  std::vector<int64_t> sorted_children;
  for (size_t i = 0; i < id_and_order_list.size(); ++i) {
    sorted_children.push_back(std::get<0>(id_and_order_list.at(i)));
  }

  content::BrowserThread::GetTaskRunnerForThread(content::BrowserThread::UI)->PostTask(
    FROM_HERE, base::Bind(&Bookmarks::ReorderFolderUiWorkApplyIndexes,
         base::Unretained(this), folder_id, base::Passed(std::move(sorted_children)) ));
}

void Bookmarks::ReorderFolderUiWorkApplyIndexes(const int64_t &folder_id, const std::vector<int64_t> &sorted_children) {
  LOG(ERROR) << "TAGAB ReorderFolderUiWorkApplyIndexes";
  LOG(ERROR) << "TAGAB folder_id=" << folder_id;
  LOG(ERROR) << "TAGAB sorted_children.size()=" << sorted_children.size();

  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  const bookmarks::BookmarkNode* parent = bookmarks::GetBookmarkNodeByID(model_, folder_id);
  if (!parent) {
    return;
  }

  std::vector<const bookmarks::BookmarkNode*> ordered_nodes;
  for (size_t i = 0; i < sorted_children.size(); ++i) {
    const bookmarks::BookmarkNode* child = bookmarks::GetBookmarkNodeByID(model_, sorted_children.at(i));
    if (child) {
      ordered_nodes.push_back(child);
    }
  }

  PauseObserver();
  model_->ReorderChildren(parent, ordered_nodes);
  ResumeObserver();
}

void Bookmarks::DeleteBookmark(const jslib::SyncRecord &sync_record) {
  const jslib::Bookmark &sync_bookmark = sync_record.GetBookmark();
  LOG(ERROR) << "TAGAB brave_sync::Bookmarks::DeleteBookmark location="<<sync_bookmark.site.location;
  LOG(ERROR) << "TAGAB brave_sync::Bookmarks::DeleteBookmark title="<<sync_bookmark.site.title;
  LOG(ERROR) << "TAGAB brave_sync::Bookmarks::DeleteBookmark order="<<sync_bookmark.order;
  LOG(ERROR) << "TAGAB brave_sync::Bookmarks::DeleteBookmark parentFolderObjectId="<<sync_record.GetBookmark().parentFolderObjectId;
  DCHECK(model_);
  if (model_ == nullptr) {
    return;
  }

  std::string s_local_object_id = sync_obj_map_->GetLocalIdByObjectId(storage::ObjectMap::Type::Bookmark,
    sync_record.objectId);
  DCHECK(!s_local_object_id.empty());
  if (s_local_object_id.empty()) {
    LOG(ERROR) << "TAGAB brave_sync::Bookmarks::DeleteBookmark: could not find local id";
    return;
  }

  int64_t local_object_id = -1;
  if (!base::StringToInt64(s_local_object_id, &local_object_id) || local_object_id == -1) {
    LOG(ERROR) << "TAGAB brave_sync::Bookmarks::DeleteBookmark: could not convert local id";
    return;
  }

  // Jump to UI thread and then back to file thread
  content::BrowserThread::GetTaskRunnerForThread(content::BrowserThread::UI)->PostTaskAndReply(
    FROM_HERE,
    base::Bind(&Bookmarks::DeleteBookmarkUiWork, base::Unretained(this), local_object_id ),
    base::Bind(&Bookmarks::DeleteBookmarkPostUiFileWork, base::Unretained(this), s_local_object_id )
  );
}

void Bookmarks::DeleteBookmarkUiWork(const int64_t &local_object_id) {
  LOG(ERROR) << "TAGAB brave_sync::Bookmarks::DeleteBookmarkUiWork local_object_id=" << local_object_id;
  DCHECK(local_object_id != -1);
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  const bookmarks::BookmarkNode* node = bookmarks::GetBookmarkNodeByID(model_, local_object_id);
  LOG(ERROR) << "TAGAB brave_sync::Bookmarks::DeleteBookmarkUiWork node=" << node;
  if (!node) {
    return;
  }

  PauseObserver();
  model_->Remove(node);
  ResumeObserver();
}

void Bookmarks::DeleteBookmarkPostUiFileWork(const std::string &s_local_object_id) {
  LOG(ERROR) << "TAGAB brave_sync::Bookmarks::DeleteBookmarkUiWork s_local_object_id=<" << s_local_object_id << ">";
  DCHECK(!s_local_object_id.empty());
  sync_obj_map_->DeleteByLocalId(storage::ObjectMap::Type::Bookmark, s_local_object_id);
}

void Bookmarks::UpdateBookmark(const jslib::SyncRecord &sync_record) {
  const jslib::Bookmark &sync_bookmark = sync_record.GetBookmark();
  LOG(ERROR) << "TAGAB brave_sync::Bookmarks::UpdateBookmark";
  LOG(ERROR) << "TAGAB objectId="<<sync_record.objectId;
  LOG(ERROR) << "TAGAB location="<<sync_bookmark.site.location;
  LOG(ERROR) << "TAGAB title="<<sync_bookmark.site.title;
  LOG(ERROR) << "TAGAB order="<<sync_bookmark.order;
  LOG(ERROR) << "TAGAB parentFolderObjectId="<<sync_record.GetBookmark().parentFolderObjectId;
  DCHECK(model_);

  // Find native bookmark, file thread
  std::string s_local_object_id = sync_obj_map_->GetLocalIdByObjectId(storage::ObjectMap::Type::Bookmark,
    sync_record.objectId);
  DCHECK(!s_local_object_id.empty());
  if (s_local_object_id.empty()) {
    LOG(ERROR) << "TAGAB brave_sync::Bookmarks::UpdateBookmark: could not find local id";
    return;
  }
  int64_t local_object_id = -1;
  if (!base::StringToInt64(s_local_object_id, &local_object_id) || local_object_id == -1) {
    LOG(ERROR) << "TAGAB brave_sync::Bookmarks::UpdateBookmark: could not convert local id";
    return;
  }

  // While we are in file thread, get two variables we actually need just later in UpdateBookmarkUiWork
  std::string s_new_parent_object_id;
  if (!sync_bookmark.parentFolderObjectId.empty()) {
    sync_obj_map_->GetLocalIdByObjectId(storage::ObjectMap::Type::Bookmark,
       sync_bookmark.parentFolderObjectId);
  } else {
    // This a permanent node, will resolve it later in UpdateBookmarkUiWork
  }
  std::string old_order = sync_obj_map_->GetOrderByObjectId(storage::ObjectMap::Bookmark, sync_record.objectId);

  // Jump to UI thread and update title, URL and date
  auto sync_record_ptr = jslib::SyncRecord::Clone(sync_record);
  content::BrowserThread::GetTaskRunnerForThread(content::BrowserThread::UI)->PostTask(
    FROM_HERE, base::Bind(&Bookmarks::UpdateBookmarkUiWork,
         base::Unretained(this),
         base::Passed(std::move(sync_record_ptr)),
         std::move(local_object_id),
         std::move(s_new_parent_object_id),
         std::move(old_order)
       )
   );
}

void Bookmarks::UpdateBookmarkUiWork(
  std::unique_ptr<jslib::SyncRecord> sync_record,
  const int64_t &local_object_id,
  const std::string &s_new_parent_object_id,
  const std::string &old_order) {
  LOG(ERROR) << "TAGAB brave_sync::Bookmarks::UpdateBookmarkUiWork";
  LOG(ERROR) << "TAGAB local_object_id=" << local_object_id;
  LOG(ERROR) << "TAGAB location=" << sync_record->GetBookmark().site.location;
  LOG(ERROR) << "TAGAB title=" << sync_record->GetBookmark().site.title;
  LOG(ERROR) << "TAGAB order=" << sync_record->GetBookmark().order;

  const jslib::Bookmark &sync_bookmark = sync_record->GetBookmark();

  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DCHECK(local_object_id != -1);

  const bookmarks::BookmarkNode* node = bookmarks::GetBookmarkNodeByID(model_, local_object_id);
  LOG(ERROR) << "TAGAB brave_sync::Bookmarks::UpdateBookmarkUiWork node=" << node;
  if (!node) {
    return;
  }

  // Update fields except parent id and order
  PauseObserver();
  model_->SetTitle(node, base::UTF8ToUTF16(sync_bookmark.site.title));
  model_->SetURL(node, GURL(sync_bookmark.site.location));
  model_->SetDateAdded(node, sync_bookmark.site.creationTime);
  ResumeObserver();

  // 1. if order changed => save new order
  // 2. if parent changed => move to new parent
  // 3. if (1) or (2) reorder boomkarks in parent

  int64_t old_parent_id = node->parent()->id();

  //s_new_parent_object_id - from argument, queried in file thread above
  const bookmarks::BookmarkNode* new_parent_node = nullptr;

  if (!s_new_parent_object_id.empty()) {
    int64_t new_parent_id = -1;
    if (base::StringToInt64(s_new_parent_object_id, &new_parent_id) && new_parent_id != -1) {
      new_parent_node = bookmarks::GetBookmarkNodeByID(model_, local_object_id);
    }
  }

  if (!new_parent_node) {
    if (!sync_bookmark.hideInToolbar) {
      new_parent_node = model_->bookmark_bar_node();
      LOG(ERROR) << "TAGAB brave_sync::Bookmarks::UpdateBookmark use bookmark_bar_node";
    } else if (!sync_bookmark.order.empty() && sync_bookmark.order.at(0) == '2') {
      new_parent_node = model_->mobile_node();
      LOG(ERROR) << "TAGAB brave_sync::Bookmarks::UpdateBookmark use mobile_node";
    } else {
      new_parent_node = model_->other_node();
      LOG(ERROR) << "TAGAB brave_sync::Bookmarks::UpdateBookmark use other";
    }
  }

  DCHECK(new_parent_node);

  int64_t new_parent_id = new_parent_node->id();

  //old_order - from argument, queried in file thread above
  const std::string &new_order = sync_bookmark.order;

  LOG(ERROR) << "TAGAB brave_sync::Bookmarks::UpdateBookmarkUiWork orders and parents";
  LOG(ERROR) << "TAGAB old_order=" << old_order;
  LOG(ERROR) << "TAGAB new_order=" << new_order;
  LOG(ERROR) << "TAGAB old_parent_id=" << old_parent_id;
  LOG(ERROR) << "TAGAB new_parent_id=" << new_parent_id;

  // If folder is the same and order is the same, we are done
  if (old_order == new_order && old_parent_id == new_parent_id) {
    LOG(ERROR) << "TAGAB brave_sync::Bookmarks::UpdateBookmarkUiWork order and parent folder are the same, all done";
    return;
  }

  // Move into another folder if required
  if (old_parent_id != new_parent_id) {
    // Add into the end, will reorder later
    int new_index = new_parent_node->child_count();
    PauseObserver();
    model_->Move(node, new_parent_node, new_index);
    ResumeObserver();
  }

  // Jump to File thread, save new order
  // Then in any way reorder bookmarks in new folder
  // The same as during AddBookmark
  controller_exports_->GetTaskRunner()->PostTask(
    FROM_HERE,
    base::Bind(&Bookmarks::AddOrUpdateBookmarkPostUiFileWork,
      base::Unretained(this),
      new_parent_id,
      local_object_id,
      sync_bookmark.order,
      sync_record->objectId)
  );
}

int Bookmarks::CalculateNewIndex(
  const bookmarks::BookmarkNode* new_parent_node,
  const bookmarks::BookmarkNode* old_parent_node,
  const bookmarks::BookmarkNode* target_node,
  const std::string &old_order,
  const std::string &inserted_node_order) {

  // Go through all new_parent_node direct children, UI thread
  std::vector<int64_t> new_parent_children_ids;
  for (int i = 0; i < new_parent_node->child_count(); ++i) {
    const bookmarks::BookmarkNode* current_child = new_parent_node->GetChild(i);
    new_parent_children_ids.push_back(current_child->id());
  }

  // Get the orders from map, FILE thread
  std::vector<std::string> new_parent_children_orders;
  for (size_t i = 0; i < new_parent_children_ids.size(); ++i) {
    std::string current_child_node = sync_obj_map_->GetOrderByLocalObjectId(
      storage::ObjectMap::Bookmark, std::to_string(new_parent_children_ids.at(i)));
    DCHECK(!current_child_node.empty());
    new_parent_children_orders.push_back(current_child_node);
  }

  // Check orders are sorted by orders sorting rules
  bool is_ordered = IsOrdered(new_parent_children_orders);
  DCHECK(is_ordered);

  // If somehow not ordered, should re-order whole folder (UI thread) with
  // BookmarkModel::ReorderChildren . No point to search the position
  // which does not break sorting, because sorting is already broken


  // Remove old_order from `new_parent_children_orders` if present
  const auto &it = std::find(std::begin(new_parent_children_orders),
    std::end(new_parent_children_orders), old_order);
  // TODO, AB: search with std::lower_bound, because it is sorted?
  if (it != std::end(new_parent_children_orders)) {
    new_parent_children_orders.erase(it);
  }

  // Find 0-based index of inserted_node_order which does not break sorting
  int pos_to_insert = GetPositionToInsert(new_parent_children_orders, inserted_node_order);
  DCHECK(pos_to_insert >= 0);
  return pos_to_insert;
}

void Bookmarks::PauseObserver() {
  LOG(ERROR) << "TAGAB brave_sync::Bookmarks::PauseObserver";
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DCHECK(model_);
  DCHECK(observer_is_set_);
  model_->RemoveObserver(this);
  observer_is_set_ = false;
}

void Bookmarks::ResumeObserver() {
  LOG(ERROR) << "TAGAB brave_sync::Bookmarks::ResumeObserver";
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DCHECK(model_);
  DCHECK(observer_is_set_ == false);
  model_->AddObserver(this);
  observer_is_set_ = true;
}

void Bookmarks::GetAllBookmarks_DEPRECATED(std::vector<const bookmarks::BookmarkNode*> &nodes) {
  const size_t max_count = 300;

  ui::TreeNodeIterator<const bookmarks::BookmarkNode> iterator(model_->root_node());
  while (iterator.has_next()) {
    const bookmarks::BookmarkNode* node = iterator.Next();
    if ( model_->is_permanent_node(node)) {
      continue;
    }
    nodes.push_back(node);
    if (nodes.size() == max_count)
      return;
  }
}

void Bookmarks::GetInitialBookmarksWithOrders(
  std::vector<InitialBookmarkNodeInfo> &nodes,
  std::map<const bookmarks::BookmarkNode*, std::string> &order_map) {
  DCHECK(nodes.empty());
  DCHECK(order_map.empty());
  DCHECK(!base_order_.empty());

  LOG(ERROR) << "TAGAB GetInitialBookmarksWithOrders: model_->root_node()" << model_->root_node();
  LOG(ERROR) << "TAGAB GetInitialBookmarksWithOrders: model_->bookmark_bar_node()" << model_->bookmark_bar_node();
  LOG(ERROR) << "TAGAB GetInitialBookmarksWithOrders: model_->other_node()" << model_->other_node();
  LOG(ERROR) << "TAGAB GetInitialBookmarksWithOrders: model_-> mobile_node()" << model_->mobile_node();

  GetInitialBookmarksWithOrdersWork(model_->root_node(), base_order_, nodes, order_map);
}

void Bookmarks::GetInitialBookmarksWithOrdersWork(
  const bookmarks::BookmarkNode* this_parent_node,
  const std::string &this_node_order,
  std::vector<InitialBookmarkNodeInfo> &nodes,
  std::map<const bookmarks::BookmarkNode*, std::string> &order_map) {

  LOG(ERROR) << "TAGAB GetInitialBookmarksWithOrdersWork: this_parent_node->child_count()" << this_parent_node->child_count();

  for (int i = 0; i < this_parent_node->child_count(); ++i) {
    const bookmarks::BookmarkNode* node = this_parent_node->GetChild(i);
    std::string node_order = this_node_order + "." + std::to_string(i + 1); // Index goes from 0, order goes from 0, so "+ 1"

    if (node != model_->root_node()) {
      // Permanent nodes: 'bookmark bar', 'other' or 'mobile' will not be sent to sync backend, just save order.
      // Because they are childeren of root node and we are not allowed
      // to add anything into 'root' bookmark node directly.
      // See BookmarkModel::AddFolderWithMetaInfo or BookmarkModel::AddURLWithCreationTimeAndMetaInfo

      if (model_->is_permanent_node(node)) {
        LOG(ERROR) << "TAGAB GetInitialBookmarksWithOrdersWork: PERM NODE ptr=" << node;
        LOG(ERROR) << "TAGAB GetInitialBookmarksWithOrdersWork: PERM NODE id=" << node->id();
        LOG(ERROR) << "TAGAB GetInitialBookmarksWithOrdersWork: PERM NODE tittle=" << node->GetTitledUrlNodeTitle();
        LOG(ERROR) << "TAGAB GetInitialBookmarksWithOrdersWork: PERM NODE order=" << node_order;
      }

      InitialBookmarkNodeInfo info;
      info.node_ = node;
      info.should_send_ = !model_->is_permanent_node(node);
      nodes.push_back(info);
    }
    // In anyway, even for permanent node, save node to order the map
    order_map[node] = node_order;

    if (!node->empty()) {
      GetInitialBookmarksWithOrdersWork(node, node_order, nodes, order_map);
    }
  }
}

std::unique_ptr<RecordsList> Bookmarks::NativeBookmarksToSyncRecords(
  const std::vector<InitialBookmarkNodeInfo> &list,
  const std::map<const bookmarks::BookmarkNode*, std::string> &order_map,
  int action) {
  LOG(ERROR) << "TAGAB NativeBookmarksToSyncRecords:";
  std::unique_ptr<RecordsList> records = std::make_unique<RecordsList>();

  LOG(ERROR) << "TAGAB NativeBookmarksToSyncRecords: order_map.size()=" << order_map.size();
  for (const auto & the_pair : order_map) {
    LOG(ERROR) << "TAGAB <" << the_pair.first << "> => <" << the_pair.second << ">";
  }

  for (const InitialBookmarkNodeInfo &info : list) {
    const bookmarks::BookmarkNode* node = info.node_;
    LOG(ERROR) << "TAGAB NativeBookmarksToSyncRecords: node=" << node;
    LOG(ERROR) << "TAGAB NativeBookmarksToSyncRecords: node->parent()=" << node->parent();
    LOG(ERROR) << "TAGAB NativeBookmarksToSyncRecords: node->title=" << node->GetTitledUrlNodeTitle();

    int64_t parent_folder_id = node->parent() ? node->parent()->id() : -1;
    LOG(ERROR) << "TAGAB NativeBookmarksToSyncRecords: parent_folder_id=" << parent_folder_id;

    std::string parent_folder_object_sync_id;
    if (parent_folder_id != -1) {
      if (!order_map.empty()) {
        const auto &it_node_parent_pair = order_map.find(node->parent());
        DCHECK(it_node_parent_pair != order_map.end()  || info.should_send_ == false);
        if (it_node_parent_pair != order_map.end()) {
          const std::string parent_node_order = order_map.at(node->parent()); // Segmentation fault here, because there is no such item
          parent_folder_object_sync_id = GetOrCreateObjectByLocalId(parent_folder_id, parent_node_order);
        } else {
          // Could not find the order for parent node
        }
      } else {
        parent_folder_object_sync_id = sync_obj_map_->GetObjectIdByLocalId(
          storage::ObjectMap::Type::Bookmark, std::to_string(parent_folder_id));
      }
    }

    std::string node_order;
    if (!order_map.empty()) {
      node_order = order_map.at(node);
      DCHECK(!node_order.empty());
    } else {
      node_order = sync_obj_map_->GetOrderByLocalObjectId(storage::ObjectMap::Type::Bookmark, std::to_string(node->id()));
    }
    DCHECK(!node_order.empty());
    std::string object_id = GetOrCreateObjectByLocalId(node->id(), node_order);
    LOG(ERROR) << "TAGAB NativeBookmarksToSyncLV: object_id=<" << object_id << ">";
    CHECK(!object_id.empty());

    if (!info.should_send_) {
      LOG(ERROR) << "TAGAB NativeBookmarksToSyncRecords: (!info.should_send_) for " << node->GetTitledUrlNodeTitle();
      continue;
    }

    std::unique_ptr<jslib::SyncRecord> record = std::make_unique<jslib::SyncRecord>();
    record->action = ConvertEnum<brave_sync::jslib::SyncRecord::Action>(action,
        brave_sync::jslib::SyncRecord::Action::A_MIN,
        brave_sync::jslib::SyncRecord::Action::A_MAX,
        brave_sync::jslib::SyncRecord::Action::A_INVALID);
    record->deviceId = device_id_;
    record->objectId = object_id;
    record->objectData = "bookmark";

    std::unique_ptr<jslib::Bookmark> bookmark = std::make_unique<jslib::Bookmark>();
    bookmark->site.location = node->url().spec();
    bookmark->site.title = base::UTF16ToUTF8(node->GetTitledUrlNodeTitle());
    bookmark->site.customTitle = base::UTF16ToUTF8(node->GetTitle());
    //bookmark->site.lastAccessedTime = ;
    bookmark->site.creationTime = node->date_added();
    bookmark->site.favicon = node->icon_url() ? node->icon_url()->spec() : "";
    bookmark->isFolder = node->is_folder();
    bookmark->parentFolderObjectId = parent_folder_object_sync_id;
    //bookmark->fields = ;

    // 'Show in toolbar' means the node is descendant of 'bookmark bar' node
    // 'Hide in toolbar means' the node is not a descendant of 'bookmark bar' node
    bookmark->hideInToolbar = !node->HasAncestor(model_->bookmark_bar_node());
    bookmark->order = node_order;
    record->SetBookmark(std::move(bookmark));

    record->syncTimestamp = base::Time::Now();
    records->emplace_back(std::move(record));
  }

  return records;
}

void Bookmarks::BookmarkModelLoaded(bookmarks::BookmarkModel* model,
    bool ids_reassigned) {
  LOG(ERROR) << "TAGAB brave_sync::Bookmarks::BookmarkModelLoaded";
}

void Bookmarks::BookmarkNodeMoved(bookmarks::BookmarkModel* model,
    const bookmarks::BookmarkNode* old_parent,
    int old_index,
    const bookmarks::BookmarkNode* new_parent,
    int new_index) {
  LOG(ERROR) << "TAGAB brave_sync::Bookmarks::BookmarkNodeMoved";
  LOG(ERROR) << "TAGAB old_parent=" << old_parent->GetTitledUrlNodeTitle();
  LOG(ERROR) << "TAGAB old_index=" << old_index;
  LOG(ERROR) << "TAGAB new_parent=" << new_parent->GetTitledUrlNodeTitle();
  LOG(ERROR) << "TAGAB new_index=" << new_index;
  const bookmarks::BookmarkNode* node = new_parent->GetChild(new_index);
  LOG(ERROR) << "TAGAB node->id()=" << node->id();
  LOG(ERROR) << "TAGAB node->title=" << node->GetTitledUrlNodeTitle();

  if (!controller_exports_ || !controller_exports_->IsSyncConfigured()) {
    LOG(ERROR) << "TAGAB brave_sync::Bookmarks::BookmarkNodeMoved sync is not configured";
    return;
  }

  int64_t prev_item_id = (new_index == 0) ? (-1) : new_parent->GetChild(new_index - 1)->id();
  int64_t next_item_id = (new_index == new_parent->child_count() - 1) ? (-1) : new_parent->GetChild(new_index + 1)->id();
  LOG(ERROR) << "TAGAB prev_item_id=" << prev_item_id;
  LOG(ERROR) << "TAGAB next_item_id=" << next_item_id;

  const bookmarks::BookmarkNode* prev_bookmark = nullptr;
  const bookmarks::BookmarkNode* next_bookmark = nullptr;
  if (new_index > 0) {
    prev_bookmark = new_parent->GetChild(new_index - 1);
    LOG(ERROR) << "TAGAB prev_bookmark->id=" << prev_bookmark->id();
    LOG(ERROR) << "TAGAB prev_bookmark->title=" << prev_bookmark->GetTitledUrlNodeTitle();
  } else {
    LOG(ERROR) << "TAGAB prev_bookmark empty";
  }
  if (new_index != new_parent->child_count() - 1) {
    next_bookmark = new_parent->GetChild(new_index + 1);
    LOG(ERROR) << "TAGAB next_bookmark->id=" << next_bookmark->id();
    LOG(ERROR) << "TAGAB next_bookmark->title=" << next_bookmark->GetTitledUrlNodeTitle();
  } else {
    LOG(ERROR) << "TAGAB next_bookmark empty";
  }

  if (prev_bookmark) {
    DCHECK(prev_bookmark->id() == prev_item_id);
  }
  if (next_bookmark) {
    DCHECK(next_bookmark->id() == next_item_id);
  }

  // Next: ask lib, jump into task runner to get orders with file ops
  controller_exports_->GetTaskRunner()->PostTask(
    FROM_HERE,
    base::Bind(&ControllerForBookmarksExports::BookmarkMoved, base::Unretained(controller_exports_),
    node->id(),
    prev_item_id,
    next_item_id,
    new_parent->id())
  );
}

void Bookmarks::BookmarkNodeAdded(bookmarks::BookmarkModel* model,
    const bookmarks::BookmarkNode* parent,
    int index) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  LOG(ERROR) << "TAGAB brave_sync::Bookmarks::BookmarkNodeAdded " << GetThreadInfoString();
  LOG(ERROR) << "TAGAB brave_sync::Bookmarks::BookmarkNodeAdded model=" << model;

  LOG(ERROR) << "TAGAB brave_sync::Bookmarks::BookmarkNodeAdded parent->is_folder()=" << parent->is_folder();
  LOG(ERROR) << "TAGAB brave_sync::Bookmarks::BookmarkNodeAdded parent->id()=" << parent->id();
  LOG(ERROR) << "TAGAB brave_sync::Bookmarks::BookmarkNodeAdded parent->url().spec()=" << parent->url().spec();
  LOG(ERROR) << "TAGAB brave_sync::Bookmarks::BookmarkNodeAdded parent->GetTitle()=" << parent->GetTitle();
  LOG(ERROR) << "TAGAB brave_sync::Bookmarks::BookmarkNodeAdded GetBookmarkNodeString(parent->type())=" << GetBookmarkNodeString(parent->type());
  LOG(ERROR) << "TAGAB brave_sync::Bookmarks::BookmarkNodeAdded index=" << index;
  const bookmarks::BookmarkNode* node = parent->GetChild(index);
  LOG(ERROR) << "TAGAB brave_sync::Bookmarks::BookmarkNodeAdded node->url().spec()=" << node->url().spec();
  LOG(ERROR) << "TAGAB brave_sync::Bookmarks::BookmarkNodeAdded node->GetTitle()=" << node->GetTitle();
  LOG(ERROR) << "TAGAB brave_sync::Bookmarks::BookmarkNodeAdded GetBookmarkNodeString(node->type())=" << GetBookmarkNodeString(node->type());

  if (!controller_exports_ || !controller_exports_->IsSyncConfigured()) {
    LOG(ERROR) << "TAGAB brave_sync::Bookmarks::BookmarkNodeAdded sync is not configured";
    return;
  }

  int64_t prev_item_id = (index == 0) ? (-1) : parent->GetChild(index - 1)->id();
  int64_t next_item_id = (index == parent->child_count() - 1) ? (-1) : parent->GetChild(index + 1)->id();
  LOG(ERROR) << "TAGAB brave_sync::Bookmarks::BookmarkNodeAdded prev_item_id=" << prev_item_id;
  LOG(ERROR) << "TAGAB brave_sync::Bookmarks::BookmarkNodeAdded next_item_id=" << next_item_id;
  const bookmarks::BookmarkNode* prev_bookmark = nullptr;
  const bookmarks::BookmarkNode* next_bookmark = nullptr;
  if (index > 0) {
    prev_bookmark = parent->GetChild(index - 1);
    LOG(ERROR) << "TAGAB prev_bookmark->id=" << prev_bookmark->id();
    LOG(ERROR) << "TAGAB prev_bookmark->title=" << prev_bookmark->GetTitledUrlNodeTitle();
  } else {
    LOG(ERROR) << "TAGAB prev_bookmark empty";
  }
  if (index != parent->child_count() - 1) {
    next_bookmark = parent->GetChild(index + 1);
    LOG(ERROR) << "TAGAB next_bookmark->id=" << next_bookmark->id();
    LOG(ERROR) << "TAGAB next_bookmark->title=" << next_bookmark->GetTitledUrlNodeTitle();
  } else {
    LOG(ERROR) << "TAGAB next_bookmark empty";
  }

  if (prev_bookmark) {
    DCHECK(prev_bookmark->id() == prev_item_id);
  }
  if (next_bookmark) {
    DCHECK(next_bookmark->id() == next_item_id);
  }

  // Next: ask lib, jump into task runner to get orders with file ops
  controller_exports_->GetTaskRunner()->PostTask(
    FROM_HERE,
    base::Bind(&ControllerForBookmarksExports::BookmarkAdded, base::Unretained(controller_exports_),
    node->id(),
    prev_item_id,
    next_item_id,
    parent->id())
  );
}

void Bookmarks::BookmarkNodeRemoved(
    bookmarks::BookmarkModel* model,
    const bookmarks::BookmarkNode* parent,
    int old_index,
    const bookmarks::BookmarkNode* node,
    const std::set<GURL>& no_longer_bookmarked) {
  LOG(ERROR) << "TAGAB brave_sync::Bookmarks::BookmarkNodeRemoved model="<<model;
  LOG(ERROR) << "TAGAB brave_sync::Bookmarks::BookmarkNodeRemoved parent->url().spec()="<<parent->url().spec();
  LOG(ERROR) << "TAGAB brave_sync::Bookmarks::BookmarkNodeRemoved old_index="<<old_index;
  LOG(ERROR) << "TAGAB brave_sync::Bookmarks::BookmarkNodeRemoved node->url().spec()="<<node->url().spec();
  for (const GURL &url_no_longer_bookmarked : no_longer_bookmarked) {
    LOG(ERROR) << "TAGAB brave_sync::Bookmarks::BookmarkNodeRemoved url_no_longer_bookmarked.spec()="<<url_no_longer_bookmarked.spec();
  }
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  if (!controller_exports_ || !controller_exports_->IsSyncConfigured()) {
    LOG(ERROR) << "TAGAB brave_sync::Bookmarks::BookmarkNodeRemoved sync is not configured";
    return;
  }

  // TODO, AB: what to do with no_longer_bookmarked?
  // How no_longer_bookmarked appears,
  //void BookmarkModel::Remove(const BookmarkNode* node) {
  // std::set<GURL> removed_urls;
  // std::unique_ptr<BookmarkNode> owned_node =
  //     url_index_->Remove(AsMutable(node), &removed_urls);
  //
  //std::unique_ptr<UrlIndex> url_index_;
  // ...
  //void UrlIndex::RemoveImpl(BookmarkNode* node, std::set<GURL>* removed_urls) {
  //   if (removed_urls)
  //     removed_urls->insert(node->url());
  // }
  // for (int i = node->child_count() - 1; i >= 0; --i)
  //   RemoveImpl(node->GetChild(i), removed_urls);
  //
  // no_longer_bookmarked is the set of urls which were removed as child nodes
  // of |node| if the node is a folder
  //
  // This line below works or a single bookmark but should be checked for the folder

  controller_exports_->CreateUpdateDeleteBookmarks(jslib_const::kActionDelete,
    {InitialBookmarkNodeInfo(node, true)}, std::map<const bookmarks::BookmarkNode*, std::string>(), false, false);

  //=> File task
  // node can be dead
  controller_exports_->GetTaskRunner()->PostTask(
    FROM_HERE,
    base::Bind(&Bookmarks::BookmarkNodeRemovedFileWork, base::Unretained(this), node)
  );
}

void Bookmarks::BookmarkNodeRemovedFileWork(const bookmarks::BookmarkNode* node) {
  LOG(ERROR) << "TAGAB brave_sync::Bookmarks::BookmarkNodeRemovedFileWork";
  LOG(ERROR) << "TAGAB node=" << node;
  LOG(ERROR) << "TAGAB node->url().spec()=" << node->url().spec();

  //=> File task
  sync_obj_map_->DeleteByLocalId(storage::ObjectMap::Type::Bookmark, base::NumberToString(node->id()));
}

void Bookmarks::BookmarkNodeChanged(bookmarks::BookmarkModel* model,
    const bookmarks::BookmarkNode* node) {
  LOG(ERROR) << "TAGAB brave_sync::Bookmarks::BookmarkNodeChanged model="<<model;
  LOG(ERROR) << "TAGAB brave_sync::Bookmarks::BookmarkNodeChanged node->url().spec()="<<node->url().spec();
  LOG(ERROR) << "TAGAB brave_sync::Bookmarks::BookmarkNodeChanged node->GetTitledUrlNodeTitle()=" << node->GetTitledUrlNodeTitle();
  LOG(ERROR) << "TAGAB brave_sync::Bookmarks::BookmarkNodeChanged node->GetTitle()=" << node->GetTitle();

  controller_exports_->CreateUpdateDeleteBookmarks(jslib_const::kActionUpdate,
     {InitialBookmarkNodeInfo(node, true)}, std::map<const bookmarks::BookmarkNode*, std::string>(), false, false);
}

void Bookmarks::BookmarkNodeFaviconChanged(bookmarks::BookmarkModel* model,
    const bookmarks::BookmarkNode* node) {
  LOG(ERROR) << "TAGAB brave_sync::Bookmarks::BookmarkNodeFaviconChanged model="<<model;
  LOG(ERROR) << "TAGAB brave_sync::Bookmarks::BookmarkNodeFaviconChanged node->url().spec()="<<node->url().spec();
}

void Bookmarks::BookmarkNodeChildrenReordered(bookmarks::BookmarkModel* model,
    const bookmarks::BookmarkNode* node) {
  LOG(ERROR) << "TAGAB brave_sync::Bookmarks::BookmarkNodeChildrenReordered model="<<model;
  LOG(ERROR) << "TAGAB brave_sync::Bookmarks::BookmarkNodeChildrenReordered node->url().spec()="<<node->url().spec();
}

void Bookmarks::BookmarkAllUserNodesRemoved(
    bookmarks::BookmarkModel* model,
    const std::set<GURL>& removed_urls) {
  LOG(ERROR) << "TAGAB brave_sync::Bookmarks::BookmarkAllUserNodesRemoved model="<<model;
  for (const GURL &removed_url : removed_urls) {
    LOG(ERROR) << "TAGAB brave_sync::Bookmarks::BookmarkAllUserNodesRemoved removed_url.spec()="<<removed_url.spec();
  }
}
void Bookmarks::SetBaseOrder(const std::string &base_order) {
  LOG(ERROR) << "TAGAB brave_sync::Bookmarks::SetBaseOrder base_order="<<base_order;
  LOG(ERROR) << "TAGAB brave_sync::Bookmarks::SetBaseOrder base_order_="<<base_order_;
  DCHECK(base_order_.empty());
  DCHECK(ValidateBookmarksBaseOrder(base_order));

  base_order_ = base_order;
  if (base_order_.length() >= 3 && base_order_.at(base_order_.length() - 1) == '.') {
    base_order_.resize(base_order_.length() - 1);
  }
}


} // namespace brave_sync
