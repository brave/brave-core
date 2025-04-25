/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ntp_background_images/common/view_counter_pref_registry.h"

#include "brave/components/ntp_background_images/common/new_tab_takeover_infobar_util.h"
#include "brave/components/ntp_background_images/common/pref_names.h"
#include "brave/components/ntp_background_images/common/view_counter_pref_names.h"
#include "brave/components/ntp_background_images/common/view_counter_theme_option_type.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"

namespace ntp_background_images {

namespace {

constexpr char kCountToBrandedWallpaperPref[] =
    "brave.count_to_branded_wallpaper";

}  // namespace

void RegisterLocalStatePrefs(PrefRegistrySimple* registry) {
  registry->RegisterListPref(prefs::kNewTabsCreated);
  registry->RegisterListPref(prefs::kSponsoredNewTabsCreated);
}

void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterBooleanPref(prefs::kBrandedWallpaperNotificationDismissed,
                                false);
  registry->RegisterBooleanPref(
      prefs::kNewTabPageShowSponsoredImagesBackgroundImage, true);
  // Integer type is used because this pref is used by radio button group in
  // appearance settings. Super referral is disabled when it is set to kDefault.
  registry->RegisterIntegerPref(prefs::kNewTabPageSuperReferralThemesOption,
                                static_cast<int>(ThemesOption::kSuperReferral));
  registry->RegisterBooleanPref(prefs::kNewTabPageShowBackgroundImage, true);
  registry->RegisterIntegerPref(prefs::kNewTabTakeoverInfobarShowCount,
                                GetNewTabTakeoverInfobarShowCountThreshold());
}

void RegisterProfilePrefsForMigration(
    user_prefs::PrefRegistrySyncable* registry) {
  // Added 09/2023.
  registry->RegisterIntegerPref(kCountToBrandedWallpaperPref, 0);
}

void MigrateObsoleteProfilePrefs(PrefService* prefs) {
  // Added 09/2023.
  prefs->ClearPref(kCountToBrandedWallpaperPref);
}

}  // namespace ntp_background_images
