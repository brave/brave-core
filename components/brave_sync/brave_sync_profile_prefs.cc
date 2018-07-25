/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "brave/components/brave_sync/brave_sync_profile_prefs.h"

#include "base/debug/stack_trace.h"
#include "brave/components/brave_sync/brave_sync_pref_names.h"
#include "brave/components/brave_sync/brave_sync_settings.h"

#include "chrome/browser/profiles/profile_manager.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_service.h"

namespace brave_sync {

namespace prefs {

void BraveSyncPrefs::RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry) {
  LOG(ERROR) << "TAGAB brave_sync::prefs::RegisterProfilePrefs ";
  //LOG(ERROR) << base::debug::StackTrace().ToString();

  registry->RegisterStringPref(kThisDeviceId, std::string());
  registry->RegisterStringPref(kSeed, std::string());
  registry->RegisterStringPref(kThisDeviceName, std::string());

  registry->RegisterBooleanPref(kSyncThisDeviceEnabled, false);
  registry->RegisterBooleanPref(kSyncBookmarksEnabled, false);
  registry->RegisterBooleanPref(kSiteSettingsEnabled, false);
  registry->RegisterBooleanPref(kHistoryEnabled, false);

  registry->RegisterTimePref(kLatestRecordTime, base::Time());
  registry->RegisterTimePref(kLastFetchTime, base::Time());
}

BraveSyncPrefs::BraveSyncPrefs(PrefService* pref_service) : pref_service_(pref_service) {
  LOG(ERROR) << "TAGAB BraveSyncPrefs::BraveSyncPrefs this="<<this;

  // use ptr from BraveSyncServiceBase::BraveSyncServiceBase, //sync_client->GetPrefService()

  pref_service_ = ProfileManager::GetActiveUserProfile()->GetPrefs();
  LOG(ERROR) << "TAGAB BraveSyncPrefs::BraveSyncPrefs pref_service_="<<pref_service_;
}

std::string BraveSyncPrefs::GetSeed() const {
  LOG(ERROR) << "TAGAB BraveSyncPrefs::GetSeed seed="<<pref_service_->GetString(kSeed);
  return pref_service_->GetString(kSeed);
}

void BraveSyncPrefs::SetSeed(const std::string& seed) {
  LOG(ERROR) << "TAGAB BraveSyncPrefs::SetSeed seed=<"<<seed<<">";
  DCHECK(!seed.empty());
  pref_service_->SetString(kSeed, seed);
}

std::string BraveSyncPrefs::GetThisDeviceId() const {
  return pref_service_->GetString(kThisDeviceId);
}

void BraveSyncPrefs::SetThisDeviceId(const std::string& device_id) {
  LOG(ERROR) << "TAGAB BraveSyncPrefs::SetThisDeviceId device_id=<"<<device_id<<">";
  DCHECK(!device_id.empty());
  pref_service_->SetString(kThisDeviceId, device_id);
}

std::string BraveSyncPrefs::GetThisDeviceName() const {
  return pref_service_->GetString(kThisDeviceName);
}

void BraveSyncPrefs::SetDeviceName(const std::string& device_name) {
  LOG(ERROR) << "TAGAB BraveSyncPrefs::SetDeviceName device_name=<"<<device_name<<">";
  DCHECK(!device_name.empty());
  pref_service_->SetString(kThisDeviceName, device_name);
}

bool BraveSyncPrefs::GetSyncThisDevice() const {
  return pref_service_->GetBoolean(kSyncThisDeviceEnabled);
}

void BraveSyncPrefs::SetSyncThisDevice(const bool &sync_this_device) {
  pref_service_->SetBoolean(kSyncThisDeviceEnabled, sync_this_device);
}

bool BraveSyncPrefs::GetSyncBookmarksEnabled() const {
  return pref_service_->GetBoolean(kSyncBookmarksEnabled);
}

void BraveSyncPrefs::SetSyncBookmarksEnabled(const bool &sync_bookmarks_enabled) {
  pref_service_->SetBoolean(kSyncBookmarksEnabled, sync_bookmarks_enabled);
}

bool BraveSyncPrefs::GetSyncSiteSettingsEnabled() const {
  return pref_service_->GetBoolean(kSiteSettingsEnabled);
}

void BraveSyncPrefs::SetSyncSiteSettingsEnabled(const bool &sync_site_settings_enabled) {
  pref_service_->SetBoolean(kSiteSettingsEnabled, sync_site_settings_enabled);
}

bool BraveSyncPrefs::GetSyncHistoryEnabled() const {
  return pref_service_->GetBoolean(kHistoryEnabled);
}

void BraveSyncPrefs::SetSyncHistoryEnabled(const bool &sync_history_enabled) {
  pref_service_->SetBoolean(kHistoryEnabled, sync_history_enabled);
}

std::unique_ptr<BraveSyncSettings> BraveSyncPrefs::GetBraveSyncSettings() const {
  auto settings = std::make_unique<BraveSyncSettings>();

  settings->this_device_name_ = GetThisDeviceName();
  settings->sync_this_device_ = GetSyncThisDevice();
  settings->sync_bookmarks_ = GetSyncBookmarksEnabled();
  settings->sync_settings_ = GetSyncSiteSettingsEnabled();
  settings->sync_history_ = GetSyncHistoryEnabled();

  return settings;
}

void BraveSyncPrefs::SetLatestRecordTime(const base::Time &time) {
  pref_service_->SetTime(kLatestRecordTime, time);
}

base::Time BraveSyncPrefs::GetLatestRecordTime() {
  return pref_service_->GetTime(kLatestRecordTime);
}

void BraveSyncPrefs::SetLastFetchTime(const base::Time &time) {
  pref_service_->SetTime(kLastFetchTime, time);
}

base::Time BraveSyncPrefs::GetLastFetchTime() {
  return pref_service_->GetTime(kLastFetchTime);
}

void BraveSyncPrefs::Clear() {
  pref_service_->ClearPref(kThisDeviceId);
  pref_service_->ClearPref(kSeed);
  pref_service_->ClearPref(kThisDeviceName);
  pref_service_->ClearPref(kSyncThisDeviceEnabled);
  pref_service_->ClearPref(kSyncBookmarksEnabled);
  pref_service_->ClearPref(kSiteSettingsEnabled);
  pref_service_->ClearPref(kHistoryEnabled);
  pref_service_->ClearPref(kLatestRecordTime);
  pref_service_->ClearPref(kLastFetchTime);
}

} // namespace prefs

} // namespace brave_sync
