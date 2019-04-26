#include "brave/components/brave_sync/syncer_helper.h"
#include "../../../../components/sync_bookmarks/bookmark_change_processor.cc"
#include "components/bookmarks/browser/bookmark_model.h"

namespace sync_bookmarks {

bool BookmarkChangeProcessor::IsFirstLoadedFavicon(const BookmarkNode* node) {
  // Avoid sending duplicate records right after applying CREATE records,
  // BookmarkChangeProcessor::SetBookmarkFavicon, put favicon data into database
  // BookmarkNode::favicon() and BookmarkNode::icon_url() are available only
  // after first successfuly BookmarkModel::GetFavicon() which means
  // BookmarkModel::OnFaviconDataAvailable has image result available.
  // So we set metainfo to know if it is first time favicon load after create
  // node from remote record
  std::string FirstLoadedFavicon;
  if (node->GetMetaInfo("FirstLoadedFavicon", &FirstLoadedFavicon)) {
    if (!node->icon_url())
      return true;
    bookmark_model_->RemoveObserver(this);
    BookmarkNode* mutable_node = const_cast<BookmarkNode*>(node);
    mutable_node->DeleteMetaInfo("FirstLoadedFavicon");
    bookmark_model_->AddObserver(this);
    return true;
  }
  return false;
}

}   // namespace sync_bookmarks
