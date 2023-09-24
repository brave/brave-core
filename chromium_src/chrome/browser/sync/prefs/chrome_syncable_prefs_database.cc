/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/sync/prefs/chrome_syncable_prefs_database.h"

#include <string_view>

#include "base/containers/fixed_flat_map.h"

namespace browser_sync {
namespace {
namespace brave_syncable_prefs_ids {
enum {
  // Starts at 300000 to avoid clashing with the Chromium's syncable_prefs_ids:
  // chrome_syncable_prefs_database.cc starts at 100000
  // ios_chrome_syncable_prefs_database.cc starts at 200000
  kProfileContentSettingsExceptionsShieldsAds = 300000,
  kProfileContentSettingsExceptionsTrackers = 300001,
  kProfileContentSettingsExceptionsHttpsUpgrades = 300002,
  kProfileContentSettingsExceptionsHttpUpgradableResources = 300003,
  kProfileContentSettingsExceptionsReferrers = 300004,
  kProfileContentSettingsExceptionsShieldsCookiesV3 = 300005,
  kProfileContentSettingsExceptionsCosmeticFiltering = 300006,
  kProfileContentSettingsExceptionsFingerprintingV2 = 300007,
  kProfileContentSettingsExceptionsBraveShields = 300008,
  kProfileContentSettingsExceptionsBraveSpeedreader = 300009,
  kProfileDefaultContentSettingValuesShieldsAds = 300010,
  kProfileDefaultContentSettingValuesTrackers = 300011,
  kProfileDefaultContentSettingValuesHttpsUpgrades = 300012,
  kProfileDefaultContentSettingValuesHttpUpgradableResources = 300013,
  kProfileDefaultContentSettingValuesReferrers = 300014,
  kProfileDefaultContentSettingValuesShieldsCookiesV3 = 300015,
  kProfileDefaultContentSettingValuesCosmeticFiltering = 300016,
  kProfileDefaultContentSettingValuesFingerprintingV2 = 300017,
  kProfileDefaultContentSettingValuesBraveShields = 300018,
  kProfileDefaultContentSettingValuesBraveSpeedreader = 300019,
};
}  // namespace brave_syncable_prefs_ids

const auto& BraveSyncablePreferences() {
  static const auto kBraveSyncablePrefsAllowList = base::MakeFixedFlatMap<
      std::string_view, sync_preferences::SyncablePrefMetadata>({
      {"profile.content_settings.exceptions.shieldsAds",
       {brave_syncable_prefs_ids::kProfileContentSettingsExceptionsShieldsAds,
        syncer::PREFERENCES, /*is_history_opt_in_required*/ false,
        sync_preferences::MergeBehavior::kNone}},
      {"profile.content_settings.exceptions.trackers",
       {brave_syncable_prefs_ids::kProfileContentSettingsExceptionsTrackers,
        syncer::PREFERENCES, /*is_history_opt_in_required*/ false,
        sync_preferences::MergeBehavior::kNone}},
      {"profile.content_settings.exceptions.httpsUpgrades",
       {brave_syncable_prefs_ids::
            kProfileContentSettingsExceptionsHttpsUpgrades,
        syncer::PREFERENCES, /*is_history_opt_in_required*/ false,
        sync_preferences::MergeBehavior::kNone}},
      {"profile.content_settings.exceptions.httpUpgradableResources",
       {brave_syncable_prefs_ids::
            kProfileContentSettingsExceptionsHttpUpgradableResources,
        syncer::PREFERENCES, /*is_history_opt_in_required*/ false,
        sync_preferences::MergeBehavior::kNone}},
      {"profile.content_settings.exceptions.referrers",
       {brave_syncable_prefs_ids::kProfileContentSettingsExceptionsReferrers,
        syncer::PREFERENCES, /*is_history_opt_in_required*/ false,
        sync_preferences::MergeBehavior::kNone}},
      {"profile.content_settings.exceptions.shieldsCookiesV3",
       {brave_syncable_prefs_ids::
            kProfileContentSettingsExceptionsShieldsCookiesV3,
        syncer::PREFERENCES, /*is_history_opt_in_required*/ false,
        sync_preferences::MergeBehavior::kNone}},
      {"profile.content_settings.exceptions.cosmeticFiltering",
       {brave_syncable_prefs_ids::
            kProfileContentSettingsExceptionsCosmeticFiltering,
        syncer::PREFERENCES, /*is_history_opt_in_required*/ false,
        sync_preferences::MergeBehavior::kNone}},
      {"profile.content_settings.exceptions.fingerprintingV2",
       {brave_syncable_prefs_ids::
            kProfileContentSettingsExceptionsFingerprintingV2,
        syncer::PREFERENCES, /*is_history_opt_in_required*/ false,
        sync_preferences::MergeBehavior::kNone}},
      {"profile.content_settings.exceptions.braveShields",
       {brave_syncable_prefs_ids::kProfileContentSettingsExceptionsBraveShields,
        syncer::PREFERENCES, /*is_history_opt_in_required*/ false,
        sync_preferences::MergeBehavior::kNone}},
      {"profile.content_settings.exceptions.braveSpeedreader",
       {brave_syncable_prefs_ids::
            kProfileContentSettingsExceptionsBraveSpeedreader,
        syncer::PREFERENCES, /*is_history_opt_in_required*/ false,
        sync_preferences::MergeBehavior::kNone}},
      {"profile.default_content_setting_values.shieldsAds",
       {brave_syncable_prefs_ids::kProfileDefaultContentSettingValuesShieldsAds,
        syncer::PREFERENCES, /*is_history_opt_in_required*/ false,
        sync_preferences::MergeBehavior::kNone}},
      {"profile.default_content_setting_values.trackers",
       {brave_syncable_prefs_ids::kProfileDefaultContentSettingValuesTrackers,
        syncer::PREFERENCES, /*is_history_opt_in_required*/ false,
        sync_preferences::MergeBehavior::kNone}},
      {"profile.default_content_setting_values.httpsUpgrades",
       {brave_syncable_prefs_ids::
            kProfileDefaultContentSettingValuesHttpsUpgrades,
        syncer::PREFERENCES, /*is_history_opt_in_required*/ false,
        sync_preferences::MergeBehavior::kNone}},
      {"profile.default_content_setting_values.httpUpgradableResources",
       {brave_syncable_prefs_ids::
            kProfileDefaultContentSettingValuesHttpUpgradableResources,
        syncer::PREFERENCES, /*is_history_opt_in_required*/ false,
        sync_preferences::MergeBehavior::kNone}},
      {"profile.default_content_setting_values.referrers",
       {brave_syncable_prefs_ids::kProfileDefaultContentSettingValuesReferrers,
        syncer::PREFERENCES, /*is_history_opt_in_required*/ false,
        sync_preferences::MergeBehavior::kNone}},
      {"profile.default_content_setting_values.shieldsCookiesV3",
       {brave_syncable_prefs_ids::
            kProfileDefaultContentSettingValuesShieldsCookiesV3,
        syncer::PREFERENCES, /*is_history_opt_in_required*/ false,
        sync_preferences::MergeBehavior::kNone}},
      {"profile.default_content_setting_values.cosmeticFiltering",
       {brave_syncable_prefs_ids::
            kProfileDefaultContentSettingValuesCosmeticFiltering,
        syncer::PREFERENCES, /*is_history_opt_in_required*/ false,
        sync_preferences::MergeBehavior::kNone}},
      {"profile.default_content_setting_values.fingerprintingV2",
       {brave_syncable_prefs_ids::
            kProfileDefaultContentSettingValuesFingerprintingV2,
        syncer::PREFERENCES, /*is_history_opt_in_required*/ false,
        sync_preferences::MergeBehavior::kNone}},
      {"profile.default_content_setting_values.braveShields",
       {brave_syncable_prefs_ids::
            kProfileDefaultContentSettingValuesBraveShields,
        syncer::PREFERENCES, /*is_history_opt_in_required*/ false,
        sync_preferences::MergeBehavior::kNone}},
      {"profile.default_content_setting_values.braveSpeedreader",
       {brave_syncable_prefs_ids::
            kProfileDefaultContentSettingValuesBraveSpeedreader,
        syncer::PREFERENCES, /*is_history_opt_in_required*/ false,
        sync_preferences::MergeBehavior::kNone}},
  });
  return kBraveSyncablePrefsAllowList;
}
}  // namespace
}  // namespace browser_sync

#define GetSyncablePrefMetadata GetSyncablePrefMetadata_ChromiumImpl
#include "src/chrome/browser/sync/prefs/chrome_syncable_prefs_database.cc"
#undef GetSyncablePrefMetadata

namespace browser_sync {

absl::optional<sync_preferences::SyncablePrefMetadata>
ChromeSyncablePrefsDatabase::GetSyncablePrefMetadata(
    const std::string& pref_name) const {
  const auto* it = BraveSyncablePreferences().find(pref_name);
  if (it != BraveSyncablePreferences().end()) {
    return it->second;
  }
  return GetSyncablePrefMetadata_ChromiumImpl(pref_name);
}

}  // namespace browser_sync
