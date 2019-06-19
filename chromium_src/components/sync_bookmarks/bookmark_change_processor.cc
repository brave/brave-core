/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/sync_bookmarks/bookmark_change_processor.h"

#include "brave/components/brave_sync/syncer_helper.h"
#include "components/bookmarks/browser/bookmark_model.h"

using bookmarks::BookmarkModel;
using bookmarks::BookmarkModelObserver;
using bookmarks::BookmarkNode;
using sync_bookmarks::BookmarkChangeProcessor;

namespace {

class ScopedPauseObserver {
 public:
  explicit ScopedPauseObserver(BookmarkModel* model,
                               BookmarkModelObserver* observer) :
      model_(model),  observer_(observer) {
    DCHECK_NE(observer_, nullptr);
    DCHECK_NE(model_, nullptr);
    model_->RemoveObserver(observer_);
  }
  ~ScopedPauseObserver() {
    model_->AddObserver(observer_);
  }

 private:
  BookmarkModel* model_;  // Not owned
  BookmarkModelObserver* observer_;  // Not owned
};

bool IsFirstLoadedFavicon(BookmarkChangeProcessor* bookmark_change_processor,
                          BookmarkModel* bookmark_model,
                          const BookmarkNode* node) {
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
    ScopedPauseObserver pause(bookmark_model, bookmark_change_processor);
    BookmarkNode* mutable_node = const_cast<BookmarkNode*>(node);
    mutable_node->DeleteMetaInfo("FirstLoadedFavicon");
    return true;
  }
  return false;
}

}   // namespace

#define BRAVE_BOOKMARK_CHANGE_PROCESSOR_BOOKMARK_NODE_FAVICON_CHANGED \
  if (IsFirstLoadedFavicon(this, bookmark_model_, node)) return;

#define BRAVE_BOOKMARK_CHANGE_PROCESSOR_BOOKMARK_NODE_MOVED_1 \
  ScopedPauseObserver pause(bookmark_model_, this); \
  brave_sync::AddBraveMetaInfo(child, model, old_parent != new_parent); \
  SetSyncNodeMetaInfo(child, &sync_node);

#define BRAVE_BOOKMARK_CHANGE_PROCESSOR_BOOKMARK_NODE_MOVED_2 \
  BookmarkNodeChildrenReordered(model, new_parent); \
  if (old_parent != new_parent) \
    BookmarkNodeChildrenReordered(model, old_parent);

#define BRAVE_BOOKMARK_CHANGE_PROCESSOR_CHILDREN_REORDERED \
      ScopedPauseObserver pause(bookmark_model_, this); \
      brave_sync::AddBraveMetaInfo(child, model, false); \
      SetSyncNodeMetaInfo(child, &sync_child);

#include "../../../../components/sync_bookmarks/bookmark_change_processor.cc"   // NOLINT
#undef BRAVE_BOOKMARK_CHANGE_PROCESSOR_BOOKMARK_NODE_FAVICON_CHANGED
#undef BRAVE_BOOKMARK_CHANGE_PROCESSOR_BOOKMARK_NODE_MOVED_1
#undef BRAVE_BOOKMARK_CHANGE_PROCESSOR_BOOKMARK_NODE_MOVED_2
#undef BRAVE_BOOKMARK_CHANGE_PROCESSOR_CHILDREN_REORDERED
