/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _BRAVE_COMPONENTS_BRAVE_SYNC_DEBUG_H_
#define _BRAVE_COMPONENTS_BRAVE_SYNC_DEBUG_H_

#include <string>
// cannot do forward declaration of nested class
#include "components/bookmarks/browser/bookmark_node.h"

std::string GetThreadInfoString();
const char *GetBookmarkNodeString(bookmarks::BookmarkNode::Type type);
bool ValidateBookmarksBaseOrder(const std::string &base_order);

#endif // _BRAVE_COMPONENTS_BRAVE_SYNC_DEBUG_H_
