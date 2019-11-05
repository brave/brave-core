/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SYNC_BRAVE_SYNC_PREFS_MEM_STORE_H_
#define BRAVE_COMPONENTS_BRAVE_SYNC_BRAVE_SYNC_PREFS_MEM_STORE_H_

#include <memory>
#include <string>
#include <vector>

#include "base/callback.h"
#include "base/macros.h"
#include "base/values.h"
#include "brave/components/brave_sync/brave_sync_prefs_base.h"

namespace base {
class Time;
}

namespace brave_sync {

class Settings;
class SyncDevices;

namespace prefs {

class PrefsMemStore : public PrefsBase {
 public:
  using NamedChangeCallback = base::RepeatingCallback<void(const std::string&)>;

  PrefsMemStore();
  virtual ~PrefsMemStore();

  void AddObserver(const NamedChangeCallback& obs);
  void ResetObserver();

  std::string GetSeed() const override;
  void SetSeed(const std::string& seed) override;
  std::string GetPrevSeed() const override;
  void SetPrevSeed(const std::string& seed) override;
  std::string GetThisDeviceId() const override;
  void SetThisDeviceId(const std::string& device_id) override;
  std::string GetThisDeviceName() const override;
  void SetThisDeviceName(const std::string& device_name) override;
  std::string GetBookmarksBaseOrder() override;
  void SetBookmarksBaseOrder(const std::string& order) override;

  bool GetSyncEnabled() const override;
  void SetSyncEnabled(const bool sync_this_device) override;
  bool GetSyncBookmarksEnabled() const override;
  void SetSyncBookmarksEnabled(const bool sync_bookmarks_enabled) override;
  bool GetSyncSiteSettingsEnabled() const override;
  void SetSyncSiteSettingsEnabled(const bool sync_site_settings) override;
  bool GetSyncHistoryEnabled() const override;
  void SetSyncHistoryEnabled(const bool sync_history_enabled) override;

  void SetLatestRecordTime(const base::Time& time) override;
  base::Time GetLatestRecordTime() override;
  void SetLatestDeviceRecordTime(const base::Time& time) override;
  base::Time GetLatestDeviceRecordTime() override;
  void SetLastFetchTime(const base::Time& time) override;
  base::Time GetLastFetchTime() override;

  std::unique_ptr<SyncDevices> GetSyncDevices() override;
  void SetSyncDevices(const SyncDevices& sync_devices) override;

  std::string GetApiVersion() override;
  void SetApiVersion(const std::string& api_version) override;

  std::unique_ptr<Settings> GetBraveSyncSettings() const override;

  int GetMigratedBookmarksVersion() override;
  void SetMigratedBookmarksVersion(const int) override;

  std::vector<std::string> GetRecordsToResend() const override;
  void AddToRecordsToResend(
      const std::string& object_id,
      std::unique_ptr<base::DictionaryValue> meta) override;
  void RemoveFromRecordsToResend(const std::string& object_id) override;
  const base::DictionaryValue* GetRecordToResendMeta(
      const std::string& object_id) const override;
  void SetRecordToResendMeta(
      const std::string& object_id,
      std::unique_ptr<base::DictionaryValue> meta) override;

  void Clear() override;

 private:
  std::string seed_;
  std::string prev_seed_;
  std::string this_device_id_;
  std::string this_device_name_;
  bool sync_enabled_ = false;
  std::string json_device_list_;

  bool bookmarks_enabled_ = false;
  bool site_settings_enabled_ = false;
  bool history_enabled_ = false;

  int migrate_bookmarks_version_ = 0;
  std::string api_version_;
  std::string bookmark_base_order_;

  base::Time last_fetch_time_;
  // Bookmarks are always enabled on start of sync
  base::Time latest_record_time_;
  // We are using latest device record until chain is not fully created
  base::Time latest_device_record_time_;

  NamedChangeCallback obs_;
  void FireCallback(const std::string& name);

  PrefsMemStore(const PrefsMemStore&) = delete;
  PrefsMemStore& operator=(const PrefsMemStore&) = delete;
};

void CloneMemPrefsToDisk(PrefsMemStore* prefs_mem, PrefsBase* prefs_disk);

}  // namespace prefs
}  // namespace brave_sync

#endif  // BRAVE_COMPONENTS_BRAVE_SYNC_BRAVE_SYNC_PREFS_MEM_STORE_H_
