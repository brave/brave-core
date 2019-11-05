/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_sync/brave_sync_prefs_mem_store.h"

#include <utility>

#include "brave/components/brave_sync/brave_sync_prefs.h"
#include "brave/components/brave_sync/settings.h"
#include "brave/components/brave_sync/sync_devices.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"

namespace brave_sync {
namespace prefs {

PrefsMemStore::PrefsMemStore() {}

PrefsMemStore::~PrefsMemStore() {
  ResetObserver();
}

void PrefsMemStore::AddObserver(const NamedChangeCallback& obs) {
  obs_ = obs;
}

void PrefsMemStore::ResetObserver() {
  obs_.Reset();
}

std::string PrefsMemStore::GetSeed() const {
  return seed_;
}

void PrefsMemStore::SetSeed(const std::string& seed) {
  DCHECK(!seed.empty());
  seed_ = seed;
}

std::string PrefsMemStore::GetPrevSeed() const {
  return prev_seed_;
}

void PrefsMemStore::SetPrevSeed(const std::string& seed) {
  // This may be invoked by tests
  prev_seed_ = seed;
}

std::string PrefsMemStore::GetThisDeviceId() const {
  return this_device_id_;
}

void PrefsMemStore::SetThisDeviceId(const std::string& device_id) {
  DCHECK(!device_id.empty());
  this_device_id_ = device_id;
}

std::string PrefsMemStore::GetThisDeviceName() const {
  return this_device_name_;
}

void PrefsMemStore::SetThisDeviceName(const std::string& device_name) {
  DCHECK(!device_name.empty());
  bool should_fire_callback = (this_device_name_ != device_name);
  this_device_name_ = device_name;
  if (should_fire_callback)
    FireCallback(prefs::kSyncDeviceName);
}

std::string PrefsMemStore::GetBookmarksBaseOrder() {
  return bookmark_base_order_;
}
void PrefsMemStore::SetBookmarksBaseOrder(const std::string& order) {
  bookmark_base_order_ = order;
}

bool PrefsMemStore::GetSyncEnabled() const {
  return sync_enabled_;
}

void PrefsMemStore::SetSyncEnabled(const bool sync_this_device) {
  bool should_fire_callback = (sync_enabled_ != sync_this_device);
  sync_enabled_ = sync_this_device;
  if (should_fire_callback)
    FireCallback(prefs::kSyncEnabled);
}

bool PrefsMemStore::GetSyncBookmarksEnabled() const {
  return bookmarks_enabled_;
}

void PrefsMemStore::SetSyncBookmarksEnabled(const bool sync_bookmarks_enabled) {
  bool should_fire_callback = (bookmarks_enabled_ != sync_bookmarks_enabled);
  bookmarks_enabled_ = sync_bookmarks_enabled;
  if (should_fire_callback)
    FireCallback(prefs::kSyncBookmarksEnabled);
}

bool PrefsMemStore::GetSyncSiteSettingsEnabled() const {
  return site_settings_enabled_;
}

void PrefsMemStore::SetSyncSiteSettingsEnabled(
    const bool sync_site_settings_enabled) {
  bool should_fire_callback =
      (site_settings_enabled_ != sync_site_settings_enabled);
  site_settings_enabled_ = sync_site_settings_enabled;
  if (should_fire_callback)
    FireCallback(prefs::kSyncSiteSettingsEnabled);
}

bool PrefsMemStore::GetSyncHistoryEnabled() const {
  return history_enabled_;
}

void PrefsMemStore::SetSyncHistoryEnabled(const bool sync_history_enabled) {
  bool should_fire_callback = (history_enabled_ != sync_history_enabled);
  history_enabled_ = sync_history_enabled;
  if (should_fire_callback)
    FireCallback(prefs::kSyncHistoryEnabled);
}

std::unique_ptr<brave_sync::Settings> PrefsMemStore::GetBraveSyncSettings()
    const {
  auto settings = std::make_unique<brave_sync::Settings>();

  settings->this_device_name_ = GetThisDeviceName();
  settings->this_device_id_ = GetThisDeviceId();
  settings->sync_this_device_ = GetSyncEnabled();
  settings->sync_bookmarks_ = GetSyncBookmarksEnabled();
  settings->sync_settings_ = GetSyncSiteSettingsEnabled();
  settings->sync_history_ = GetSyncHistoryEnabled();

  settings->sync_configured_ =
      !GetSeed().empty() && !GetThisDeviceName().empty();

  return settings;
}

void PrefsMemStore::SetLatestRecordTime(const base::Time& time) {
  latest_record_time_ = time;
}

base::Time PrefsMemStore::GetLatestRecordTime() {
  return latest_record_time_;
}

void PrefsMemStore::SetLatestDeviceRecordTime(const base::Time& time) {
  latest_device_record_time_ = time;
}

base::Time PrefsMemStore::GetLatestDeviceRecordTime() {
  return latest_device_record_time_;
}

void PrefsMemStore::SetLastFetchTime(const base::Time& time) {
  last_fetch_time_ = time;
}

base::Time PrefsMemStore::GetLastFetchTime() {
  return last_fetch_time_;
}

std::unique_ptr<SyncDevices> PrefsMemStore::GetSyncDevices() {
  auto existing_sync_devices = std::make_unique<SyncDevices>();
  if (!json_device_list_.empty())
    existing_sync_devices->FromJson(json_device_list_);

  return existing_sync_devices;
}

void PrefsMemStore::SetSyncDevices(const SyncDevices& devices) {
  const std::string json_device_list_to_set = devices.ToJson();
  bool should_fire_callback = (json_device_list_ != json_device_list_to_set);
  json_device_list_ = json_device_list_to_set;
  if (should_fire_callback)
    FireCallback(prefs::kSyncDeviceList);
}

std::string PrefsMemStore::GetApiVersion() {
  return api_version_;
}

void PrefsMemStore::SetApiVersion(const std::string& api_version) {
  api_version_ = api_version;
}

int PrefsMemStore::GetMigratedBookmarksVersion() {
  return migrate_bookmarks_version_;
}

void PrefsMemStore::SetMigratedBookmarksVersion(const int migrate_bookmarks) {
  migrate_bookmarks_version_ = migrate_bookmarks;
}

std::vector<std::string> PrefsMemStore::GetRecordsToResend() const {
  // Until sync chain is fully created we never have records to resend
  return std::vector<std::string>();
}

void PrefsMemStore::AddToRecordsToResend(
    const std::string& object_id,
    std::unique_ptr<base::DictionaryValue> meta) {
  NOTREACHED();
}

void PrefsMemStore::RemoveFromRecordsToResend(const std::string& object_id) {
  NOTREACHED();
}

const base::DictionaryValue* PrefsMemStore::GetRecordToResendMeta(
    const std::string& object_id) const {
  NOTREACHED();
  return nullptr;
}

void PrefsMemStore::SetRecordToResendMeta(
    const std::string& object_id,
    std::unique_ptr<base::DictionaryValue> meta) {
  NOTREACHED();
}

void PrefsMemStore::Clear() {
  seed_.clear();
  this_device_id_.clear();
  this_device_name_.clear();
  sync_enabled_ = false;
  json_device_list_.clear();
  bookmarks_enabled_ = false;
  site_settings_enabled_ = false;
  history_enabled_ = false;

  migrate_bookmarks_version_ = 0;
  api_version_.clear();
}

void PrefsMemStore::FireCallback(const std::string& name) {
  if (obs_) {
    obs_.Run(name);
  }
}

void CloneMemPrefsToDisk(PrefsMemStore* prefs_mem, PrefsBase* prefs_disk) {
  prefs_disk->SetSeed(prefs_mem->GetSeed());
  prefs_disk->SetPrevSeed(prefs_mem->GetPrevSeed());
  prefs_disk->SetThisDeviceId(prefs_mem->GetThisDeviceId());
  prefs_disk->SetThisDeviceName(prefs_mem->GetThisDeviceName());
  prefs_disk->SetSyncEnabled(prefs_mem->GetSyncEnabled());
  prefs_disk->SetSyncDevices(*(prefs_mem->GetSyncDevices()));

  prefs_disk->SetMigratedBookmarksVersion(
      prefs_mem->GetMigratedBookmarksVersion());
  prefs_disk->SetApiVersion(prefs_mem->GetApiVersion());
  prefs_disk->SetBookmarksBaseOrder(prefs_mem->GetBookmarksBaseOrder());

  prefs_disk->SetLastFetchTime(prefs_mem->GetLastFetchTime());

  prefs_disk->SetLatestDeviceRecordTime(prefs_mem->GetLatestDeviceRecordTime());
  prefs_disk->SetLatestDeviceRecordTime(prefs_mem->GetLatestDeviceRecordTime());

  prefs_disk->SetSyncBookmarksEnabled(prefs_mem->GetSyncBookmarksEnabled());
  prefs_disk->SetSyncSiteSettingsEnabled(
      prefs_mem->GetSyncSiteSettingsEnabled());
  prefs_disk->SetSyncHistoryEnabled(prefs_mem->GetSyncHistoryEnabled());
}

}  // namespace prefs
}  // namespace brave_sync
