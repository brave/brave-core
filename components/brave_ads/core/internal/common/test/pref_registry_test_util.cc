/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/test/pref_registry_test_util.h"

#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/common/test/local_state_pref_registry_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/profile_pref_registry_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/time_test_util.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"
#include "brave/components/brave_rewards/core/pref_names.h"
#include "brave/components/ntp_background_images/common/pref_names.h"
#include "components/variations/pref_names.h"

namespace brave_ads::test {

void RegisterLocalStatePrefs() {
  // Ads prefs.
  RegisterLocalStateTimePref(prefs::kFirstRunAt, DistantPast());

  // Variations prefs.
  RegisterLocalStateStringPref(variations::prefs::kVariationsCountry, "US");
}

void RegisterProfilePrefs() {
  // Ads prefs.
  RegisterProfileTimeDeltaPref(prefs::kGracePeriod, base::Days(3));

  RegisterProfileStringPref(prefs::kDiagnosticId, "");

  RegisterProfileBooleanPref(prefs::kOptedInToNotificationAds, true);
  RegisterProfileInt64Pref(prefs::kMaximumNotificationAdsPerHour, -1);

  RegisterProfileBooleanPref(prefs::kOptedInToSearchResultAds, true);

  RegisterProfileBooleanPref(prefs::kShouldAllowSubdivisionTargeting, false);
  RegisterProfileStringPref(prefs::kSubdivisionTargetingUserSelectedSubdivision,
                            "AUTO");
  RegisterProfileStringPref(prefs::kSubdivisionTargetingAutoDetectedSubdivision,
                            "");

  RegisterProfileStringPref(prefs::kCatalogId, "");
  RegisterProfileIntegerPref(prefs::kCatalogVersion, 0);
  RegisterProfileInt64Pref(prefs::kCatalogPing, 7'200'000);
  RegisterProfileTimePref(prefs::kCatalogLastUpdated, base::Time());

  RegisterProfileIntegerPref(prefs::kIssuerPing, 0);
  RegisterProfileListPref(prefs::kIssuers);

  RegisterProfileListPref(prefs::kNotificationAds);
  RegisterProfileTimePref(prefs::kServeAdAt, Now());

  RegisterProfileTimePref(prefs::kNextPaymentTokenRedemptionAt,
                          DistantFuture());

  RegisterProfileDictPref(prefs::kAdReactions);
  RegisterProfileDictPref(prefs::kSegmentReactions);
  RegisterProfileListPref(prefs::kSaveAds);
  RegisterProfileListPref(prefs::kMarkedAsInappropriate);

  RegisterProfileBooleanPref(prefs::kHasMigratedState, true);
  RegisterProfileBooleanPref(prefs::kHasMigratedClientState, true);
  RegisterProfileBooleanPref(prefs::kHasMigratedConfirmationState, true);

  RegisterProfileStringPref(prefs::kBrowserVersionNumber, "");

  // Rewards prefs.
  RegisterProfileBooleanPref(brave_rewards::prefs::kEnabled, true);
  RegisterProfileStringPref(brave_rewards::prefs::kExternalWalletType,
                            "connected");

  // New tab page background image prefs.
  RegisterProfileBooleanPref(
      ntp_background_images::prefs::kNewTabPageShowBackgroundImage, true);
  RegisterProfileBooleanPref(ntp_background_images::prefs::
                                 kNewTabPageShowSponsoredImagesBackgroundImage,
                             true);
  RegisterProfileBooleanPref(
      ntp_background_images::prefs::kNewTabPageSponsoredImagesSurveyPanelist,
      true);
}

}  // namespace brave_ads::test
