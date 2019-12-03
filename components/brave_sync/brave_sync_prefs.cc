/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_sync/brave_sync_prefs.h"

#include <utility>

#include "brave/components/brave_sync/settings.h"
#include "brave/components/brave_sync/sync_devices.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"

void MigrateBraveSyncPrefs(PrefService* prefs) {
  prefs->ClearPref(brave_sync::prefs::kSyncPrevSeed);
}

namespace brave_sync {
namespace prefs {

const char kSyncDeviceId[] = "brave_sync.device_id";
const char kSyncDeviceIdV2[] = "brave_sync.device_id_v2";
const char kSyncSeed[] = "brave_sync.seed";
const char kSyncPrevSeed[] = "brave_sync.previous_seed";
const char kSyncDeviceName[] = "brave_sync.device_name";
const char kSyncBookmarksBaseOrder[] = "brave_sync.bookmarks_base_order";
const char kSyncEnabled[] = "brave_sync.enabled";
const char kSyncBookmarksEnabled[] = "brave_sync.bookmarks_enabled";
const char kSyncSiteSettingsEnabled[] = "brave_sync.site_settings_enabled";
const char kSyncHistoryEnabled[] = "brave_sync.history_enabled";
const char kSyncLatestRecordTime[] = "brave_sync.latest_record_time";
const char kSyncLatestDeviceRecordTime[] =
    "brave_sync.latest_device_record_time";
const char kSyncLastFetchTime[] = "brave_sync.last_fetch_time";
const char kSyncLastCompactTimeBookmarks[] =
    "brave_sync.last_compact_time.bookmarks";
const char kSyncDeviceList[] = "brave_sync.device_list";
const char kSyncApiVersion[] = "brave_sync.api_version";
const char kSyncMigrateBookmarksVersion[]
                                       = "brave_sync.migrate_bookmarks_version";
const char kSyncRecordsToResend[] = "brave_sync_records_to_resend";
const char kSyncRecordsToResendMeta[] = "brave_sync_records_to_resend_meta";

Prefs::Prefs(PrefService* pref_service) : pref_service_(pref_service) {}

void Prefs::RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterStringPref(prefs::kSyncDeviceId, std::string());
  registry->RegisterStringPref(prefs::kSyncDeviceIdV2, std::string());
  registry->RegisterStringPref(prefs::kSyncSeed, std::string());
  registry->RegisterStringPref(prefs::kSyncPrevSeed, std::string());
  registry->RegisterStringPref(prefs::kSyncDeviceName, std::string());
  registry->RegisterStringPref(prefs::kSyncBookmarksBaseOrder, std::string());

  registry->RegisterBooleanPref(prefs::kSyncEnabled, false);
  registry->RegisterBooleanPref(prefs::kSyncBookmarksEnabled, false);
  registry->RegisterBooleanPref(prefs::kSyncSiteSettingsEnabled, false);
  registry->RegisterBooleanPref(prefs::kSyncHistoryEnabled, false);

  registry->RegisterTimePref(prefs::kSyncLatestRecordTime, base::Time());
  registry->RegisterTimePref(prefs::kSyncLatestDeviceRecordTime, base::Time());
  registry->RegisterTimePref(prefs::kSyncLastFetchTime, base::Time());
  registry->RegisterTimePref(prefs::kSyncLastCompactTimeBookmarks,
                             base::Time());

  registry->RegisterStringPref(prefs::kSyncDeviceList, std::string());
  registry->RegisterStringPref(prefs::kSyncApiVersion, std::string("0"));
  registry->RegisterIntegerPref(prefs::kSyncMigrateBookmarksVersion, 0);

