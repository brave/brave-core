/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "brave/components/brave_sync/brave_sync_bookmarks.h"
#include <memory>
#include <string>
#include "base/values.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/components/brave_sync/brave_sync_jslib_const.h"
#include "brave/components/brave_sync/brave_sync_obj_map.h"
#include "brave/components/brave_sync/brave_sync_tools.h"
#include "brave/components/brave_sync/values_conv.h"
#include "chrome/browser/bookmarks/bookmark_model_factory.h"
#include "chrome/browser/ui/browser.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/bookmarks/browser/bookmark_utils.h"
#include "chrome/browser/bookmarks/bookmark_model_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "ui/base/models/tree_node_iterator.h"

namespace brave_sync {

BraveSyncBookmarks::BraveSyncBookmarks() : browser_(nullptr), model_(nullptr),
  sync_obj_map_(nullptr) {
  LOG(ERROR) << "TAGAB BraveSyncBookmarks::GetResolvedBookmarkValue CTOR";
}

BraveSyncBookmarks::~BraveSyncBookmarks() {
  LOG(ERROR) << "TAGAB BraveSyncBookmarks::~GetResolvedBookmarkValue DTOR";
}

void BraveSyncBookmarks::SetBrowser(Browser* browser) {
  LOG(ERROR) << "TAGAB BraveSyncBookmarks::SetBrowser browser="<<browser;
  DCHECK(browser != nullptr);
  if (browser_ == nullptr) {
    browser_ = browser;
    model_ = BookmarkModelFactory::GetForBrowserContext(browser_->profile());
    LOG(ERROR) << "TAGAB BraveSyncBookmarks::SetBrowser model_="<<model_;
    // per profile
  } else {
    LOG(ERROR) << "TAGAB BraveSyncBookmarks::SetModel already set browser_="<<browser_;
  }
}

void BraveSyncBookmarks::SetThisDeviceId(const std::string &device_id) {
  DCHECK(device_id_.empty());
  DCHECK(!device_id.empty());
  device_id_ = device_id;
}

void BraveSyncBookmarks::SetObjMap(storage::BraveSyncObjMap* sync_obj_map) {
  DCHECK(sync_obj_map != nullptr);
  DCHECK(sync_obj_map_ == nullptr);
  sync_obj_map_ = sync_obj_map;
}

std::unique_ptr<base::Value> BraveSyncBookmarks::GetResolvedBookmarkValue(
  const std::string &object_id,
  const std::string &local_object_id) {
  LOG(ERROR) << "TAGAB BraveSyncBookmarks::GetResolvedBookmarkValue local_object_id=<"<<local_object_id<<">";
  DCHECK(!local_object_id.empty());
  CHECK(model_);

  int64_t id = 0;
  bool convert_result = base::StringToInt64(local_object_id, &id);
  DCHECK(convert_result);
  if (!convert_result) {
    return std::make_unique<base::Value>(base::Value::Type::NONE);
  }

  const bookmarks::BookmarkNode* node = bookmarks::GetBookmarkNodeByID(model_, id);
  if (node == nullptr) {
    return std::make_unique<base::Value>(base::Value::Type::NONE);
  }

  std::unique_ptr<base::Value> value = BookmarkToValue(node, object_id);

  return value;
}

std::unique_ptr<base::Value> BraveSyncBookmarks::BookmarkToValue(const bookmarks::BookmarkNode* node,
  const std::string &object_id) {
  LOG(ERROR) << "TAGAB BraveSyncBookmarks::BookmarkToValue node="<<node;

  CHECK(!device_id_.empty());
  CHECK(node != nullptr);
  CHECK(!object_id.empty());

  int64_t parent_folder_id = node->parent() ? node->parent()->id() : 0;
  std::string parent_folder_object_sync_id;
  if (parent_folder_id) {
    parent_folder_object_sync_id = GetOrCreateObjectByLocalId(parent_folder_id);
  }

  std::unique_ptr<base::Value> value = CreateBookmarkSyncRecordValue(
    jslib_const::kActionCreate,
    device_id_,
    object_id,
    //object data - site
    node->url().spec(), //const std::string &location,
    base::UTF16ToUTF8(node->GetTitledUrlNodeTitle()), //const std::string &title,
    base::UTF16ToUTF8(node->GetTitle()),//const std::string &customTitle,
    0,//const uint64_t &lastAccessedTime,
    node->date_added().ToJsTime(), //const uint64_t &creationTime,
    node->icon_url() ? node->icon_url()->spec() : "",//const std::string &favicon,
    //object data - bookmark
    node->is_folder(),//bool isFolder,
    parent_folder_object_sync_id,   //const std::string &parentFolderObjectId,
    //repeated string fields = 6;
    false,//?//bool hideInToolbar,
    ""//const std::string &order
  );

  return value;
}

std::string BraveSyncBookmarks::GetOrCreateObjectByLocalId(const int64_t &local_id) {
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

void BraveSyncBookmarks::AddBookmark(const std::string &location, const std::string &title) {
  LOG(ERROR) << "TAGAB BraveSyncBookmarks::AddBookmark location="<<location;
  LOG(ERROR) << "TAGAB BraveSyncBookmarks::AddBookmark title="<<title;
  DCHECK(model_);
  if (model_ == nullptr) {
    return;
  }

  PauseObserver();
  base::string16 title16 = base::UTF8ToUTF16(title);
  // TODO, AB: AddIfNotBookmarked is used for tests only, replace with an appropriate
  bookmarks::AddIfNotBookmarked(model_,
                          GURL(location),
                          title16);
  ResumeObserver();
}

void BraveSyncBookmarks::PauseObserver() {
  LOG(ERROR) << "TAGAB BraveSyncBookmarks::PauseObserver";
}

void BraveSyncBookmarks::ResumeObserver() {
  LOG(ERROR) << "TAGAB BraveSyncBookmarks::ResumeObserver";
}

void BraveSyncBookmarks::GetAllBookmarks(std::vector<const bookmarks::BookmarkNode*> &nodes) {
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

std::unique_ptr<base::Value> BraveSyncBookmarks::NativeBookmarksToSyncLV(const std::vector<const bookmarks::BookmarkNode*> &list,
  int action) {
  LOG(ERROR) << "TAGAB NativeBookmarksToSyncLV:";
  auto result = std::make_unique<base::Value>(base::Value::Type::LIST);

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

    std::unique_ptr<base::Value> bookmark_sync_record =
      CreateBookmarkSyncRecordValue(
        action, // int action, // kActionCreate/kActionUpdate/kActionDelete 0/1/2
        device_id_,// const std::string &device_id,
        object_id,// const std::string &object_id,
        // //object data - site
        node->url().spec(),//const std::string &location,
        base::UTF16ToUTF8(node->GetTitledUrlNodeTitle()), //const std::string &title,
        base::UTF16ToUTF8(node->GetTitle()),//const std::string &customTitle,
        0,//const uint64_t &lastAccessedTime,
        node->date_added().ToJsTime(), //const uint64_t &creationTime,
        node->icon_url() ? node->icon_url()->spec() : "",//const std::string &favicon,
        // //object data - bookmark
        node->is_folder(),//bool isFolder,
        parent_folder_object_sync_id,//const std::string &parentFolderObjectId,
        // //repeated string fields = 6;
        false,//?//bool hideInToolbar,
        ""//const std::string &order
      );

    std::string extracted = ExtractObjectIdFromList(bookmark_sync_record.get());
    LOG(ERROR) << "TAGAB NativeBookmarksToSyncLV: extracted=<" << extracted << ">";
    CHECK(!extracted.empty());

    result->GetList().emplace_back(std::move(*bookmark_sync_record));
  }

  return result;
}

} // namespace brave_sync
