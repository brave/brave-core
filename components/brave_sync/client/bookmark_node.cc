#include "brave/components/brave_sync/client/bookmark_node.h"

namespace brave_sync {

BraveBookmarkNode::BraveBookmarkNode(int64_t id, const GURL& url, Type type)
    : BookmarkNode(id, url, type, false) {}

BraveBookmarkPermanentNode::BraveBookmarkPermanentNode(int64_t id, Type type)
    : bookmarks::BookmarkPermanentNode(id, type) {}

BraveBookmarkPermanentNode::~BraveBookmarkPermanentNode() = default;

bool BraveBookmarkPermanentNode::IsVisible() const {
  return visible_;
}

}  // namespace brave_sync
