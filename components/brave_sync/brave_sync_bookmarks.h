/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef H_BRAVE_COMPONENTS_BRAVE_SYNC_BRAVE_SYNC_BOOKMARKS_H_
#define H_BRAVE_COMPONENTS_BRAVE_SYNC_BRAVE_SYNC_BOOKMARKS_H_

#include "base/macros.h"
#include "base/values.h"

class Browser;

namespace bookmarks {
  class BookmarkModel;
  class BookmarkNode;
} // namespace bookmarks

namespace brave_sync {

namespace storage {
  class BraveSyncObjMap;
}

class BraveSyncBookmarks {
public:
  BraveSyncBookmarks();
  virtual ~BraveSyncBookmarks();
  void SetBrowser(Browser* browser);
  void SetThisDeviceId(const std::string &device_id);
  void SetObjMap(storage::BraveSyncObjMap* sync_obj_map);

  std::unique_ptr<base::Value> GetResolvedBookmarkValue(
    const std::string &object_id,
    const std::string &local_object_id);

  void AddBookmark(const std::string &location, const std::string &title);

  void GetAllBookmarks(std::vector<const bookmarks::BookmarkNode*> &nodes);
  std::unique_ptr<base::Value>NativeBookmarksToSyncLV(const std::vector<const bookmarks::BookmarkNode*> &list, int action);

private:
  std::unique_ptr<base::Value> BookmarkToValue(const bookmarks::BookmarkNode* node, const std::string &object_id);

  std::string GetOrCreateObjectByLocalId(const int64_t &local_id);

  void PauseObserver();
  void ResumeObserver();

  DISALLOW_COPY_AND_ASSIGN(BraveSyncBookmarks);
  Browser* browser_;
  bookmarks::BookmarkModel* model_;
  std::string device_id_;

  storage::BraveSyncObjMap* sync_obj_map_;
};

} // namespace brave_sync

#endif // H_BRAVE_COMPONENTS_BRAVE_SYNC_BRAVE_SYNC_BOOKMARKS_H_
