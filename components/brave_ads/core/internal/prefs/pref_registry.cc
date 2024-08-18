/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/public/prefs/pref_registry.h"

#include "brave/components/brave_ads/core/public/prefs/pref_names.h"
#include "components/prefs/pref_registry_simple.h"

namespace brave_ads {

void RegisterLocalStatePrefs(PrefRegistrySimple* const registry) {
  // Add local state prefs to be registered here.
}

void RegisterProfilePrefs(PrefRegistrySimple* const registry) {
  // Ads prefs.
  registry->RegisterStringPref(prefs::kDiagnosticId, "");

  registry->RegisterBooleanPref(prefs::kOptedInToNotificationAds, false);
  registry->RegisterInt64Pref(prefs::kMaximumNotificationAdsPerHour, -1);

  registry->RegisterBooleanPref(prefs::kOptedInToSearchResultAds, true);

  registry->RegisterBooleanPref(prefs::kShouldAllowSubdivisionTargeting, false);
  registry->RegisterStringPref(
      prefs::kSubdivisionTargetingUserSelectedSubdivision, "AUTO");
  registry->RegisterStringPref(
      prefs::kSubdivisionTargetingAutoDetectedSubdivision, "");

  registry->RegisterStringPref(prefs::kCatalogId, "");
  registry->RegisterIntegerPref(prefs::kCatalogVersion, 0);
  registry->RegisterInt64Pref(prefs::kCatalogPing, 0);
  registry->RegisterTimePref(prefs::kCatalogLastUpdated, base::Time());

  registry->RegisterIntegerPref(prefs::kIssuerPing, 7'200'000);
  registry->RegisterListPref(prefs::kIssuers);

  registry->RegisterListPref(prefs::kNotificationAds);
  registry->RegisterTimePref(prefs::kServeAdAt, base::Time());

  registry->RegisterTimePref(prefs::kNextTokenRedemptionAt, base::Time::Now());

  registry->RegisterDictionaryPref(prefs::kAdReactions);
  registry->RegisterDictionaryPref(prefs::kSegmentReactions);
  registry->RegisterListPref(prefs::kSaveAds);
  registry->RegisterListPref(prefs::kMarkedAsInappropriate);

  registry->RegisterBooleanPref(prefs::kHasMigratedClientState, false);
  registry->RegisterBooleanPref(prefs::kHasMigratedConfirmationState, false);
  registry->RegisterBooleanPref(prefs::kShouldMigrateVerifiedRewardsUser,
                                false);

  registry->RegisterStringPref(prefs::kBrowserVersionNumber, "");

  // Ads service prefs.
  registry->RegisterBooleanPref(prefs::kShouldShowOnboardingNotification, true);

  registry->RegisterDoublePref(prefs::kNotificationAdLastNormalizedCoordinateX,
                               0.0);
  registry->RegisterDoublePref(prefs::kNotificationAdLastNormalizedCoordinateY,
                               0.0);
  registry->RegisterBooleanPref(prefs::kNotificationAdDidFallbackToCustom,
                                false);
}

}  // namespace brave_ads
