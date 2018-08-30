/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "brave/components/brave_sync/bookmarks.h"
#include <memory>
#include <string>
#include "base/values.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/components/brave_sync/cansendbookmarks.h"
#include "brave/components/brave_sync/jslib_const.h"
#include "brave/components/brave_sync/jslib_messages.h"
#include "brave/components/brave_sync/client/client.h"
#include "brave/components/brave_sync/object_map.h"
#include "brave/components/brave_sync/tools.h"
#include "brave/components/brave_sync/debug.h"
#include "brave/components/brave_sync/values_conv.h"
#include "chrome/browser/bookmarks/bookmark_model_factory.h"
#include "chrome/browser/ui/browser.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/bookmarks/browser/bookmark_utils.h"
#include "components/bookmarks/browser/bookmark_model_observer.h"
#include "chrome/browser/bookmarks/bookmark_model_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "ui/base/models/tree_node_iterator.h"

namespace brave_sync {

Bookmarks::Bookmarks(CanSendSyncBookmarks *send_bookmarks) :
  browser_(nullptr), model_(nullptr), sync_obj_map_(nullptr),
  observer_is_set_(false), send_bookmarks_(send_bookmarks) {
  LOG(ERROR) << "TAGAB brave_sync::Bookmarks::Bookmarks CTOR";
}

Bookmarks::~Bookmarks() {
  LOG(ERROR) << "TAGAB brave_sync::Bookmarks::~Bookmarks DTOR";
  if (model_ && observer_is_set_) {
    model_->RemoveObserver(this);
  }
}

void Bookmarks::SetBrowser(Browser* browser) {
  LOG(ERROR) << "TAGAB brave_sync::Bookmarks::SetBrowser browser="<<browser;
  DCHECK(browser != nullptr);
  if (browser_ == nullptr) {
    browser_ = browser;
    model_ = BookmarkModelFactory::GetForBrowserContext(browser_->profile());
    LOG(ERROR) << "TAGAB brave_sync::Bookmarks::SetBrowser model_="<<model_;
    DCHECK(observer_is_set_ == false);
    model_->AddObserver(this);
    observer_is_set_ = true;
    // per profile
  } else {
    LOG(ERROR) << "TAGAB brave_sync::Bookmarks::SetModel already set browser_="<<browser_;
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


std::unique_ptr<jslib::Bookmark> Bookmarks::GetFromNode(const bookmarks::BookmarkNode* node) {
  auto bookmark = std::make_unique<jslib::Bookmark>();

  int64_t parent_folder_id = node->parent() ? node->parent()->id() : 0;
  std::string parent_folder_object_sync_id;
  if (parent_folder_id) {
    parent_folder_object_sync_id = GetOrCreateObjectByLocalId(parent_folder_id);
  }


  bookmark->site.location = node->url().spec();
  bookmark->site.title = base::UTF16ToUTF8(node->GetTitledUrlNodeTitle());
  bookmark->site.customTitle = base::UTF16ToUTF8(node->GetTitle());
  //bookmark->site.lastAccessedTime = ; ??
  bookmark->site.creationTime = node->date_added();
  bookmark->site.favicon = node->icon_url() ? node->icon_url()->spec() : "";

  bookmark->isFolder = node->is_folder();
  bookmark->parentFolderObjectId = parent_folder_object_sync_id; // bytes

  //bookmark->hideInToolbar = false; ??
  bookmark->order = ""; // order should be taken from obj map

  return bookmark;
}

std::unique_ptr<jslib::SyncRecord> Bookmarks::GetResolvedBookmarkValue2(
  const std::string &object_id) {
  LOG(ERROR) << "TAGAB brave_sync::Bookmarks::GetResolvedBookmarkValue2 object_id=<"<<object_id<<">";
  std::string local_object_id = sync_obj_map_->GetLocalIdByObjectId(object_id);
  LOG(ERROR) << "TAGAB brave_sync::Bookmarks::GetResolvedBookmarkValue2 local_object_id=<"<<local_object_id<<">";
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
    LOG(ERROR) << "TAGAB brave_sync::Bookmarks::GetResolvedBookmarkValue2 node not found for local_object_id=<"<<local_object_id<<">";
    // Node was removed
    // NOTREACHED() << "means we had not removed (object_id => local_id) pair from objects map";
    // Something gone wrong previously, no obvious way to fix

    return nullptr;
  }

  //std::unique_ptr<base::Value> value = BookmarkToValue(node, object_id);
  auto record = std::make_unique<jslib::SyncRecord>();
  record->action = jslib::SyncRecord::Action::CREATE;
  record->deviceId = device_id_;
  record->objectId = object_id;

  std::unique_ptr<jslib::Bookmark> bookmark = GetFromNode(node);
  record->SetBookmark(std::move(bookmark));

  return record;
}

std::string Bookmarks::GetOrCreateObjectByLocalId(const int64_t &local_id) {
  CHECK(sync_obj_map_);
  const std::string s_local_id = base::Int64ToString(local_id);
  std::string object_id = sync_obj_map_->GetObjectIdByLocalId(s_local_id);
  if (!object_id.empty()) {
    return object_id;
  }

  object_id = tools::GenerateObjectId(); // TODO, AB: pack 8 bytes from s_local_id?
  sync_obj_map_->SaveObjectId(
        s_local_id,
        "",// order or empty
        object_id);

  return object_id;
}

void Bookmarks::SaveIdMap(const int64_t &local_id, const std::string &sync_object_id) {
  CHECK(sync_obj_map_);
  const std::string s_local_id = base::Int64ToString(local_id);
  sync_obj_map_->SaveObjectId(
        s_local_id,
        "",// order or empty
        sync_object_id);
  LOG(ERROR) << "TAGAB brave_sync::Bookmarks::SaveIdMap <"<<s_local_id<<"> ==> <"<<sync_object_id<<">";
}

void Bookmarks::AddBookmark(const jslib::SyncRecord &sync_record) {
  const jslib::Bookmark &sync_bookmark = sync_record.GetBookmark();
  LOG(ERROR) << "TAGAB brave_sync::Bookmarks::AddBookmark location="<<sync_bookmark.site.location;
  LOG(ERROR) << "TAGAB brave_sync::Bookmarks::AddBookmark title="<<sync_bookmark.site.title;
  DCHECK(model_);
  if (model_ == nullptr) {
    return;
  }

  PauseObserver();

  // std::unique_ptr<BookmarkNode> new_node =
  //     std::make_unique<BookmarkNode>(generate_next_node_id(), url);
  // new_node->SetTitle(title);
  // new_node->set_date_added(creation_time);
  // new_node->set_type(BookmarkNode::URL);
  // cannot use, ^-- are private

  const bookmarks::BookmarkNode* parent = bookmarks::GetParentForNewNodes(model_);

  base::string16 title16 = base::UTF8ToUTF16(sync_bookmark.site.title);
  const bookmarks::BookmarkNode* added_node = model_->AddURLWithCreationTimeAndMetaInfo(
      parent,
      parent->child_count(),//int index,
      title16,
      GURL(sync_bookmark.site.location),
      sync_bookmark.site.creationTime, //const base::Time& creation_time,
      nullptr//const BookmarkNode::MetaInfoMap* meta_info
    );
  LOG(ERROR) << "TAGAB brave_sync::Bookmarks::AddBookmark added_node="<<added_node;
  LOG(ERROR) << "TAGAB brave_sync::Bookmarks::AddBookmark added_node->id()="<<added_node->id();
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

  // Save id:
  // std::string local_object_id = sync_obj_map_->GetLocalIdByObjectId(object_id);
  //
  SaveIdMap(added_node->id(), sync_record.objectId);

  ResumeObserver();
}

void Bookmarks::PauseObserver() {
  LOG(ERROR) << "TAGAB brave_sync::Bookmarks::PauseObserver";
  DCHECK(model_);
  DCHECK(observer_is_set_);
  model_->RemoveObserver(this);
  observer_is_set_ = false;
}

void Bookmarks::ResumeObserver() {
  LOG(ERROR) << "TAGAB brave_sync::Bookmarks::ResumeObserver";
  DCHECK(model_);
  DCHECK(observer_is_set_ == false);
  model_->AddObserver(this);
  observer_is_set_ = true;
}

void Bookmarks::GetAllBookmarks(std::vector<const bookmarks::BookmarkNode*> &nodes) {
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

std::unique_ptr<RecordsList> Bookmarks::NativeBookmarksToSyncRecords(
  const std::vector<const bookmarks::BookmarkNode*> &list, int action) {
  LOG(ERROR) << "TAGAB NativeBookmarksToSyncLV:";
  std::unique_ptr<RecordsList> records = std::make_unique<RecordsList>();

  for (const bookmarks::BookmarkNode* node : list) {
    LOG(ERROR) << "TAGAB NativeBookmarksToSyncLV: node=" << node;

    int64_t parent_folder_id = node->parent() ? node->parent()->id() : 0;
    std::string parent_folder_object_sync_id;
    if (parent_folder_id) {
      parent_folder_object_sync_id = GetOrCreateObjectByLocalId(parent_folder_id);
    }

    std::string object_id = GetOrCreateObjectByLocalId(node->id());
    LOG(ERROR) << "TAGAB NativeBookmarksToSyncLV: object_id=<" << object_id << ">";
    CHECK(!object_id.empty());

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
    //bookmark->hideInToolbar = ;
    //bookmark->order = ;
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
}

void Bookmarks::BookmarkNodeAdded(bookmarks::BookmarkModel* model,
    const bookmarks::BookmarkNode* parent,
    int index) {
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

  // Send to sync cloud
  send_bookmarks_->CreateUpdateDeleteBookmarks(jslib_const::kActionCreate,
    {node}, false, false);
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

  sync_obj_map_->DeleteByLocalId(base::NumberToString(node->id()));

  send_bookmarks_->CreateUpdateDeleteBookmarks(jslib_const::kActionDelete,
    {node}, false, false);
}

void Bookmarks::BookmarkNodeChanged(bookmarks::BookmarkModel* model,
    const bookmarks::BookmarkNode* node) {
  LOG(ERROR) << "TAGAB brave_sync::Bookmarks::BookmarkNodeChanged model="<<model;
  LOG(ERROR) << "TAGAB brave_sync::Bookmarks::BookmarkNodeChanged node->url().spec()="<<node->url().spec();
  LOG(ERROR) << "TAGAB brave_sync::Bookmarks::BookmarkNodeAdded node->GetTitledUrlNodeTitle()=" << node->GetTitledUrlNodeTitle();
  LOG(ERROR) << "TAGAB brave_sync::Bookmarks::BookmarkNodeAdded node->GetTitle()=" << node->GetTitle();

  send_bookmarks_->CreateUpdateDeleteBookmarks(jslib_const::kActionUpdate,
     {node}, false, false);
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


} // namespace brave_sync
