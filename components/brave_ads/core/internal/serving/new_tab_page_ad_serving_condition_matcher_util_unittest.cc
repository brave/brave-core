/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/public/serving/new_tab_page_ad_serving_condition_matcher_util.h"

#include "brave/components/brave_ads/core/internal/ads_client/ads_client_pref_provider.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsNewTabPageAdServingConditionMatcherUtilTest
    : public test::TestBase {
 protected:
  const AdsClientPrefProvider pref_provider_;
};

TEST_F(BraveAdsNewTabPageAdServingConditionMatcherUtilTest,
       MatchEmptyConditions) {
  // Act & Assert
  EXPECT_TRUE(MatchConditions(&pref_provider_, /*condition_matchers=*/{}));
}

TEST_F(BraveAdsNewTabPageAdServingConditionMatcherUtilTest, MatchConditions) {
  // Arrange
  const NewTabPageAdConditionMatchers condition_matchers = {
      {prefs::kSubdivisionTargetingUserSelectedSubdivision, "AUTO"},
      {prefs::kOptedInToNotificationAds, "1"}};

  // Act & Assert
  EXPECT_TRUE(MatchConditions(&pref_provider_, condition_matchers));
}

TEST_F(BraveAdsNewTabPageAdServingConditionMatcherUtilTest,
       DoNotMatchCondition) {
  // Arrange
  const NewTabPageAdConditionMatchers condition_matchers = {
      {prefs::kSubdivisionTargetingUserSelectedSubdivision, "US"},
      {prefs::kOptedInToNotificationAds, "1"}};

  // Act & Assert
  EXPECT_FALSE(MatchConditions(&pref_provider_, condition_matchers));
}

TEST_F(BraveAdsNewTabPageAdServingConditionMatcherUtilTest,
       DoNotMatchConditions) {
  // Arrange
  const NewTabPageAdConditionMatchers condition_matchers = {
      {prefs::kSubdivisionTargetingUserSelectedSubdivision, "US"},
      {prefs::kOptedInToNotificationAds, "0"}};

  // Act & Assert
  EXPECT_FALSE(MatchConditions(&pref_provider_, condition_matchers));
}

}  // namespace brave_ads
