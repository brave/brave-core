/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef BRAVE_COMPONENTS_BRAVE_SYNC_BRAVE_SYNC_BOOKMARKS_CLIENT_H
#define BRAVE_COMPONENTS_BRAVE_SYNC_BRAVE_SYNC_BOOKMARKS_CLIENT_H

namespace base {
  class SequencedTaskRunner;
}

namespace bookmarks {
  class BookmarkNode;
}

namespace brave_sync {

class InitialBookmarkNodeInfo;

class BookmarksClient {
public:
  virtual ~BookmarksClient() = default;

  // Not sure about to place it here or to have a separate interface
  virtual void CreateUpdateDeleteBookmarks(
    const int &action,
    const std::vector<InitialBookmarkNodeInfo> &list,
    const std::map<const bookmarks::BookmarkNode*, std::string> &order_map,
    const bool &addIdsToNotSynced,
    const bool &isInitialSync) = 0;

  virtual void BookmarkMoved(
    const int64_t &node_id,
    const int64_t &prev_item_id,
    const int64_t &next_item_id,
    const int64_t &parent_id) = 0;

  virtual void BookmarkAdded(
    const int64_t &node_id,
    const int64_t &prev_item_id,
    const int64_t &next_item_id,
    const int64_t &parent_id) = 0;

  virtual base::SequencedTaskRunner *GetTaskRunner() = 0;

  virtual bool IsSyncConfigured() = 0;
  virtual bool IsSyncInitialized() = 0;
};

} // namespace brave_sync

#endif //BRAVE_COMPONENTS_BRAVE_SYNC_BRAVE_SYNC_BOOKMARKS_CLIENT_H
