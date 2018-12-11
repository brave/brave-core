#include "brave/components/brave_sync/client/bookmark_node.h"

namespace brave_sync {

BraveBookmarkPermanentNode::BraveBookmarkPermanentNode(int64_t id)
    : bookmarks::BookmarkPermanentNode(id) {}

BraveBookmarkPermanentNode::~BraveBookmarkPermanentNode() = default;

bool BraveBookmarkPermanentNode::IsVisible() const {
  return visible_;
}

}  // namespace brave_sync
