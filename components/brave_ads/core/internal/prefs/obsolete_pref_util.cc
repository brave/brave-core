/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/public/prefs/obsolete_pref_util.h"

#include <string_view>

#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"

namespace brave_ads {

namespace {

constexpr std::string_view kObsoleteHasMigratedConfirmationStateV8 =
    "brave.brave_ads.state.has_migrated.confirmations.v8";
constexpr std::string_view kObsoleteHasMigratedStateV2 =
    "brave.brave_ads.state.has_migrated.v2";

constexpr std::string_view kObsoleteNotificationAdLastNormalizedCoordinateX =
    "brave.brave_ads.ad_notification.last_normalized_coordinate_x";
constexpr std::string_view kObsoleteNotificationAdLastNormalizedCoordinateY =
    "brave.brave_ads.ad_notification.last_normalized_coordinate_y";
constexpr std::string_view kObsoleteNotificationAdDidFallbackToCustom =
    "brave.brave_ads.ad_notification.did_fallback_to_custom";

constexpr std::string_view kNewTabPageEventCountConstellationDictPref =
    "brave.brave_ads.p3a.ntp_event_count_constellation";
constexpr std::string_view kNewTabPageKnownCampaignsDictPref =
    "brave.brave_ads.p3a.ntp_known_campaigns";

}  // namespace

void RegisterProfilePrefsForMigration(PrefRegistrySimple* const registry) {
  // Added 03/2026.
  registry->RegisterDoublePref(kObsoleteNotificationAdLastNormalizedCoordinateX,
                               0.0);
  registry->RegisterDoublePref(kObsoleteNotificationAdLastNormalizedCoordinateY,
                               0.0);
  registry->RegisterBooleanPref(kObsoleteNotificationAdDidFallbackToCustom,
                                false);

  // Added 04/2026.
  registry->RegisterBooleanPref(kObsoleteHasMigratedConfirmationStateV8, false);
  registry->RegisterBooleanPref(kObsoleteHasMigratedStateV2, false);
}

void MigrateObsoleteProfilePrefs(PrefService* const prefs) {
  // Added 03/2026.
  prefs->ClearPref(kObsoleteNotificationAdLastNormalizedCoordinateX);
  prefs->ClearPref(kObsoleteNotificationAdLastNormalizedCoordinateY);
  prefs->ClearPref(kObsoleteNotificationAdDidFallbackToCustom);

  // Added 04/2026.
  prefs->ClearPref(kObsoleteHasMigratedConfirmationStateV8);
  prefs->ClearPref(kObsoleteHasMigratedStateV2);
}

void RegisterLocalStatePrefsForMigration(PrefRegistrySimple* registry) {
  // Added 10/2025
  registry->RegisterDictionaryPref(kNewTabPageEventCountConstellationDictPref);
  registry->RegisterDictionaryPref(kNewTabPageKnownCampaignsDictPref);
}

void MigrateObsoleteLocalStatePrefs(PrefService* local_state) {
  // Added 10/2025
  local_state->ClearPref(kNewTabPageEventCountConstellationDictPref);
  local_state->ClearPref(kNewTabPageKnownCampaignsDictPref);
}

}  // namespace brave_ads
