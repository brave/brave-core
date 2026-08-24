/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ntp_background_images/common/view_counter_pref_registry.h"

#include "brave/components/brave_ads/buildflags/buildflags.h"
#include "brave/components/ntp_background_images/common/infobar_constants.h"
#include "brave/components/ntp_background_images/common/pref_names.h"
#include "brave/components/ntp_background_images/common/view_counter_pref_names.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"

#if BUILDFLAG(ENABLE_BRAVE_ADS)
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"
#endif  // BUILDFLAG(ENABLE_BRAVE_ADS)

namespace ntp_background_images {

namespace {

// Added 09/2023.
constexpr char kCountToBrandedWallpaperPref[] =
    "brave.count_to_branded_wallpaper";

// Added 05/2025.
// Migration was needed for the following reasons:
// - Reset the counter as the infobar was incorrectly displayed due to a bug
// - Changed preference name as part of using 'display' instead of 'show'
constexpr char kNewTabTakeoverInfobarShowCount[] =
    "brave.new_tab_page.new_tab_takeover_infobar_show_count";

// Added 11/2025.
constexpr char kNewTabPageSuperReferralThemesOption[] =
    "brave.new_tab_page.super_referral_themes_option";

// Added 08/2026.
constexpr char kObsoleteNewTabPageShowSponsoredImages[] =
    "brave.new_tab_page.show_branded_background_image";
constexpr char kObsoleteNewTabPageShowSponsoredSites[] =
    "brave.new_tab_page.show_sponsored_sites";

}  // namespace

void RegisterLocalStatePrefs(PrefRegistrySimple* registry) {
  registry->RegisterListPref(prefs::kNewTabsCreated);
  registry->RegisterListPref(prefs::kSponsoredNewTabsCreated);
}

void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterBooleanPref(prefs::kNewTabPageSponsoredImagesSurveyPanelist,
                                false);
  registry->RegisterBooleanPref(prefs::kBrandedWallpaperNotificationDismissed,
                                false);
  registry->RegisterBooleanPref(prefs::kNewTabPageShowBackgroundImage, true);
  registry->RegisterIntegerPref(
      prefs::kNewTabTakeoverInfobarRemainingDisplayCount,
      kNewTabTakeoverInfobarRemainingDisplayCountThreshold);
  registry->RegisterListPref(prefs::kNewTabsCreatedDaily);
}

void RegisterProfilePrefsForMigration(
    user_prefs::PrefRegistrySyncable* registry) {
  // Added 09/2023.
  registry->RegisterIntegerPref(kCountToBrandedWallpaperPref, 0);

  // Added 05/2025.
  registry->RegisterIntegerPref(kNewTabTakeoverInfobarShowCount, 0);

  // Added 11/2025.
  registry->RegisterIntegerPref(kNewTabPageSuperReferralThemesOption, 0);

  // Added 08/2026.
  registry->RegisterBooleanPref(kObsoleteNewTabPageShowSponsoredImages, true);
  registry->RegisterBooleanPref(kObsoleteNewTabPageShowSponsoredSites, true);
}

void MigrateObsoleteProfilePrefs(PrefService* prefs) {
  // Added 09/2023.
  prefs->ClearPref(kCountToBrandedWallpaperPref);

  // Added 05/2025.
  prefs->ClearPref(kNewTabTakeoverInfobarShowCount);

  // Added 11/2025.
  prefs->ClearPref(kNewTabPageSuperReferralThemesOption);

  // Added 08/2026.
#if BUILDFLAG(ENABLE_BRAVE_ADS)
  if (prefs->HasPrefPath(kObsoleteNewTabPageShowSponsoredImages) ||
      prefs->HasPrefPath(kObsoleteNewTabPageShowSponsoredSites)) {
    prefs->SetBoolean(
        brave_ads::prefs::kSponsoredEnabled,
        prefs->GetBoolean(kObsoleteNewTabPageShowSponsoredImages) &&
            prefs->GetBoolean(kObsoleteNewTabPageShowSponsoredSites));
  }
#endif  // BUILDFLAG(ENABLE_BRAVE_ADS)
  prefs->ClearPref(kObsoleteNewTabPageShowSponsoredImages);
  prefs->ClearPref(kObsoleteNewTabPageShowSponsoredSites);
}

}  // namespace ntp_background_images
