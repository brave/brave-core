/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 3.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BRAVE_IOS_COMPONENTS_BRAVE_SYNC_BRAVE_SYNC_SERVICE_H_
#define BRAVE_VENDOR_BRAVE_IOS_COMPONENTS_BRAVE_SYNC_BRAVE_SYNC_SERVICE_H_

#include <memory>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/sequenced_task_runner.h"
#include "base/supports_user_data.h"
#include "brave/vendor/brave-ios/components/bookmarks/bookmarks_api.h"

//namespace bookmarks {
//class BookmarksAPI;
//}

namespace sync_preferences {
class PrefServiceSyncable;
class PrefServiceSyncableFactory;
}

namespace user_prefs {
class PrefRegistrySyncable;
}

class BraveSyncService : public base::SupportsUserData {
 public:
  BraveSyncService();
  ~BraveSyncService();

  bookmarks::BookmarksAPI* bookmarks_api();

 private:
  scoped_refptr<user_prefs::PrefRegistrySyncable> pref_registry_;
  scoped_refptr<base::SequencedTaskRunner> io_task_runner_;
  std::unique_ptr<sync_preferences::PrefServiceSyncable> prefs_;
  std::unique_ptr<bookmarks::BookmarksAPI> bookmarks_api_;

  DISALLOW_COPY_AND_ASSIGN(BraveSyncService);
};

#endif  // BRAVE_VENDOR_BRAVE_IOS_COMPONENTS_BRAVE_SYNC_BRAVE_SYNC_SERVICE_H_
