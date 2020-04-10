/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SYNC_BRAVE_SYNC_PREFS_H_
#define BRAVE_COMPONENTS_BRAVE_SYNC_BRAVE_SYNC_PREFS_H_

#include <memory>
#include <string>
#include <vector>

#include "base/macros.h"
#include "base/values.h"

class PrefService;

namespace base {
class Time;
}

namespace user_prefs {
class PrefRegistrySyncable;
}

void MigrateBraveSyncPrefs(PrefService* prefs);

namespace brave_sync {

namespace prefs {
// Stored as bip39 keywords
extern const char kSyncSeed[];

// Deprecated
// ============================================================================
extern const char kSyncDeviceId[];
extern const char kSyncDeviceIdV2[];
extern const char kSyncDeviceObjectId[];
extern const char kSyncPrevSeed[];
extern const char kSyncDeviceName[];
extern const char kSyncBookmarksBaseOrder[];
extern const char kSyncEnabled[];
extern const char kSyncBookmarksEnabled[];
extern const char kSyncSiteSettingsEnabled[];
extern const char kSyncHistoryEnabled[];
extern const char kSyncLatestRecordTime[];
extern const char kSyncLatestDeviceRecordTime[];
extern const char kSyncLastFetchTime[];
extern const char kSyncDeviceList[];
extern const char kSyncApiVersion[];
extern const char kSyncMigrateBookmarksVersion[];
extern const char kSyncRecordsToResend[];
extern const char kSyncRecordsToResendMeta[];
extern const char kDuplicatedBookmarksRecovered[];
// ============================================================================

// Version indicates had recovered duplicated bookmarks object ids:
// 2 - we had migrated object ids
extern const char kDuplicatedBookmarksMigrateVersion[];

class Prefs {
 public:
  explicit Prefs(PrefService* pref_service);

  static void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry);

  std::string GetSeed() const;
  void SetSeed(const std::string& seed);

  int GetMigratedBookmarksVersion();
  void SetMigratedBookmarksVersion(const int);

  void Clear();

 private:
  // May be null.
  PrefService* pref_service_;

  DISALLOW_COPY_AND_ASSIGN(Prefs);
};

}  // namespace prefs
}  // namespace brave_sync

#endif  // BRAVE_COMPONENTS_BRAVE_SYNC_BRAVE_SYNC_PREFS_H_
