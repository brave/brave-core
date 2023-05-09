/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

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
  registry->RegisterBooleanPref(prefs::kShouldShowOnboardingNotification, true);

  registry->RegisterIntegerPref(prefs::kSupportedCountryCodesLastSchemaVersion,
                                0);

  registry->RegisterIntegerPref(prefs::kVersion, kCurrentPrefVersion);

  registry->RegisterBooleanPref(prefs::kEnabled, false);

  registry->RegisterIntegerPref(prefs::kNotificationAdLastScreenPositionX, 0);
  registry->RegisterIntegerPref(prefs::kNotificationAdLastScreenPositionY, 0);
  registry->RegisterBooleanPref(prefs::kNotificationAdDidFallbackToCustom,
                                false);

  registry->RegisterStringPref(prefs::kDiagnosticId, "");

  registry->RegisterInt64Pref(prefs::kMaximumNotificationAdsPerHour, -1);

  registry->RegisterIntegerPref(prefs::kIdleTimeThreshold, 15);

  registry->RegisterBooleanPref(prefs::kShouldAllowSubdivisionTargeting, false);
  registry->RegisterStringPref(prefs::kSubdivisionTargetingCode, "AUTO");
  registry->RegisterStringPref(prefs::kAutoDetectedSubdivisionTargetingCode,
                               "");

  registry->RegisterStringPref(prefs::kCatalogId, "");
  registry->RegisterIntegerPref(prefs::kCatalogVersion, 0);
  registry->RegisterInt64Pref(prefs::kCatalogPing, 0);
  registry->RegisterTimePref(prefs::kCatalogLastUpdated, base::Time());

  registry->RegisterIntegerPref(prefs::kIssuerPing, 7'200'000);
  registry->RegisterListPref(prefs::kIssuers);

  registry->RegisterDictionaryPref(prefs::kEpsilonGreedyBanditArms);
  registry->RegisterListPref(prefs::kEpsilonGreedyBanditEligibleSegments);

  registry->RegisterListPref(prefs::kNotificationAds);
  registry->RegisterTimePref(prefs::kServeAdAt, base::Time());

  registry->RegisterTimePref(prefs::kNextTokenRedemptionAt, base::Time::Now());

  registry->RegisterBooleanPref(prefs::kHasMigratedClientState, false);
  registry->RegisterBooleanPref(prefs::kHasMigratedConfirmationState, false);
  registry->RegisterBooleanPref(prefs::kHasMigratedConversionState, false);
  registry->RegisterBooleanPref(prefs::kHasMigratedNotificationState, false);
  registry->RegisterBooleanPref(prefs::kHasMigratedRewardsState, false);
  registry->RegisterBooleanPref(prefs::kShouldMigrateVerifiedRewardsUser,
                                false);

  registry->RegisterUint64Pref(prefs::kConfirmationsHash, 0);
  registry->RegisterUint64Pref(prefs::kClientHash, 0);

  registry->RegisterStringPref(prefs::kBrowserVersionNumber, "");
}

}  // namespace brave_ads
