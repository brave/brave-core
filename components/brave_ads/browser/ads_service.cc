/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/browser/ads_service.h"

#include "bat/ads/pref_names.h"
#include "brave/components/brave_ads/common/pref_names.h"
#include "components/pref_registry/pref_registry_syncable.h"

namespace brave_ads {

AdsService::AdsService() = default;

AdsService::~AdsService() = default;

void AdsService::AddObserver(AdsServiceObserver* observer) {
  observers_.AddObserver(observer);
}

void AdsService::RemoveObserver(AdsServiceObserver* observer) {
  observers_.RemoveObserver(observer);
}

void AdsService::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterBooleanPref(prefs::kAdsWereDisabled, false);
  registry->RegisterBooleanPref(prefs::kHasAdsP3AState, false);

  registry->RegisterBooleanPref(prefs::kShouldShowMyFirstAdNotification, true);

  registry->RegisterIntegerPref(prefs::kSupportedCountryCodesLastSchemaVersion,
                                0);

  registry->RegisterIntegerPref(
      prefs::kSupportedCountryCodesSchemaVersion,
      prefs::kSupportedCountryCodesSchemaVersionNumber);

  registry->RegisterIntegerPref(prefs::kVersion, prefs::kCurrentVersionNumber);

  registry->RegisterBooleanPref(ads::prefs::kEnabled, false);

  registry->RegisterIntegerPref(prefs::kAdNotificationLastScreenPositionX, 0);
  registry->RegisterIntegerPref(prefs::kAdNotificationLastScreenPositionY, 0);

  registry->RegisterBooleanPref(ads::prefs::kShouldAllowConversionTracking,
                                true);

  registry->RegisterInt64Pref(ads::prefs::kAdsPerHour, -1);

  registry->RegisterIntegerPref(ads::prefs::kIdleTimeThreshold, 15);

  registry->RegisterBooleanPref(ads::prefs::kShouldAllowAdsSubdivisionTargeting,
                                false);
  registry->RegisterStringPref(ads::prefs::kAdsSubdivisionTargetingCode,
                               "AUTO");
  registry->RegisterStringPref(
      ads::prefs::kAutoDetectedAdsSubdivisionTargetingCode, "");

  registry->RegisterStringPref(ads::prefs::kCatalogId, "");
  registry->RegisterIntegerPref(ads::prefs::kCatalogVersion, 0);
  registry->RegisterInt64Pref(ads::prefs::kCatalogPing, 0);
  registry->RegisterInt64Pref(ads::prefs::kCatalogLastUpdated, 0);

  registry->RegisterStringPref(ads::prefs::kEpsilonGreedyBanditArms, "");
  registry->RegisterStringPref(ads::prefs::kEpsilonGreedyBanditEligibleSegments,
                               "");

  registry->RegisterBooleanPref(ads::prefs::kHasMigratedConversionState, false);
}

}  // namespace brave_ads
