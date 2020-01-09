/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SYNC_SYNCER_HELPER_H_
#define BRAVE_COMPONENTS_BRAVE_SYNC_SYNCER_HELPER_H_

#include <map>
#include <string>

namespace bookmarks {
class BookmarkNode;
}  // namespace bookmarks

namespace brave_sync {

// Get index to insert by comparing order or object_id of the node
size_t GetIndex(const bookmarks::BookmarkNode* parent,
                const std::string& order,
                const std::string& object_id);

size_t GetIndex(const bookmarks::BookmarkNode* parent,
                const bookmarks::BookmarkNode* node);

void AddBraveMetaInfo(const bookmarks::BookmarkNode* node);

}  // namespace brave_sync

#endif  // BRAVE_COMPONENTS_BRAVE_SYNC_SYNCER_HELPER_H_
