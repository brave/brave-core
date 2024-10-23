/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/public/serving/new_tab_page_ad_serving_condition_matcher_util.h"

#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/ads_client/ads_client_pref_provider.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/common/test/time_test_util.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsNewTabPageAdServingConditionMatcherUtilTest
    : public test::TestBase {
 public:
  BraveAdsNewTabPageAdServingConditionMatcherUtilTest() : pref_provider_() {
    // We need to set the clock to 00:00:00 UTC here to ensure the pref registry
    // in `common/test/pref_registry_test_util.cc` is initialized correctly with
    // deterministic default values.
    AdvanceClockTo(test::TimeFromUTCString("14 October 2024 00:00:00"));
  }

 protected:
  const AdsClientPrefProvider pref_provider_;
};

TEST_F(BraveAdsNewTabPageAdServingConditionMatcherUtilTest,
       MatchEmptyConditions) {
  // Act & Assert
  EXPECT_TRUE(MatchConditions(&pref_provider_, /*condition_matchers=*/{}));
}

TEST_F(BraveAdsNewTabPageAdServingConditionMatcherUtilTest,
       MatchConditionsIfAllConditionsAreTrue) {
  // Arrange
  const NewTabPageAdConditionMatcherMap condition_matchers = {
      {prefs::kSubdivisionTargetingUserSelectedSubdivision, "AUTO"},
      {prefs::kOptedInToNotificationAds, "1"}};

  // Act & Assert
  EXPECT_TRUE(MatchConditions(&pref_provider_, condition_matchers));
}

TEST_F(BraveAdsNewTabPageAdServingConditionMatcherUtilTest,
       MatchEqualOperatorCondition) {
  // Arrange
  const NewTabPageAdConditionMatcherMap condition_matchers = {
      {prefs::kServeAdAt, "[=]:7"}};

  AdvanceClockBy(base::Days(7));

  // Act & Assert
  EXPECT_TRUE(MatchConditions(&pref_provider_, condition_matchers));
}

TEST_F(BraveAdsNewTabPageAdServingConditionMatcherUtilTest,
       DoNotMatchEqualOperatorCondition) {
  // Arrange
  const NewTabPageAdConditionMatcherMap condition_matchers = {
      {prefs::kServeAdAt, "[=]:7"}};

  AdvanceClockBy(base::Days(7) - base::Milliseconds(1));

  // Act & Assert
  EXPECT_FALSE(MatchConditions(&pref_provider_, condition_matchers));
}

TEST_F(BraveAdsNewTabPageAdServingConditionMatcherUtilTest,
       MatchPatternCondition) {
  // Arrange
  const NewTabPageAdConditionMatcherMap condition_matchers = {
      {prefs::kSubdivisionTargetingUserSelectedSubdivision,
       "?UT*"}};  // Value is "AUTO" in the pref.

  // Act & Assert
  EXPECT_TRUE(MatchConditions(&pref_provider_, condition_matchers));
}

TEST_F(BraveAdsNewTabPageAdServingConditionMatcherUtilTest,
       DoNotMatchPatternCondition) {
  // Arrange
  const NewTabPageAdConditionMatcherMap condition_matchers = {
      {prefs::kSubdivisionTargetingUserSelectedSubdivision,
       "?FOO*"}};  // Value is "AUTO" in the pref.

  // Act & Assert
  EXPECT_FALSE(MatchConditions(&pref_provider_, condition_matchers));
}

TEST_F(BraveAdsNewTabPageAdServingConditionMatcherUtilTest,
       MatchRegexCondition) {
  // Arrange
  const NewTabPageAdConditionMatcherMap condition_matchers = {
      {prefs::kSubdivisionTargetingUserSelectedSubdivision,
       "^AU"}};  // Value is "AUTO" in the pref.

  // Act & Assert
  EXPECT_TRUE(MatchConditions(&pref_provider_, condition_matchers));
}

TEST_F(BraveAdsNewTabPageAdServingConditionMatcherUtilTest,
       DoNotMatchRegexCondition) {
  // Arrange
  const NewTabPageAdConditionMatcherMap condition_matchers = {
      {prefs::kSubdivisionTargetingUserSelectedSubdivision,
       "^FOO"}};  // Value is "AUTO" in the pref.

  // Act & Assert
  EXPECT_FALSE(MatchConditions(&pref_provider_, condition_matchers));
}

TEST_F(BraveAdsNewTabPageAdServingConditionMatcherUtilTest,
       DoNotMatchConditionsIfPrefPathWasNotFound) {
  // Arrange
  const NewTabPageAdConditionMatcherMap condition_matchers = {
      {"foo.bar", "baz"}};

  // Act & Assert
  EXPECT_FALSE(MatchConditions(&pref_provider_, condition_matchers));
}

TEST_F(BraveAdsNewTabPageAdServingConditionMatcherUtilTest,
       DoNotMatchConditionsIfAllConditionsAreFalse) {
  // Arrange
  const NewTabPageAdConditionMatcherMap condition_matchers = {
      {prefs::kOptedInToNotificationAds, "0"},  // Value is "1" in the pref.
      {prefs::kServeAdAt, "[>]:7"}};            // 5 days ago in the pref.

  AdvanceClockBy(base::Days(5));

  // Act & Assert
  EXPECT_FALSE(MatchConditions(&pref_provider_, condition_matchers));
}

}  // namespace brave_ads
