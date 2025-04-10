/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/public/prefs/obsolete_pref_util.h"

#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"

namespace brave_ads {

namespace {

constexpr char kHasMigratedConversionState[] =
    "brave.brave_ads.migrated.conversion_state";
constexpr char kHasMigratedNotificationState[] =
    "brave.brave_ads.has_migrated.notification_state";
constexpr char kHasMigratedRewardsState[] =
    "brave.brave_ads.migrated.rewards_state";

constexpr char kShouldMigrateVerifiedRewardsUser[] =
    "brave.brave_ads.rewards.verified_user.should_migrate";

}  // namespace

void RegisterProfilePrefsForMigration(PrefRegistrySimple* const registry) {
  // Added 08/2024.
  registry->RegisterBooleanPref(kHasMigratedConversionState, false);
  registry->RegisterBooleanPref(kHasMigratedNotificationState, false);
  registry->RegisterBooleanPref(kHasMigratedRewardsState, false);

  // Added 10/2024.
  registry->RegisterBooleanPref(kShouldMigrateVerifiedRewardsUser, false);
}

void MigrateObsoleteProfilePrefs(PrefService* const prefs) {
  // Added 08/2024.
  prefs->ClearPref(kHasMigratedConversionState);
  prefs->ClearPref(kHasMigratedNotificationState);
  prefs->ClearPref(kHasMigratedRewardsState);

  // Added 10/2024.
  prefs->ClearPref(kShouldMigrateVerifiedRewardsUser);
}

}  // namespace brave_ads
