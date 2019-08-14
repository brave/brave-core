/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_BOOKMARKS_BOOKMARK_CHANGE_PROCESSOR_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_BOOKMARKS_BOOKMARK_CHANGE_PROCESSOR_H_


#define BRAVE_SYNC_REPOSITION_METHODS \
void MoveSyncNode( \
    int index, \
    const bookmarks::BookmarkNode* node, \
    const syncer::BaseTransaction* trans);

#include "../../../../../components/sync_bookmarks/bookmark_change_processor.h"

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_BOOKMARKS_BOOKMARK_CHANGE_PROCESSOR_H_
