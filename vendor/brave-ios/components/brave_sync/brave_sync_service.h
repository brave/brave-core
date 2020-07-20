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

namespace syncer {
class DeviceInfoSyncService;
class DeviceInfoTracker;
}  // namespace syncer

class BraveSyncService {
 public:
  BraveSyncService(ChromeBrowserState* browser_state);
  ~BraveSyncService();

  bookmarks::BookmarksAPI* bookmarks_api();
  syncer::DeviceInfoSyncService* device_info_service();

 private:
  std::unique_ptr<bookmarks::BookmarksAPI> bookmarks_api_;
  syncer::DeviceInfoSyncService* device_info_sync_service_;

  DISALLOW_COPY_AND_ASSIGN(BraveSyncService);
};

#endif  // BRAVE_VENDOR_BRAVE_IOS_COMPONENTS_BRAVE_SYNC_BRAVE_SYNC_SERVICE_H_
