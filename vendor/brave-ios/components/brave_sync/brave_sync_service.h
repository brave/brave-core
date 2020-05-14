/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 3.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BRAVE_IOS_COMPONENTS_BRAVE_SYNC_BRAVE_SYNC_SERVICE_H_
#define BRAVE_VENDOR_BRAVE_IOS_COMPONENTS_BRAVE_SYNC_BRAVE_SYNC_SERVICE_H_

#include <memory>

#include "base/compiler_specific.h"
#include "base/macros.h"

class ChromeBrowserState;

namespace bookmarks {
class BookmarksAPI;
}

class BraveSyncService {
 public:
  BraveSyncService(ChromeBrowserState* browser_state);
  ~BraveSyncService();

  bookmarks::BookmarksAPI* bookmarks_api();

 private:
  std::unique_ptr<bookmarks::BookmarksAPI> bookmarks_api_;

  DISALLOW_COPY_AND_ASSIGN(BraveSyncService);
};

#endif  // BRAVE_VENDOR_BRAVE_IOS_COMPONENTS_BRAVE_SYNC_BRAVE_SYNC_SERVICE_H_
