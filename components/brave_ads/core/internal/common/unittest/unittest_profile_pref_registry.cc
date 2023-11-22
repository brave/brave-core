/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/unittest/unittest_profile_pref_registry.h"

#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_profile_pref_registry_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"
#include "brave/components/brave_news/common/pref_names.h"
#include "brave/components/brave_rewards/common/pref_names.h"
#include "brave/components/ntp_background_images/common/pref_names.h"

namespace brave_ads {

void RegisterProfilePrefs() {
  RegisterProfileBooleanPref(brave_rewards::prefs::kEnabled, true);

  RegisterProfileStringPref(prefs::kDiagnosticId, "");

  RegisterProfileBooleanPref(brave_news::prefs::kBraveNewsOptedIn, true);
  RegisterProfileBooleanPref(brave_news::prefs::kNewTabPageShowToday, true);

  RegisterProfileBooleanPref(
      ntp_background_images::prefs::kNewTabPageShowBackgroundImage, true);
  RegisterProfileBooleanPref(ntp_background_images::prefs::
                                 kNewTabPageShowSponsoredImagesBackgroundImage,
                             true);

  RegisterProfileBooleanPref(prefs::kOptedInToNotificationAds, true);
  RegisterProfileInt64Pref(prefs::kMaximumNotificationAdsPerHour, -1);

  RegisterProfileBooleanPref(prefs::kShouldAllowSubdivisionTargeting, false);
  RegisterProfileStringPref(prefs::kSubdivisionTargetingSubdivision, "AUTO");
  RegisterProfileStringPref(prefs::kSubdivisionTargetingAutoDetectedSubdivision,
                            "");

  RegisterProfileStringPref(prefs::kCatalogId, "");
  RegisterProfileIntegerPref(prefs::kCatalogVersion, 0);
  RegisterProfileInt64Pref(prefs::kCatalogPing, 7'200'000);
  RegisterProfileTimePref(prefs::kCatalogLastUpdated, base::Time());

  RegisterProfileIntegerPref(prefs::kIssuerPing, 0);
  RegisterProfileListPref(prefs::kIssuers);

  RegisterProfileDictPref(prefs::kEpsilonGreedyBanditArms);
  RegisterProfileListPref(prefs::kEpsilonGreedyBanditEligibleSegments);

  RegisterProfileListPref(prefs::kNotificationAds);
  RegisterProfileTimePref(prefs::kServeAdAt, Now());

  RegisterProfileTimePref(prefs::kNextTokenRedemptionAt, DistantFuture());

  RegisterProfileBooleanPref(prefs::kHasMigratedClientState, true);
  RegisterProfileBooleanPref(prefs::kHasMigratedConfirmationState, true);
  RegisterProfileBooleanPref(prefs::kHasMigratedConversionState, true);
  RegisterProfileBooleanPref(prefs::kHasMigratedNotificationState, true);
  RegisterProfileBooleanPref(prefs::kHasMigratedRewardsState, true);
  RegisterProfileBooleanPref(prefs::kShouldMigrateVerifiedRewardsUser, false);

  RegisterProfileStringPref(prefs::kBrowserVersionNumber, "");
}

}  // namespace brave_ads
