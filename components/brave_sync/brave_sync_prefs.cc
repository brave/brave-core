/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_sync/brave_sync_prefs.h"

#include "brave/components/brave_sync/settings.h"
#include "brave/components/brave_sync/devices.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_thread.h"

namespace brave_sync {
namespace prefs {

const char kSyncDeviceId[] = "brave_sync.device_id";
const char kSyncSeed[] = "brave_sync.seed";
const char kSyncDeviceName[] = "brave_sync.device_name";
const char kSyncBookmarksBaseOrder[] = "brave_sync.bookmarks_base_order";
const char kSyncEnabled[] = "brave_sync.enabled";
const char kSyncBookmarksEnabled[] = "brave_sync.bookmarks_enabled";
const char kSyncSiteSettingsEnabled[] = "brave_sync.site_settings_enabled";
const char kSyncHistoryEnabled[] = "brave_sync.history_enabled";
const char kSyncLatestRecordTime[] = "brave_sync.latest_record_time";
const char kSyncLastFetchTime[] = "brave_sync.last_fetch_time";
const char kSyncDeviceList[] = "brave_sync.device_list";
const char kSyncApiVersion[] = "brave_sync.api_version";

Prefs::Prefs(Profile* profile) : pref_service_(nullptr) {
  DCHECK(profile);
  pref_service_ = profile->GetPrefs();
}

std::string Prefs::GetSeed() const {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  return pref_service_->GetString(kSyncSeed);
}

void Prefs::SetSeed(const std::string& seed) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DCHECK(!seed.empty());
  pref_service_->SetString(kSyncSeed, seed);
}

std::string Prefs::GetThisDeviceId() const {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  return pref_service_->GetString(kSyncDeviceId);
}

void Prefs::SetThisDeviceId(const std::string& device_id) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DCHECK(!device_id.empty());
  pref_service_->SetString(kSyncDeviceId, device_id);
}

std::string Prefs::GetThisDeviceName() const {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  return pref_service_->GetString(kSyncDeviceName);
}

void Prefs::SetThisDeviceName(const std::string& device_name) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DCHECK(!device_name.empty());
  pref_service_->SetString(kSyncDeviceName, device_name);
}
std::string Prefs::GetBookmarksBaseOrder() {
  return pref_service_->GetString(kSyncBookmarksBaseOrder);
}
void Prefs::SetBookmarksBaseOrder(const std::string& order) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  pref_service_->SetString(kSyncBookmarksBaseOrder, order);
}

bool Prefs::GetSyncEnabled() const {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  return pref_service_->GetBoolean(kSyncEnabled);
}

void Prefs::SetSyncEnabled(const bool sync_this_device) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  pref_service_->SetBoolean(kSyncEnabled, sync_this_device);
}

bool Prefs::GetSyncBookmarksEnabled() const {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  return pref_service_->GetBoolean(kSyncBookmarksEnabled);
}

void Prefs::SetSyncBookmarksEnabled(const bool sync_bookmarks_enabled) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  pref_service_->SetBoolean(kSyncBookmarksEnabled, sync_bookmarks_enabled);
}

bool Prefs::GetSyncSiteSettingsEnabled() const {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  return pref_service_->GetBoolean(kSyncSiteSettingsEnabled);
}

void Prefs::SetSyncSiteSettingsEnabled(const bool sync_site_settings_enabled) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  pref_service_->SetBoolean(kSyncSiteSettingsEnabled, sync_site_settings_enabled);
}

bool Prefs::GetSyncHistoryEnabled() const {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  return pref_service_->GetBoolean(kSyncHistoryEnabled);
}

void Prefs::SetSyncHistoryEnabled(const bool sync_history_enabled) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  pref_service_->SetBoolean(kSyncHistoryEnabled, sync_history_enabled);
}

std::unique_ptr<brave_sync::Settings> Prefs::GetBraveSyncSettings() const {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  auto settings = std::make_unique<brave_sync::Settings>();

  settings->this_device_name_ = GetThisDeviceName();
  settings->sync_this_device_ = GetSyncEnabled();
  settings->sync_bookmarks_ = GetSyncBookmarksEnabled();
  settings->sync_settings_ = GetSyncSiteSettingsEnabled();
  settings->sync_history_ = GetSyncHistoryEnabled();

  settings->sync_configured_ =
      !GetSeed().empty() && !GetThisDeviceName().empty();

  return settings;
}

void Prefs::SetLatestRecordTime(const base::Time &time) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  pref_service_->SetTime(kSyncLatestRecordTime, time);
}

base::Time Prefs::GetLatestRecordTime() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  return pref_service_->GetTime(kSyncLatestRecordTime);
}

void Prefs::SetLastFetchTime(const base::Time &time) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  pref_service_->SetTime(kSyncLastFetchTime, time);
}

base::Time Prefs::GetLastFetchTime() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  return pref_service_->GetTime(kSyncLastFetchTime);
}

std::unique_ptr<SyncDevices> Prefs::GetSyncDevices() {
  auto existing_sync_devices = std::make_unique<SyncDevices>();
  std::string json_device_list = pref_service_->GetString(kSyncDeviceList);
  if (!json_device_list.empty())
    existing_sync_devices->FromJson(json_device_list);

  return existing_sync_devices;
}

void Prefs::SetSyncDevices(const SyncDevices& devices) {
  pref_service_->SetString(kSyncDeviceList, devices.ToJson());
}

std::string Prefs::GetApiVersion() {
  return pref_service_->GetString(kSyncApiVersion);
}

void Prefs::SetApiVersion(const std::string& api_version) {
  pref_service_->SetString(kSyncApiVersion, api_version);
}

void Prefs::Clear() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  pref_service_->ClearPref(kSyncDeviceId);
  pref_service_->ClearPref(kSyncSeed);
  pref_service_->ClearPref(kSyncDeviceName);
  pref_service_->ClearPref(kSyncEnabled);
  pref_service_->ClearPref(kSyncBookmarksEnabled);
  pref_service_->ClearPref(kSyncBookmarksBaseOrder);
  pref_service_->ClearPref(kSyncSiteSettingsEnabled);
  pref_service_->ClearPref(kSyncHistoryEnabled);
  pref_service_->ClearPref(kSyncLatestRecordTime);
  pref_service_->ClearPref(kSyncLastFetchTime);
  pref_service_->ClearPref(kSyncDeviceList);
  pref_service_->ClearPref(kSyncApiVersion);
}

} // namespace prefs
} // namespace brave_sync
