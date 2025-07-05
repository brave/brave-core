/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <algorithm>

#include "components/bookmarks/browser/bookmark_node.h"

// Restores simplified modification date comparison to avoid changing the
// default bookmark save location to "Other bookmarks" (`more_recently_modified`
// is mentioned at the end in order to avoid an unused variable warning)
#define stable_sort(NODES, COMPARATOR)                              \
  stable_sort(NODES, [](const bookmarks::BookmarkNode* n1,          \
                        const bookmarks::BookmarkNode* n2) {        \
    return n1->date_folder_modified() > n2->date_folder_modified(); \
  });                                                               \
  more_recently_modified;

#include "src/components/bookmarks/browser/bookmark_utils.cc"

#undef stable_sort
