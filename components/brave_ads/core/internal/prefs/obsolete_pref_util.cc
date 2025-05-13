/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/public/prefs/obsolete_pref_util.h"

#include "brave/components/brave_ads/core/public/prefs/pref_names.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"

namespace brave_ads {

namespace {

constexpr char kObsoleteHasMigratedConversionState[] =
    "brave.brave_ads.migrated.conversion_state";
constexpr char kObsoleteHasMigratedNotificationState[] =
    "brave.brave_ads.has_migrated.notification_state";
constexpr char kObsoleteHasMigratedRewardsState[] =
    "brave.brave_ads.migrated.rewards_state";

constexpr char kObsoleteShouldMigrateVerifiedRewardsUser[] =
    "brave.brave_ads.rewards.verified_user.should_migrate";

constexpr char kObsoleteShouldShowSearchResultAdClickedInfoBar[] =
    "brave.brave_ads.should_show_search_result_ad_clicked_infobar";

void MaybeMigrateShouldShowSearchResultAdClickedInfoBarProfilePref(
    PrefService* const prefs) {
  if (!prefs->HasPrefPath(kObsoleteShouldShowSearchResultAdClickedInfoBar)) {
    return;
  }

  prefs->SetBoolean(
      prefs::kShouldShowSearchResultAdClickedInfoBar,
      prefs->GetBoolean(kObsoleteShouldShowSearchResultAdClickedInfoBar));
  prefs->ClearPref(kObsoleteShouldShowSearchResultAdClickedInfoBar);
}

}  // namespace

void RegisterProfilePrefsForMigration(PrefRegistrySimple* const registry) {
  // Added 08/2024.
  registry->RegisterBooleanPref(kObsoleteHasMigratedConversionState, false);
  registry->RegisterBooleanPref(kObsoleteHasMigratedNotificationState, false);
  registry->RegisterBooleanPref(kObsoleteHasMigratedRewardsState, false);

  // Added 10/2024.
  registry->RegisterBooleanPref(kObsoleteShouldMigrateVerifiedRewardsUser,
                                false);

  // Added 05/2025.
  registry->RegisterBooleanPref(kObsoleteShouldShowSearchResultAdClickedInfoBar,
                                false);
}

void MigrateObsoleteProfilePrefs(PrefService* const prefs) {
  // Added 08/2024.
  prefs->ClearPref(kObsoleteHasMigratedConversionState);
  prefs->ClearPref(kObsoleteHasMigratedNotificationState);
  prefs->ClearPref(kObsoleteHasMigratedRewardsState);

  // Added 10/2024.
  prefs->ClearPref(kObsoleteShouldMigrateVerifiedRewardsUser);

  // Added 05/2025.
  MaybeMigrateShouldShowSearchResultAdClickedInfoBarProfilePref(prefs);
}

}  // namespace brave_ads
