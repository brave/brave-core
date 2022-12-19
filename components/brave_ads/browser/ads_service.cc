/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/browser/ads_service.h"

#include "brave/components/brave_ads/common/constants.h"
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

  registry->RegisterBooleanPref(prefs::kShouldShowOnboardingNotification, true);

  registry->RegisterIntegerPref(prefs::kSupportedCountryCodesLastSchemaVersion,
                                0);

  registry->RegisterIntegerPref(prefs::kVersion, ads::kCurrentVersionNumber);

  registry->RegisterBooleanPref(ads::prefs::kEnabled, false);

  registry->RegisterIntegerPref(prefs::kNotificationAdLastScreenPositionX, 0);
  registry->RegisterIntegerPref(prefs::kNotificationAdLastScreenPositionY, 0);
  registry->RegisterBooleanPref(prefs::kNotificationAdDidFallbackToCustom,
                                false);

  registry->RegisterInt64Pref(ads::prefs::kMaximumNotificationAdsPerHour, -1);

  registry->RegisterIntegerPref(ads::prefs::kIdleTimeThreshold, 15);

  registry->RegisterBooleanPref(ads::prefs::kShouldAllowSubdivisionTargeting,
                                false);
  registry->RegisterStringPref(ads::prefs::kSubdivisionTargetingCode, "AUTO");
  registry->RegisterStringPref(
      ads::prefs::kAutoDetectedSubdivisionTargetingCode, "");

  registry->RegisterStringPref(ads::prefs::kCatalogId, "");
  registry->RegisterIntegerPref(ads::prefs::kCatalogVersion, 0);
  registry->RegisterInt64Pref(ads::prefs::kCatalogPing, 0);
  registry->RegisterTimePref(ads::prefs::kCatalogLastUpdated, base::Time());

  registry->RegisterIntegerPref(ads::prefs::kIssuerPing, 7'200'000);
  registry->RegisterListPref(ads::prefs::kIssuers, base::Value::List());

  registry->RegisterDictionaryPref(ads::prefs::kEpsilonGreedyBanditArms,
                                   base::Value::Dict());
  registry->RegisterListPref(ads::prefs::kEpsilonGreedyBanditEligibleSegments,
                             base::Value::List());

  registry->RegisterListPref(ads::prefs::kNotificationAds, base::Value::List());
  registry->RegisterTimePref(ads::prefs::kServeAdAt, base::Time());

  registry->RegisterTimePref(ads::prefs::kNextTokenRedemptionAt,
                             base::Time::Now());

  registry->RegisterBooleanPref(ads::prefs::kHasMigratedClientState, false);
  registry->RegisterBooleanPref(ads::prefs::kHasMigratedConfirmationState,
                                false);
  registry->RegisterBooleanPref(ads::prefs::kHasMigratedConversionState, false);
  registry->RegisterBooleanPref(ads::prefs::kHasMigratedNotificationState,
                                false);
  registry->RegisterBooleanPref(ads::prefs::kHasMigratedRewardsState, false);
  registry->RegisterBooleanPref(ads::prefs::kShouldMigrateVerifiedRewardsUser,
                                false);

  registry->RegisterUint64Pref(ads::prefs::kConfirmationsHash, 0);
  registry->RegisterUint64Pref(ads::prefs::kClientHash, 0);

  registry->RegisterStringPref(ads::prefs::kBrowserVersionNumber, "");
}

}  // namespace brave_ads
