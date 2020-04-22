/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_sync/brave_sync_prefs.h"

#include <utility>

#include "base/base64.h"
#include "components/os_crypt/os_crypt.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"

void MigrateBraveSyncPrefs(PrefService* prefs) {
  prefs->ClearPref(brave_sync::prefs::kSyncSeed);
  prefs->ClearPref(brave_sync::prefs::kSyncEnabled);
  prefs->ClearPref(brave_sync::prefs::kSyncPrevSeed);
  prefs->ClearPref(brave_sync::prefs::kDuplicatedBookmarksRecovered);
  prefs->ClearPref(brave_sync::prefs::kSyncDeviceId);
  prefs->ClearPref(brave_sync::prefs::kSyncDeviceIdV2);
  prefs->ClearPref(brave_sync::prefs::kSyncDeviceObjectId);
  prefs->ClearPref(brave_sync::prefs::kSyncDeviceName);
  prefs->ClearPref(brave_sync::prefs::kSyncBookmarksEnabled);
  prefs->ClearPref(brave_sync::prefs::kSyncBookmarksBaseOrder);
  prefs->ClearPref(brave_sync::prefs::kSyncSiteSettingsEnabled);
  prefs->ClearPref(brave_sync::prefs::kSyncHistoryEnabled);
  prefs->ClearPref(brave_sync::prefs::kSyncLatestRecordTime);
  prefs->ClearPref(brave_sync::prefs::kSyncLatestDeviceRecordTime);
  prefs->ClearPref(brave_sync::prefs::kSyncLastFetchTime);
  prefs->ClearPref(brave_sync::prefs::kSyncDeviceList);
  prefs->ClearPref(brave_sync::prefs::kSyncApiVersion);
  prefs->ClearPref(brave_sync::prefs::kSyncMigrateBookmarksVersion);
  prefs->ClearPref(brave_sync::prefs::kSyncRecordsToResend);
  prefs->ClearPref(brave_sync::prefs::kSyncRecordsToResendMeta);
  prefs->ClearPref(brave_sync::prefs::kDuplicatedBookmarksMigrateVersion);
}

namespace brave_sync {
namespace prefs {

const char kSyncV2Seed[] = "brave_sync_v2.seed";
const char kSyncV2Enabled[] = "brave_sync_v2.enabled";
const char kSyncV2Migrated[] = "brave_sync_v2.migrated";

// Deprecated
// ============================================================================
const char kSyncSeed[] = "brave_sync.seed";
const char kSyncEnabled[] = "brave_sync.enabled";
const char kSyncDeviceId[] = "brave_sync.device_id";
const char kSyncDeviceIdV2[] = "brave_sync.device_id_v2";
const char kSyncDeviceObjectId[] = "brave_sync.device_object_id";
const char kSyncPrevSeed[] = "brave_sync.previous_seed";
const char kSyncDeviceName[] = "brave_sync.device_name";
const char kSyncBookmarksBaseOrder[] = "brave_sync.bookmarks_base_order";
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
const char kDuplicatedBookmarksRecovered[] =
    "brave_sync_duplicated_bookmarks_recovered";
const char kDuplicatedBookmarksMigrateVersion[] =
    "brave_sync_duplicated_bookmarks_migrate_version";
// ============================================================================

Prefs::Prefs(PrefService* pref_service) : pref_service_(pref_service) {}

void Prefs::RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterStringPref(prefs::kSyncV2Seed, std::string());
  registry->RegisterBooleanPref(prefs::kSyncV2Enabled, false);
  registry->RegisterBooleanPref(prefs::kSyncV2Migrated, false);

// Deprecated
// ============================================================================
  registry->RegisterStringPref(prefs::kSyncSeed, std::string());
  registry->RegisterBooleanPref(prefs::kSyncEnabled, false);
  registry->RegisterStringPref(prefs::kSyncDeviceId, std::string());
  registry->RegisterStringPref(prefs::kSyncDeviceIdV2, std::string());
  registry->RegisterStringPref(prefs::kSyncDeviceObjectId, std::string());
  registry->RegisterStringPref(prefs::kSyncPrevSeed, std::string());
  registry->RegisterStringPref(prefs::kSyncDeviceName, std::string());
  registry->RegisterStringPref(prefs::kSyncBookmarksBaseOrder, std::string());
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
  registry->RegisterBooleanPref(kDuplicatedBookmarksRecovered, false);
  registry->RegisterIntegerPref(prefs::kDuplicatedBookmarksMigrateVersion, 0);
}

std::string Prefs::GetSeed() const {
  const std::string encoded_seed = pref_service_->GetString(kSyncV2Seed);
  std::string encrypted_seed;
  if (!base::Base64Decode(encoded_seed, &encrypted_seed)) {
    LOG(ERROR) << "base64 decode sync seed failure";
    return std::string();
  }
  std::string seed;
  if (!OSCrypt::DecryptString(encrypted_seed, &seed)) {
    LOG(ERROR) << "Decrypt sync seed failure";
    return std::string();
  }
  return seed;
}

bool Prefs::SetSeed(const std::string& seed) {
  DCHECK(!seed.empty());
  std::string encrypted_seed;
  if (!OSCrypt::EncryptString(seed, &encrypted_seed)) {
    LOG(ERROR) << "Encrypt sync seed failure";
    return false;
  }
  // String stored in prefs has to be UTF8 string so we use base64 to encode it.
  std::string encoded_seed;
  base::Base64Encode(encrypted_seed, &encoded_seed);
  pref_service_->SetString(kSyncV2Seed, encoded_seed);
  return true;
}

bool Prefs::IsSyncEnabled() const {
  return pref_service_->GetBoolean(kSyncV2Enabled);
}
void Prefs::SetSyncEnabled(bool is_enabled) {
  pref_service_->SetBoolean(kSyncV2Enabled, is_enabled);
}

bool Prefs::IsSyncV2Migrated() const {
  return pref_service_->GetBoolean(kSyncV2Migrated);
}

void Prefs::SetSyncV2Migrated(bool is_migrated) {
  pref_service_->SetBoolean(kSyncV2Migrated, is_migrated);
}

void Prefs::Clear() {
  pref_service_->ClearPref(kSyncV2Seed);
  pref_service_->ClearPref(kSyncV2Enabled);
}

}  // namespace prefs
}  // namespace brave_sync
