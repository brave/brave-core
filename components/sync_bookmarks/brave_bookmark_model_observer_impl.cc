/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/sync_bookmarks/brave_bookmark_model_observer_impl.h"

#include <utility>

namespace sync_bookmarks {

BraveBookmarkModelObserverImpl::BraveBookmarkModelObserverImpl(
    const base::RepeatingClosure& nudge_for_commit_closure,
    base::OnceClosure on_bookmark_model_being_deleted_closure,
    SyncedBookmarkTracker* bookmark_tracker)
    : BookmarkModelObserverImpl(
        nudge_for_commit_closure,
        std::move(on_bookmark_model_being_deleted_closure),
        bookmark_tracker) {}

BraveBookmarkModelObserverImpl::~BraveBookmarkModelObserverImpl() = default;

void BraveBookmarkModelObserverImpl::BookmarkMetaInfoChanged(
    bookmarks::BookmarkModel* model,
    const bookmarks::BookmarkNode* node) {}

void BraveBookmarkModelObserverImpl::BookmarkNodeFaviconChanged(
    bookmarks::BookmarkModel* model,
    const bookmarks::BookmarkNode* node) {}

}  // namespace sync_bookmarks
