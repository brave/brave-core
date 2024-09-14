/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_sync/brave_sync_prefs.h"

#include <utility>

#include "base/base64.h"
#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/strings/strcat.h"
#include "build/build_config.h"
#include "components/os_crypt/sync/os_crypt.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"

namespace brave_sync {
namespace {

// Stored as bip39 keywords (encrypted)
const char kSyncV2Seed[] = "brave_sync_v2.seed";
const char kSyncFailedDecryptSeedNoticeDismissed[] =
    "brave_sync_v2.failed_decrypt_seed_notice_dismissed";
const char kSyncAccountDeletedNoticePending[] =
    "brave_sync_v2.account_deleted_notice_pending";
const char kSyncLeaveChainDetails[] = "brave_sync_v2.diag.leave_chain_details";

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
const char kSyncV1Migrated[] = "brave_sync_v2.v1_migrated";
const char kSyncV1MetaInfoCleared[] = "brave_sync_v2.v1_meta_info_cleared";
const char kSyncV2MigrateNoticeDismissed[] =
    "brave_sync_v2.migrate_notice_dismissed";
// ============================================================================

constexpr size_t kLeaveChainDetailsMaxLen = 500;

}  // namespace

Prefs::Prefs(PrefService* pref_service) : pref_service_(*pref_service) {
#if BUILDFLAG(IS_IOS)
  add_leave_chain_detail_behaviour_ = AddLeaveChainDetailBehaviour::kAdd;
#else
  add_leave_chain_detail_behaviour_ = AddLeaveChainDetailBehaviour::kIgnore;
#endif
}

Prefs::~Prefs() = default;

// static
void Prefs::RegisterProfilePrefs(PrefRegistrySimple* registry) {
  registry->RegisterStringPref(kSyncV2Seed, std::string());
  registry->RegisterBooleanPref(kSyncFailedDecryptSeedNoticeDismissed, false);
  registry->RegisterBooleanPref(kSyncAccountDeletedNoticePending, false);
  registry->RegisterStringPref(kSyncLeaveChainDetails, std::string());
  registry->RegisterStringPref(kCustomSyncServiceUrl, std::string());
}

// static
void Prefs::RegisterProfilePrefsForMigration(PrefRegistrySimple* registry) {
  registry->RegisterStringPref(kSyncSeed, std::string());
  registry->RegisterBooleanPref(kSyncEnabled, false);
  registry->RegisterStringPref(kSyncDeviceId, std::string());
  registry->RegisterStringPref(kSyncDeviceIdV2, std::string());
  registry->RegisterStringPref(kSyncDeviceObjectId, std::string());
  registry->RegisterStringPref(kSyncPrevSeed, std::string());
  registry->RegisterStringPref(kSyncDeviceName, std::string());
  registry->RegisterStringPref(kSyncBookmarksBaseOrder, std::string());
  registry->RegisterBooleanPref(kSyncBookmarksEnabled, false);
  registry->RegisterBooleanPref(kSyncSiteSettingsEnabled, false);
  registry->RegisterBooleanPref(kSyncHistoryEnabled, false);
  registry->RegisterTimePref(kSyncLatestRecordTime, base::Time());
  registry->RegisterTimePref(kSyncLatestDeviceRecordTime, base::Time());
  registry->RegisterTimePref(kSyncLastFetchTime, base::Time());
  registry->RegisterTimePref(kSyncLastCompactTimeBookmarks,
                             base::Time());
  registry->RegisterStringPref(kSyncDeviceList, std::string());
  registry->RegisterStringPref(kSyncApiVersion, std::string("0"));
  registry->RegisterIntegerPref(kSyncMigrateBookmarksVersion, 0);
  registry->RegisterListPref(kSyncRecordsToResend);
  registry->RegisterDictionaryPref(kSyncRecordsToResendMeta);
  registry->RegisterBooleanPref(kDuplicatedBookmarksRecovered, false);
  registry->RegisterIntegerPref(kDuplicatedBookmarksMigrateVersion, 0);
  registry->RegisterBooleanPref(kSyncV1Migrated, false);
  registry->RegisterBooleanPref(kSyncV1MetaInfoCleared, false);
  registry->RegisterBooleanPref(kSyncV2MigrateNoticeDismissed, false);
}

// static
std::string Prefs::GetSeedPath() {
  return kSyncV2Seed;
}

std::string Prefs::GetSeed(bool* failed_to_decrypt) const {
  CHECK(failed_to_decrypt);
  *failed_to_decrypt = true;

  const std::string encoded_seed = pref_service_->GetString(kSyncV2Seed);
  if (encoded_seed.empty()) {
    *failed_to_decrypt = false;
    return std::string();
  }

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

  *failed_to_decrypt = false;
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
  pref_service_->SetString(kSyncV2Seed, base::Base64Encode(encrypted_seed));
  SetSyncAccountDeletedNoticePending(false);
  return true;
}

bool Prefs::IsFailedDecryptSeedNoticeDismissed() const {
  return pref_service_->GetBoolean(kSyncFailedDecryptSeedNoticeDismissed);
}

void Prefs::DismissFailedDecryptSeedNotice() {
  pref_service_->SetBoolean(kSyncFailedDecryptSeedNoticeDismissed, true);
}

bool Prefs::IsSyncAccountDeletedNoticePending() const {
  return pref_service_->GetBoolean(kSyncAccountDeletedNoticePending);
}

void Prefs::SetSyncAccountDeletedNoticePending(bool is_pending) {
  pref_service_->SetBoolean(kSyncAccountDeletedNoticePending, is_pending);
}

void Prefs::AddLeaveChainDetail(const char* file, int line, const char* func) {
  if (add_leave_chain_detail_behaviour_ ==
      AddLeaveChainDetailBehaviour::kIgnore) {
    return;
  }

  std::string details = pref_service_->GetString(kSyncLeaveChainDetails);

  std::ostringstream stream;
  stream << base::Time::Now() << " "
         << base::FilePath::FromASCII(file).BaseName() << "(" << line << ") "
         << func << std::endl;

  std::string updated_details = base::StrCat({details, stream.str()});

  if (updated_details.size() > kLeaveChainDetailsMaxLen) {
    updated_details.assign(updated_details.end() - kLeaveChainDetailsMaxLen,
                           updated_details.end());
  }

  pref_service_->SetString(kSyncLeaveChainDetails, updated_details);
}

std::string Prefs::GetLeaveChainDetails() const {
  return pref_service_->GetString(kSyncLeaveChainDetails);
}

void Prefs::ClearLeaveChainDetails() {
  pref_service_->ClearPref(kSyncLeaveChainDetails);
}

// static
size_t Prefs::GetLeaveChainDetailsMaxLenForTests() {
  return kLeaveChainDetailsMaxLen;
}

// static
std::string Prefs::GetLeaveChainDetailsPathForTests() {
  return kSyncLeaveChainDetails;
}

void Prefs::SetAddLeaveChainDetailBehaviourForTests(
    AddLeaveChainDetailBehaviour add_leave_chain_detail_behaviour) {
  add_leave_chain_detail_behaviour_ = add_leave_chain_detail_behaviour;
}

void Prefs::Clear() {
  pref_service_->ClearPref(kSyncV2Seed);
  pref_service_->ClearPref(kSyncFailedDecryptSeedNoticeDismissed);
}

void MigrateBraveSyncPrefs(PrefService* prefs) {
  // Added 11/2019
  prefs->ClearPref(kSyncPrevSeed);

  // Added 05/2020
  prefs->ClearPref(kSyncSeed);

  // Added 11/2023
  prefs->ClearPref(kSyncEnabled);
  prefs->ClearPref(kDuplicatedBookmarksRecovered);
  prefs->ClearPref(kSyncDeviceId);
  prefs->ClearPref(kSyncDeviceIdV2);
  prefs->ClearPref(kSyncDeviceObjectId);
  prefs->ClearPref(kSyncDeviceName);
  prefs->ClearPref(kSyncBookmarksEnabled);
  prefs->ClearPref(kSyncBookmarksBaseOrder);
  prefs->ClearPref(kSyncSiteSettingsEnabled);
  prefs->ClearPref(kSyncHistoryEnabled);
  prefs->ClearPref(kSyncLatestRecordTime);
  prefs->ClearPref(kSyncLatestDeviceRecordTime);
  prefs->ClearPref(kSyncLastFetchTime);
  prefs->ClearPref(kSyncDeviceList);
  prefs->ClearPref(kSyncApiVersion);
  prefs->ClearPref(kSyncMigrateBookmarksVersion);
  prefs->ClearPref(kSyncRecordsToResend);
  prefs->ClearPref(kSyncRecordsToResendMeta);
  prefs->ClearPref(kDuplicatedBookmarksMigrateVersion);
  prefs->ClearPref(kSyncV1Migrated);
  prefs->ClearPref(kSyncV1MetaInfoCleared);
  prefs->ClearPref(kSyncV2MigrateNoticeDismissed);

  // Added 03/2024
#if !BUILDFLAG(IS_IOS)
  prefs->ClearPref(kSyncLeaveChainDetails);
#endif
}

}  // namespace brave_sync
