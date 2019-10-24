/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/bookmarks/browser/bookmark_utils.h"
namespace bookmarks {
// DeleteBookmarkFolders won't get a chance to delete other_node(), even with
// malicious usage deleting bookmark_bar_node() and other_node() are both
// prohibited so no need to redirect other_node() to bookmark_bar_node()
const BookmarkNode* GetBookmarkNodeByID_ChromiumImpl(const BookmarkModel* model,
                                                     int64_t id);
}  // namespace bookmarks
#define GetBookmarkNodeByID GetBookmarkNodeByID_ChromiumImpl
#include "../../../../../components/bookmarks/browser/bookmark_utils.cc"
#undef GetBookmarkNodeByID
namespace bookmarks {

const BookmarkNode* GetBookmarkNodeByID(const BookmarkModel* model,
                                        int64_t id) {
  if (id == model->other_node()->id())
    id = model->bookmark_bar_node()->id();
  return GetBookmarkNodeByID_ChromiumImpl(model, id);
}

}  // namespace bookmarks
