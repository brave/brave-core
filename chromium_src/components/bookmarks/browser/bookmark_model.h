/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_BOOKMARKS_BROWSER_BOOKMARK_MODEL_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_BOOKMARKS_BROWSER_BOOKMARK_MODEL_H_

class BraveSyncServiceTestDelayedLoadModel;

#define BRAVE_BOOKMARK_MODEL_H \
 private: \
  friend class ::BraveSyncServiceTestDelayedLoadModel;

#include "src/components/bookmarks/browser/bookmark_model.h"

namespace bookmarks {
void BraveMigrateOtherNodeFolder(BookmarkModel* model);
void BraveClearSyncV1MetaInfo(BookmarkModel* model);
}  // namespace bookmarks

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_BOOKMARKS_BROWSER_BOOKMARK_MODEL_H_
