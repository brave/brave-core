/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// Early return when the tracker hasn't been fully initialized yet
// (BookmarkModelLoaded not received). This can happen during re-entrant
// bookmark operations when a BookmarkModelLoadWaiter callback adds bookmarks
// during DoneLoading observer iteration, before this tracker's
// BookmarkModelLoaded is invoked.
#define BRAVE_PERMANENT_FOLDER_ORDERING_TRACKER_GET_INDEX_ACROSS_STORAGE \
  if (!local_or_syncable_node_) {                                        \
    return in_storage_index;                                             \
  }

#include <chrome/browser/bookmarks/permanent_folder_ordering_tracker.cc>

#undef BRAVE_PERMANENT_FOLDER_ORDERING_TRACKER_GET_INDEX_ACROSS_STORAGE
