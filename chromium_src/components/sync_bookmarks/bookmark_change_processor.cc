#include "components/bookmarks/browser/bookmark_model.h"

namespace {

class ScopedPauseObserver {
 public:
  explicit ScopedPauseObserver(bookmarks::BookmarkModel* model,
                               bookmarks::BookmarkModelObserver* observer) :
      model_(model),  observer_(observer) {
    DCHECK_NE(observer_, nullptr);
    DCHECK_NE(model_, nullptr);
    model_->RemoveObserver(observer_);
  }
  ~ScopedPauseObserver() {
    model_->AddObserver(observer_);
  }

 private:
  bookmarks::BookmarkModel* model_;  // Not owned
  bookmarks::BookmarkModelObserver* observer_;
};

}   // namespace

#include "brave/components/brave_sync/syncer_helper.h"
#include "../../../../components/sync_bookmarks/bookmark_change_processor.cc"

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
    ScopedPauseObserver pause(bookmark_model_, this);
    BookmarkNode* mutable_node = const_cast<BookmarkNode*>(node);
    mutable_node->DeleteMetaInfo("FirstLoadedFavicon");
    return true;
  }
  return false;
}

}   // namespace sync_bookmarks
