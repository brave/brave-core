/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef H_BRAVE_COMPONENTS_BRAVE_SYNC_BRAVE_SYNC_BOOKMARKS_H_
#define H_BRAVE_COMPONENTS_BRAVE_SYNC_BRAVE_SYNC_BOOKMARKS_H_

#include "base/macros.h"
#include "base/values.h"
#include "components/bookmarks/browser/bookmark_model_observer.h"

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
class CanSendSyncBookmarks;

class Bookmarks : public bookmarks::BookmarkModelObserver {
public:
  Bookmarks(CanSendSyncBookmarks *send_bookmarks);
  ~Bookmarks() override;
  void SetProfile(Profile *profile);
  void SetThisDeviceId(const std::string &device_id);
  void SetObjectMap(storage::ObjectMap* sync_obj_map);

  std::unique_ptr<jslib::SyncRecord> GetResolvedBookmarkValue2(
    const std::string &object_id);

  void AddBookmark(const jslib::SyncRecord &sync_record);

  void GetAllBookmarks(std::vector<const bookmarks::BookmarkNode*> &nodes);
  std::unique_ptr<RecordsList> NativeBookmarksToSyncRecords(const std::vector<const bookmarks::BookmarkNode*> &list, int action);

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

private:
  std::string GetOrCreateObjectByLocalId(const int64_t &local_id);
  void SaveIdMap(const int64_t &local_id, const std::string &sync_object_id);
  std::unique_ptr<jslib::Bookmark> GetFromNode(const bookmarks::BookmarkNode* node);

  void PauseObserver();
  void ResumeObserver();

  DISALLOW_COPY_AND_ASSIGN(Bookmarks);
  Profile *profile_;
  bookmarks::BookmarkModel* model_;
  std::string device_id_;

  storage::ObjectMap* sync_obj_map_;
  bool observer_is_set_;

  CanSendSyncBookmarks *send_bookmarks_;
};

} // namespace brave_sync

#endif // H_BRAVE_COMPONENTS_BRAVE_SYNC_BRAVE_SYNC_BOOKMARKS_H_
