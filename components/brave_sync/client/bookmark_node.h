/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SYNC_CLIENT_BOOKMARK_NODE_H_
#define BRAVE_COMPONENTS_BRAVE_SYNC_CLIENT_BOOKMARK_NODE_H_

#include "components/bookmarks/browser/bookmark_node.h"

namespace brave_sync {

class BraveBookmarkNode : public bookmarks::BookmarkNode {
 public:
  explicit BraveBookmarkNode(int64_t id, const GURL& url, Type type);

 private:
  DISALLOW_COPY_AND_ASSIGN(BraveBookmarkNode);
};

// Sync Managed PerrmanentNode
class BraveBookmarkPermanentNode : public bookmarks::BookmarkPermanentNode {
 public:
  explicit BraveBookmarkPermanentNode(int64_t id, Type type);
  ~BraveBookmarkPermanentNode() override;

  void set_visible(bool value) { visible_ = value; }

  // BookmarkNode overrides:
  bool IsVisible() const override;

 private:
  bool visible_ = false;

  DISALLOW_COPY_AND_ASSIGN(BraveBookmarkPermanentNode);
};

}  // namespace brave_sync

#endif  // BRAVE_COMPONENTS_BRAVE_SYNC_CLIENT_BOOKMARK_NODE_H_
