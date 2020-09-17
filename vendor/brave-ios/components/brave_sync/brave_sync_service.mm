/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 3.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/vendor/brave-ios/components/brave_sync/brave_sync_service.h"
#include "brave/vendor/brave-ios/components/bookmarks/bookmarks_api.h"
#include "brave/vendor/brave-ios/components/browser_state/brave_browser_state.h"
#include "brave/vendor/brave-ios/components/browser_state/brave_browser_state_manager.h"
#include "brave/vendor/brave-ios/components/user_prefs/user_prefs.h"
#include "brave/vendor/brave-ios/components/context/application_context.h"
#include "brave/vendor/brave-ios/components/bookmarks/bookmark_client.h"

#include "base/task/post_task.h"
#include "base/base_paths.h"
#include "base/files/file_util.h"
#include "base/strings/sys_string_conversions.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/json_pref_store.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/bookmarks/browser/bookmark_utils.h"
#include "components/sync_preferences/pref_service_syncable.h"
#include "components/sync_preferences/pref_service_syncable_factory.h"
#include "../bookmarks/bookmarks_api.h"
#include "base/mac/foundation_util.h"

namespace chrome {
    enum class Channel {
      UNKNOWN = 0,
      DEFAULT = UNKNOWN,
      CANARY = 1,
      DEV = 2,
      BETA = 3,
      STABLE = 4,
    };

    Channel GetChannel() {
        return Channel::STABLE;
    }
}

namespace {

const char kPreferencesFilename[] = "Preferences";

bool GetUserDataDir(base::FilePath* result) {
  bool success = base::mac::GetUserDirectory(NSSearchPathDirectory::NSApplicationSupportDirectory,
    result);
  
//  bool success = base::PathService::Get(ios::DIR_USER_DATA, &result);
//  DCHECK(result);

  // On IOS, this directory does not exist unless it is created explicitly.
  if (success && !base::PathExists(*result))
    success = base::CreateDirectory(*result);

  if (!success)
    return false;

  base::FilePath path = result->Append(FILE_PATH_LITERAL("BraveBrowser")); // TODO(bridiver) - dev, etc..

  if (!base::PathExists(path))
    success = base::CreateDirectory(path);

  return success;
}

}  // namespace

BraveSyncService::BraveSyncService()
    : pref_registry_(base::MakeRefCounted<user_prefs::PrefRegistrySyncable>()) {
  bookmarks::RegisterProfilePrefs(pref_registry_.get());

  io_task_runner_ =
      base::CreateSequencedTaskRunner(
          {base::ThreadPool(), base::TaskShutdownBehavior::BLOCK_SHUTDOWN,
           base::MayBlock()});

  base::FilePath browser_state_path;
  if (!GetUserDataDir(&browser_state_path))
    NOTREACHED();

  sync_preferences::PrefServiceSyncableFactory factory;
  factory.set_user_prefs(base::MakeRefCounted<JsonPrefStore>(
      browser_state_path.Append(kPreferencesFilename),
      std::unique_ptr<PrefFilter>(), io_task_runner_.get()));

  prefs_ =
      factory.CreateSyncable(pref_registry_.get());

  // Register on BrowserState.
  user_prefs::UserPrefs::Set(this, prefs_.get());

  brave::ChromeBrowserState* browser_state = brave::BraveBrowserState::FromBrowserState(brave::GetApplicationContext()->GetChromeBrowserStateManager()->GetLastUsedBrowserState());
        (void)browser_state;
//  bookmarks_api_ = std::make_unique<bookmarks::BookmarksAPI>(prefs_.get(),
//      browser_state_path,
//      io_task_runner_.get(),
//      std::make_unique<brave::BraveBookmarkClient>(browser_state, this));
}

BraveSyncService::~BraveSyncService() {}

bookmarks::BookmarksAPI* BraveSyncService::bookmarks_api() {
    return bookmarks_api_.get();
}
