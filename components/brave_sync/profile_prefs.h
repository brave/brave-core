/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef BRAVE_COMPONENT_BRAVE_SYNC_PROFILE_PREFS_H_
#define BRAVE_COMPONENT_BRAVE_SYNC_PROFILE_PREFS_H_

#include <string>

#include "base/macros.h"

class PrefService;
class Profile;

namespace base {
  class Time;
}

namespace user_prefs {
class PrefRegistrySyncable;
}

namespace brave_sync {

class Settings;

namespace prefs {

//TODO, AB: subclass SyncPrefs components/sync/base/sync_prefs.h ?
class Prefs {
public:
  static void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry);

  Prefs(Profile *profile);

  std::string GetSeed() const;
  void SetSeed(const std::string& seed);
  std::string GetThisDeviceId() const;
  void SetThisDeviceId(const std::string& device_id);
  std::string GetThisDeviceName() const;
  void SetDeviceName(const std::string& device_name);

  bool GetSyncThisDevice() const;
  void SetSyncThisDevice(const bool &sync_this_device);
  bool GetSyncBookmarksEnabled() const;
  void SetSyncBookmarksEnabled(const bool &sync_bookmarks_enabled);
  bool GetSyncSiteSettingsEnabled() const;
  void SetSyncSiteSettingsEnabled(const bool &sync_site_settings);
  bool GetSyncHistoryEnabled() const;
  void SetSyncHistoryEnabled(const bool &sync_history_enabled);

  // uint64_t GetTimeLastFetch() const;
  // void SetTimeLastFetch(const uint64_t &time_last_fetch);

  void SetLatestRecordTime(const base::Time &time);
  base::Time GetLatestRecordTime();
  void SetLastFetchTime(const base::Time &time);
  base::Time GetLastFetchTime();

  std::unique_ptr<brave_sync::Settings> GetBraveSyncSettings() const;

  void Clear();

private:
  // May be null.
  PrefService* pref_service_;

  DISALLOW_COPY_AND_ASSIGN(Prefs);
};

} // namespace prefs

} // namespace brave_sync

#endif //BRAVE_COMPONENT_BRAVE_SYNC_PROFILE_PREFS_H_
