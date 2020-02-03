/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/sync_bookmarks/bookmark_model_observer_impl.h"

#include <utility>

namespace sync_bookmarks {

class BraveBookmarkModelObserverImpl : public BookmarkModelObserverImpl {
 public:
  // |bookmark_tracker_| must not be null and must outlive this object.
  BraveBookmarkModelObserverImpl(
      const base::RepeatingClosure& nudge_for_commit_closure,
      base::OnceClosure on_bookmark_model_being_deleted_closure,
      SyncedBookmarkTracker* bookmark_tracker)
      : BookmarkModelObserverImpl(
            nudge_for_commit_closure,
            std::move(on_bookmark_model_being_deleted_closure),
            bookmark_tracker) {}
  ~BraveBookmarkModelObserverImpl() override = default;

  // BookmarkModelObserver:
  void BookmarkMetaInfoChanged(bookmarks::BookmarkModel* model,
                               const bookmarks::BookmarkNode* node) override {}
  void BookmarkNodeFaviconChanged(
      bookmarks::BookmarkModel* model,
      const bookmarks::BookmarkNode* node) override {}

 private:
  DISALLOW_COPY_AND_ASSIGN(BraveBookmarkModelObserverImpl);
};

}  // namespace sync_bookmarks

#define BookmarkModelObserverImpl BraveBookmarkModelObserverImpl
#include "../../../../components/sync_bookmarks/bookmark_model_type_processor.cc"
#undef BookmarkModelObserverImpl