  registry->RegisterListPref(prefs::kSyncRecordsToResend);
  registry->RegisterDictionaryPref(prefs::kSyncRecordsToResendMeta);
}

std::string Prefs::GetSeed() const {
  return pref_service_->GetString(kSyncSeed);
}

void Prefs::SetSeed(const std::string& seed) {
  DCHECK(!seed.empty());
  pref_service_->SetString(kSyncSeed, seed);
}

std::string Prefs::GetThisDeviceId() const {
  return pref_service_->GetString(kSyncDeviceId);
}

void Prefs::SetThisDeviceId(const std::string& device_id) {
  DCHECK(!device_id.empty());
  pref_service_->SetString(kSyncDeviceId, device_id);
}

std::string Prefs::GetThisDeviceIdV2() const {
  return pref_service_->GetString(kSyncDeviceIdV2);
}

void Prefs::SetThisDeviceIdV2(const std::string& device_id_v2) {
  DCHECK(!device_id_v2.empty());
  pref_service_->SetString(kSyncDeviceIdV2, device_id_v2);
}

std::string Prefs::GetThisDeviceName() const {
  return pref_service_->GetString(kSyncDeviceName);
}

void Prefs::SetThisDeviceName(const std::string& device_name) {
  DCHECK(!device_name.empty());
  pref_service_->SetString(kSyncDeviceName, device_name);
}
std::string Prefs::GetBookmarksBaseOrder() {
  return pref_service_->GetString(kSyncBookmarksBaseOrder);
}
void Prefs::SetBookmarksBaseOrder(const std::string& order) {
  pref_service_->SetString(kSyncBookmarksBaseOrder, order);
}

bool Prefs::GetSyncEnabled() const {
  return pref_service_->GetBoolean(kSyncEnabled);
}

void Prefs::SetSyncEnabled(const bool sync_this_device) {
  pref_service_->SetBoolean(kSyncEnabled, sync_this_device);
}

bool Prefs::GetSyncBookmarksEnabled() const {
  return pref_service_->GetBoolean(kSyncBookmarksEnabled);
}

void Prefs::SetSyncBookmarksEnabled(const bool sync_bookmarks_enabled) {
  pref_service_->SetBoolean(kSyncBookmarksEnabled, sync_bookmarks_enabled);
}

bool Prefs::GetSyncSiteSettingsEnabled() const {
  return pref_service_->GetBoolean(kSyncSiteSettingsEnabled);
}

void Prefs::SetSyncSiteSettingsEnabled(const bool sync_site_settings_enabled) {
  pref_service_->SetBoolean(
      kSyncSiteSettingsEnabled, sync_site_settings_enabled);
}

bool Prefs::GetSyncHistoryEnabled() const {
  return pref_service_->GetBoolean(kSyncHistoryEnabled);
}

void Prefs::SetSyncHistoryEnabled(const bool sync_history_enabled) {
  pref_service_->SetBoolean(kSyncHistoryEnabled, sync_history_enabled);
}

std::unique_ptr<brave_sync::Settings> Prefs::GetBraveSyncSettings() const {
  auto settings = std::make_unique<brave_sync::Settings>();

  settings->this_device_name_ = GetThisDeviceName();
  settings->this_device_id_ = GetThisDeviceId();
  settings->this_device_id_v2_ = GetThisDeviceIdV2();
  settings->sync_this_device_ = GetSyncEnabled();
  settings->sync_bookmarks_ = GetSyncBookmarksEnabled();
  settings->sync_settings_ = GetSyncSiteSettingsEnabled();
  settings->sync_history_ = GetSyncHistoryEnabled();

  settings->sync_configured_ =
      !GetSeed().empty() && !GetThisDeviceName().empty();

  return settings;
}

void Prefs::SetLatestRecordTime(const base::Time &time) {
  pref_service_->SetTime(kSyncLatestRecordTime, time);
}

base::Time Prefs::GetLatestRecordTime() {
  return pref_service_->GetTime(kSyncLatestRecordTime);
}
void Prefs::SetLatestDeviceRecordTime(const base::Time& time) {
  pref_service_->SetTime(kSyncLatestDeviceRecordTime, time);
}
base::Time Prefs::GetLatestDeviceRecordTime() {
  return pref_service_->GetTime(kSyncLatestDeviceRecordTime);
}

void Prefs::SetLastFetchTime(const base::Time &time) {
  pref_service_->SetTime(kSyncLastFetchTime, time);
}

base::Time Prefs::GetLastFetchTime() {
  return pref_service_->GetTime(kSyncLastFetchTime);
}

void Prefs::SetLastCompactTimeBookmarks(const base::Time &time) {
  pref_service_->SetTime(kSyncLastCompactTimeBookmarks, time);
}

base::Time Prefs::GetLastCompactTimeBookmarks() {
  return pref_service_->GetTime(kSyncLastCompactTimeBookmarks);
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

int Prefs::GetMigratedBookmarksVersion() {
  return pref_service_->GetInteger(kSyncMigrateBookmarksVersion);
}

void Prefs::SetMigratedBookmarksVersion(const int migrate_bookmarks) {
  pref_service_->SetInteger(kSyncMigrateBookmarksVersion, migrate_bookmarks);
}

std::vector<std::string> Prefs::GetRecordsToResend() const {
  std::vector<std::string> result;
  const base::Value* records = pref_service_->GetList(kSyncRecordsToResend);
  for (const base::Value& record : records->GetList()) {
    result.push_back(record.GetString());
  }
  return result;
}

void Prefs::AddToRecordsToResend(const std::string& object_id,
                                 std::unique_ptr<base::DictionaryValue> meta) {
  ListPrefUpdate list_update(pref_service_, kSyncRecordsToResend);
  list_update->GetList().emplace_back(object_id);
  SetRecordToResendMeta(object_id, std::move(meta));
}

void Prefs::RemoveFromRecordsToResend(const std::string& object_id) {
  ListPrefUpdate list_update(pref_service_, kSyncRecordsToResend);
  base::Erase(list_update->GetList(), base::Value(object_id));
  DictionaryPrefUpdate dict_update(pref_service_, kSyncRecordsToResendMeta);
  dict_update->RemoveKey(object_id);
}

const base::DictionaryValue* Prefs::GetRecordToResendMeta(
    const std::string& object_id) const {
  const base::DictionaryValue* dict =
      pref_service_->GetDictionary(kSyncRecordsToResendMeta);
  const base::DictionaryValue* meta = nullptr;
  const base::Value* meta_value = dict->FindDictKey(object_id);
  if (meta_value) {
    meta_value->GetAsDictionary(&meta);
  }
  return meta;
}

void Prefs::SetRecordToResendMeta(const std::string& object_id,
                                  std::unique_ptr<base::DictionaryValue> meta) {
  DictionaryPrefUpdate dict_update(pref_service_, kSyncRecordsToResendMeta);
  dict_update->SetDictionary(object_id, std::move(meta));
}

void Prefs::Clear() {
  pref_service_->ClearPref(kSyncDeviceId);
  pref_service_->ClearPref(kSyncSeed);
  pref_service_->ClearPref(kSyncDeviceName);
  pref_service_->ClearPref(kSyncEnabled);
  pref_service_->ClearPref(kSyncBookmarksEnabled);
  pref_service_->ClearPref(kSyncBookmarksBaseOrder);
  pref_service_->ClearPref(kSyncSiteSettingsEnabled);
  pref_service_->ClearPref(kSyncHistoryEnabled);
  pref_service_->ClearPref(kSyncLatestRecordTime);
  pref_service_->ClearPref(kSyncLatestDeviceRecordTime);
  pref_service_->ClearPref(kSyncLastFetchTime);
  pref_service_->ClearPref(kSyncDeviceList);
  pref_service_->ClearPref(kSyncApiVersion);
  pref_service_->ClearPref(kSyncMigrateBookmarksVersion);
  pref_service_->ClearPref(kSyncRecordsToResend);
  pref_service_->ClearPref(kSyncRecordsToResendMeta);
}

}  // namespace prefs
}  // namespace brave_sync
