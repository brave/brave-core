/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef H_BRAVE_COMPONENTS_BRAVE_SYNC_BRAVE_SYNC_BOOKMARKS_H_
#define H_BRAVE_COMPONENTS_BRAVE_SYNC_BRAVE_SYNC_BOOKMARKS_H_

#include <string>
#include <vector>
#include <map>

#include "base/macros.h"

#include "components/bookmarks/browser/bookmark_model_observer.h"
#include "brave/components/brave_sync/jslib_messages.h" // To get nested enum SyncRecord::Action

class Profile;

namespace bookmarks {
  class BookmarkModel;
  class BookmarkNode;
} // namespace bookmarks

namespace brave_sync {

namespace jslib {
  class SyncRecord;
  class Bookmark;
}
typedef std::unique_ptr<jslib::SyncRecord> SyncRecordPtr;
typedef std::vector<SyncRecordPtr> RecordsList;

namespace storage {
  class ObjectMap;
}

class Controller;
class ControllerForBookmarksExports;

class InitialBookmarkNodeInfo {
public:
  InitialBookmarkNodeInfo(){}
  InitialBookmarkNodeInfo(const bookmarks::BookmarkNode* node, bool should_send) : node_(node), should_send_(should_send) {}
  const bookmarks::BookmarkNode* node_ = nullptr;
  bool should_send_ = true;
};

class Bookmarks : public bookmarks::BookmarkModelObserver {
public:
  Bookmarks(ControllerForBookmarksExports *controller_exports);
  ~Bookmarks() override;
  void SetProfile(Profile *profile);
  void SetThisDeviceId(const std::string &device_id);
  void SetObjectMap(storage::ObjectMap* sync_obj_map);

  const bookmarks::BookmarkNode* GetNodeById(const int64_t &bookmark_local_id);

  std::unique_ptr<jslib::SyncRecord> GetResolvedBookmarkValue(
    const std::string &object_id,
    const jslib::SyncRecord::Action &action);

  // Requests from sync lib
  void AddBookmark(const jslib::SyncRecord &sync_record);
  void DeleteBookmark(const jslib::SyncRecord &sync_record);
  void UpdateBookmark(const jslib::SyncRecord &sync_record);

  void GetAllBookmarks_DEPRECATED(std::vector<const bookmarks::BookmarkNode*> &nodes);
  std::unique_ptr<RecordsList> NativeBookmarksToSyncRecords(const std::vector<const bookmarks::BookmarkNode*> &list, int action);

  void GetInitialBookmarksWithOrders(
    std::vector<InitialBookmarkNodeInfo> &nodes,
    std::map<const bookmarks::BookmarkNode*, std::string> &order_map);

  std::unique_ptr<RecordsList> NativeBookmarksToSyncRecords(
    const std::vector<InitialBookmarkNodeInfo> &list,
    const std::map<const bookmarks::BookmarkNode*, std::string> &order_map,
    int action);


  // bookmarks::BookmarkModelObserver overrides
  void BookmarkModelLoaded(bookmarks::BookmarkModel* model,
                           bool ids_reassigned) override;

  void BookmarkNodeMoved(bookmarks::BookmarkModel* model,
                         const bookmarks::BookmarkNode* old_parent,
                         int old_index,
                         const bookmarks::BookmarkNode* new_parent,
                         int new_index) override;

  void BookmarkNodeAdded(bookmarks::BookmarkModel* model,
                         const bookmarks::BookmarkNode* parent,
                         int index) override;

  void BookmarkNodeRemoved(
      bookmarks::BookmarkModel* model,
      const bookmarks::BookmarkNode* parent,
      int old_index,
      const bookmarks::BookmarkNode* node,
      const std::set<GURL>& no_longer_bookmarked) override;

  void BookmarkNodeChanged(bookmarks::BookmarkModel* model,
                           const bookmarks::BookmarkNode* node) override;


  void BookmarkNodeFaviconChanged(bookmarks::BookmarkModel* model,
                                  const bookmarks::BookmarkNode* node) override;


  void BookmarkNodeChildrenReordered(bookmarks::BookmarkModel* model,
                                     const bookmarks::BookmarkNode* node) override;

  void BookmarkAllUserNodesRemoved(
      bookmarks::BookmarkModel* model,
      const std::set<GURL>& removed_urls) override;
  void SetBaseOrder(const std::string &base_order);

private:

  void GetInitialBookmarksWithOrdersWork(
    const bookmarks::BookmarkNode* this_parent_node,
    const std::string &this_node_order,
    std::vector<InitialBookmarkNodeInfo> &nodes,
    std::map<const bookmarks::BookmarkNode*, std::string> &order_map);

  void AddBookmarkUiWork(std::unique_ptr<jslib::SyncRecord> sync_record,
                         const std::string &s_parent_local_object_id,
                         int64_t* parent_folder_id, int64_t* added_node_id);
  void AddOrUpdateBookmarkPostUiFileWork(const int64_t &folder_id, const int64_t &added_node_id, const std::string &order, const std::string &sync_record_object_id);

  void DeleteBookmarkUiWork(const int64_t &local_object_id);
  void DeleteBookmarkPostUiFileWork(const std::string &s_local_object_id);

  void UpdateBookmarkUiWork(std::unique_ptr<jslib::SyncRecord> sync_record,
    const int64_t &local_object_id,
    const std::string &s_new_parent_object_id,
    const std::string &old_order);

  std::string GetOrCreateObjectByLocalId(const int64_t &local_id, const std::string &order);
  void SaveIdMap(const int64_t &local_id, const std::string &order, const std::string &sync_object_id);
  std::unique_ptr<jslib::Bookmark> GetFromNode(const bookmarks::BookmarkNode* node,
    const std::string &node_order,
    const std::string &parent_order);

  void BookmarkNodeRemovedFileWork(const bookmarks::BookmarkNode* node);

  int CalculateNewIndex(
    const bookmarks::BookmarkNode* new_parent_node,
    const bookmarks::BookmarkNode* old_parent_node,
    const bookmarks::BookmarkNode* target_node,
    const std::string &old_order,
    const std::string &inserted_node_order);

  void ReorderFolderUiWorkCollectChildren(const int64_t &folder_id);
  void ReorderFolderFileWorkCalculateSortedIndexes(const int64_t &folder_id, const std::vector<int64_t> &children_local_ids);
  void ReorderFolderUiWorkApplyIndexes(const int64_t &folder_id, const std::vector<int64_t> &sorted_children);

  void PauseObserver();
  void ResumeObserver();

  DISALLOW_COPY_AND_ASSIGN(Bookmarks);
  Profile *profile_;
  bookmarks::BookmarkModel* model_;
  std::string device_id_;

  storage::ObjectMap* sync_obj_map_;    // Ideally bookmarks should not know about object_map
  bool observer_is_set_;
  std::string base_order_;

  ControllerForBookmarksExports *controller_exports_;
};

} // namespace brave_sync

#endif // H_BRAVE_COMPONENTS_BRAVE_SYNC_BRAVE_SYNC_BOOKMARKS_H_
