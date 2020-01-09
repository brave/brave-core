/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/sync_bookmarks/bookmark_specifics_conversions.h"

#include "brave/components/brave_sync/syncer_helper.h"
#define CreateSpecificsFromBookmarkNode \
  CreateSpecificsFromBookmarkNodeChromiumImpl
#include "../../../../components/sync_bookmarks/bookmark_specifics_conversions.cc"
#undef CreateSpecificsFromBookmarkNode

namespace sync_bookmarks {

sync_pb::EntitySpecifics CreateSpecificsFromBookmarkNode(
    const bookmarks::BookmarkNode* node,
    bookmarks::BookmarkModel* model,
    bool force_favicon_load) {
  brave_sync::AddBraveMetaInfo(node);
  return CreateSpecificsFromBookmarkNodeChromiumImpl(node, model,
                                                     force_favicon_load);
}

}  // namespace sync_bookmarks
