/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_BOOKMARKS_EXPORTER_BOOKMARKS_ENCODER_H_
#define BRAVE_IOS_BROWSER_API_BOOKMARKS_EXPORTER_BOOKMARKS_ENCODER_H_

#include <memory>
#include <vector>

#include "base/values.h"

namespace bookmarks {
class BookmarkNode;
}  // namespace bookmarks

namespace ios {
namespace bookmarks_encoder {
base::Value::Dict Encode(const bookmarks::BookmarkNode* bookmark_bar_node,
                         const bookmarks::BookmarkNode* other_folder_node,
                         const bookmarks::BookmarkNode* mobile_folder_node);
}  // namespace bookmarks_encoder
}  // namespace ios

#endif  // BRAVE_IOS_BROWSER_API_BOOKMARKS_EXPORTER_BOOKMARKS_ENCODER_H_
