/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SYNC_TOOLS_H_
#define BRAVE_COMPONENTS_BRAVE_SYNC_TOOLS_H_

#include <string>

namespace base {
class Time;
}  // namespace base

namespace bookmarks {
class BookmarkNode;
}

namespace brave_sync {

namespace tools {

extern const char kOtherNodeOrder[];

std::string GenerateObjectId();

// If old_id is empty, it would use default seed as first iteration, in future
// iteration, caller has to provide previous used id so we can have determined
// generated object id
std::string GenerateObjectIdForOtherNode(const std::string old_id);

std::string GetPlatformName();

bool IsTimeEmpty(const base::Time &time);

bookmarks::BookmarkNode* AsMutable(const bookmarks::BookmarkNode* node);

std::string GetOtherNodeName();

}  // namespace tools

}  // namespace brave_sync

#endif  // BRAVE_COMPONENTS_BRAVE_SYNC_TOOLS_H_
