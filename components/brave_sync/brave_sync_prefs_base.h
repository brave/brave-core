/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SYNC_BRAVE_SYNC_PREFS_BASE_H_
#define BRAVE_COMPONENTS_BRAVE_SYNC_BRAVE_SYNC_PREFS_BASE_H_

#include <memory>
#include <string>
#include <vector>

#include "base/macros.h"
#include "base/values.h"

namespace base {
class Time;
}

namespace user_prefs {
class PrefRegistrySyncable;
}

namespace brave_sync {

class Settings;
class SyncDevices;

namespace prefs {

class PrefsBase {
 public:
  virtual std::string GetSeed() const = 0;
  virtual void SetSeed(const std::string& seed) = 0;
  virtual std::string GetPrevSeed() const = 0;
  virtual void SetPrevSeed(const std::string& seed) = 0;
  virtual std::string GetThisDeviceId() const = 0;
  virtual void SetThisDeviceId(const std::string& device_id) = 0;
  virtual std::string GetThisDeviceName() const = 0;
  virtual void SetThisDeviceName(const std::string& device_name) = 0;
  virtual std::string GetBookmarksBaseOrder() = 0;
  virtual void SetBookmarksBaseOrder(const std::string& order) = 0;

  virtual bool GetSyncEnabled() const = 0;
  virtual void SetSyncEnabled(const bool sync_this_device) = 0;
  virtual bool GetSyncBookmarksEnabled() const = 0;
  virtual void SetSyncBookmarksEnabled(const bool sync_bookmarks_enabled) = 0;
  virtual bool GetSyncSiteSettingsEnabled() const = 0;
  virtual void SetSyncSiteSettingsEnabled(const bool sync_site_settings) = 0;
  virtual bool GetSyncHistoryEnabled() const = 0;
  virtual void SetSyncHistoryEnabled(const bool sync_history_enabled) = 0;

  virtual void SetLatestRecordTime(const base::Time& time) = 0;
  virtual base::Time GetLatestRecordTime() = 0;
  virtual void SetLatestDeviceRecordTime(const base::Time& time) = 0;
  virtual base::Time GetLatestDeviceRecordTime() = 0;
  virtual void SetLastFetchTime(const base::Time& time) = 0;
  virtual base::Time GetLastFetchTime() = 0;

  virtual std::unique_ptr<SyncDevices> GetSyncDevices() = 0;
  virtual void SetSyncDevices(const SyncDevices& sync_devices) = 0;

  virtual std::string GetApiVersion() = 0;
  virtual void SetApiVersion(const std::string& api_version) = 0;

  virtual std::unique_ptr<Settings> GetBraveSyncSettings() const = 0;

  virtual int GetMigratedBookmarksVersion() = 0;
  virtual void SetMigratedBookmarksVersion(const int) = 0;

  virtual std::vector<std::string> GetRecordsToResend() const = 0;
  virtual void AddToRecordsToResend(
      const std::string& object_id,
      std::unique_ptr<base::DictionaryValue> meta) = 0;
  virtual void RemoveFromRecordsToResend(const std::string& object_id) = 0;
  virtual const base::DictionaryValue* GetRecordToResendMeta(
      const std::string& object_id) const = 0;
  virtual void SetRecordToResendMeta(
      const std::string& object_id,
      std::unique_ptr<base::DictionaryValue> meta) = 0;

  virtual void Clear() = 0;
};

}  // namespace prefs
}  // namespace brave_sync

#endif  // BRAVE_COMPONENTS_BRAVE_SYNC_BRAVE_SYNC_PREFS_BASE_H_
