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

class Settings;
class SyncDevices;

namespace prefs {

// String of device id. Supposed to be an integer
extern const char kSyncDeviceId[];
extern const char kSyncDeviceIdV2[];
// String of 32 comma separated bytes
// like "145,58,125,111,85,164,236,38,204,67,40,31,182,114,14,152,242,..."
extern const char kSyncSeed[];
// For storing previous seed after reset. It won't be cleared by Clear()
// Now is deprecated.
extern const char kSyncPrevSeed[];
// String of current device namefor sync
extern const char kSyncDeviceName[];
// The initial bookmarks order, in a format of
// "<1(desktop)|2(mobile)>.<device_id>.">
extern const char kSyncBookmarksBaseOrder[];
// Boolean, whether sync is enabled for the current device
// If true, then sync is enabled and running
// If false, then sync is not enabled or not running (disabled after enabling,
// but seed and device id are configured)
extern const char kSyncEnabled[];
extern const char kSyncBookmarksEnabled[];
extern const char kSyncSiteSettingsEnabled[];
extern const char kSyncHistoryEnabled[];
// The latest time of synced bookmark record, field 'syncTimestamp'
extern const char kSyncLatestRecordTime[];
// The latest time of synced device record
extern const char kSyncLatestDeviceRecordTime[];
// The time of latest fetch records operation
extern const char kSyncLastFetchTime[];
// the list of all known sync devices
// TODO(bridiver) - this should be a dictionary - not raw json
extern const char kSyncDeviceList[];
// the sync api version from the server
extern const char kSyncApiVersion[];
// The version of bookmarks state: 0,1,... .
// Current to migrate to is 2.
extern const char kSyncMigrateBookmarksVersion[];
// Cached object_id list for unconfirmed records
extern const char kSyncRecordsToResend[];
// Meta info of kSyncRecordsToResend
extern const char kSyncRecordsToResendMeta[];
// Flag indicates we had recovered duplicated bookmarks object ids (deprecated)
extern const char kDuplicatedBookmarksRecovered[];

// Version indicates had recovered duplicated bookmarks object ids:
// 2 - we had migrated object ids
extern const char kDuplicatedBookmarksMigrateVersion[];

class Prefs {
 public:
  explicit Prefs(PrefService* pref_service);

  static void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry);

  std::string GetSeed() const;
  void SetSeed(const std::string& seed);
  std::string GetThisDeviceId() const;
  void SetThisDeviceId(const std::string& device_id);
  std::string GetThisDeviceIdV2() const;
  void SetThisDeviceIdV2(const std::string& device_id_v2);
  std::string GetThisDeviceObjectId() const;
  void SetThisDeviceObjectId(const std::string& device_object_id);
  std::string GetThisDeviceName() const;
  void SetThisDeviceName(const std::string& device_name);
  std::string GetBookmarksBaseOrder();
  void SetBookmarksBaseOrder(const std::string& order);

  bool GetSyncEnabled() const;
  void SetSyncEnabled(const bool sync_this_device);
  bool GetSyncBookmarksEnabled() const;
  void SetSyncBookmarksEnabled(const bool sync_bookmarks_enabled);
  bool GetSyncSiteSettingsEnabled() const;
  void SetSyncSiteSettingsEnabled(const bool sync_site_settings);
  bool GetSyncHistoryEnabled() const;
  void SetSyncHistoryEnabled(const bool sync_history_enabled);

  void SetLatestRecordTime(const base::Time &time);
  base::Time GetLatestRecordTime();
  void SetLatestDeviceRecordTime(const base::Time& time);
  base::Time GetLatestDeviceRecordTime();
  void SetLastFetchTime(const base::Time &time);
  base::Time GetLastFetchTime();
  void SetLastCompactTimeBookmarks(const base::Time &time);
  base::Time GetLastCompactTimeBookmarks();

  std::unique_ptr<SyncDevices> GetSyncDevices();
  void SetSyncDevices(const SyncDevices& sync_devices);

  std::string GetApiVersion();
  void SetApiVersion(const std::string& api_version);

  std::unique_ptr<Settings> GetBraveSyncSettings() const;

  int GetMigratedBookmarksVersion();
  void SetMigratedBookmarksVersion(const int);

  std::vector<std::string> GetRecordsToResend() const;
  void AddToRecordsToResend(const std::string& object_id,
                            std::unique_ptr<base::DictionaryValue> meta);
  void RemoveFromRecordsToResend(const std::string& object_id);
  const base::DictionaryValue* GetRecordToResendMeta(
      const std::string& object_id) const;
  void SetRecordToResendMeta(const std::string& object_id,
                             std::unique_ptr<base::DictionaryValue> meta);

  void Clear();

 private:
  // May be null.
  PrefService* pref_service_;

  DISALLOW_COPY_AND_ASSIGN(Prefs);
};

}  // namespace prefs
}  // namespace brave_sync

#endif  // BRAVE_COMPONENTS_BRAVE_SYNC_BRAVE_SYNC_PREFS_H_
