/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/unittest/unittest_pref_registry.h"

#include "base/containers/contains.h"
#include "base/time/time.h"
#include "base/values.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_current_test_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_pref.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_pref_registry_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"
#include "brave/components/brave_news/common/pref_names.h"
#include "brave/components/brave_rewards/common/pref_names.h"
#include "brave/components/l10n/common/locale_util.h"
#include "brave/components/l10n/common/prefs.h"
#include "brave/components/ntp_background_images/common/pref_names.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace brave_ads {

void RegisterPrefs() {
  RegisterBooleanPref(brave_rewards::prefs::kEnabled, true);

  RegisterStringPref(prefs::kDiagnosticId, "");

  RegisterBooleanPref(brave_news::prefs::kBraveNewsOptedIn, true);
  RegisterBooleanPref(brave_news::prefs::kNewTabPageShowToday, true);

  RegisterBooleanPref(
      ntp_background_images::prefs::kNewTabPageShowBackgroundImage, true);
  RegisterBooleanPref(ntp_background_images::prefs::
                          kNewTabPageShowSponsoredImagesBackgroundImage,
                      true);

  RegisterBooleanPref(prefs::kOptedInToNotificationAds, true);
  RegisterInt64Pref(prefs::kMaximumNotificationAdsPerHour, -1);

  RegisterBooleanPref(prefs::kShouldAllowSubdivisionTargeting, false);
  RegisterStringPref(prefs::kSubdivisionTargetingSubdivision, "AUTO");
  RegisterStringPref(prefs::kSubdivisionTargetingAutoDetectedSubdivision, "");

  RegisterStringPref(prefs::kCatalogId, "");
  RegisterIntegerPref(prefs::kCatalogVersion, 0);
  RegisterInt64Pref(prefs::kCatalogPing, 7'200'000);
  RegisterTimePref(prefs::kCatalogLastUpdated, base::Time());

  RegisterInt64Pref(prefs::kIssuerPing, 0);
  RegisterListPref(prefs::kIssuers);

  RegisterDictPref(prefs::kEpsilonGreedyBanditArms);
  RegisterListPref(prefs::kEpsilonGreedyBanditEligibleSegments);

  RegisterListPref(prefs::kNotificationAds);
  RegisterTimePref(prefs::kServeAdAt, Now());

  RegisterTimePref(prefs::kNextTokenRedemptionAt, DistantFuture());

  RegisterBooleanPref(prefs::kHasMigratedClientState, true);
  RegisterBooleanPref(prefs::kHasMigratedConfirmationState, true);
  RegisterBooleanPref(prefs::kHasMigratedConversionState, true);
  RegisterBooleanPref(prefs::kHasMigratedNotificationState, true);
  RegisterBooleanPref(prefs::kHasMigratedRewardsState, true);
  RegisterBooleanPref(prefs::kShouldMigrateVerifiedRewardsUser, false);

  RegisterStringPref(prefs::kBrowserVersionNumber, "");

  RegisterStringPref(brave_l10n::prefs::kCountryCode,
                     brave_l10n::GetDefaultISOCountryCodeString());
}

bool HasRegisteredPrefPath(const std::string& path) {
  return base::Contains(Prefs(), GetUuidForCurrentTestAndValue(path));
}

}  // namespace brave_ads
