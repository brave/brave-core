/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SYNC_SYNCER_HELPER_H_
#define BRAVE_COMPONENTS_BRAVE_SYNC_SYNCER_HELPER_H_

#include <string>

namespace bookmarks {
class BookmarkModel;
class BookmarkNode;
}   // namespace bookmarks

namespace brave_sync {

void AddBraveMetaInfo(const bookmarks::BookmarkNode* node,
                      bookmarks::BookmarkModel* bookmark_model,
                      bool has_new_parent);


// |src| is the node which is about to be inserted into |parent|
uint64_t GetIndex(const bookmarks::BookmarkNode* parent,
                  const bookmarks::BookmarkNode* src);

}   // namespace brave_sync

#endif  // BRAVE_COMPONENTS_BRAVE_SYNC_SYNCER_HELPER_H_
