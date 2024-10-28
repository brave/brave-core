/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/public/serving/targeting/condition_matcher/condition_matcher_util.h"

#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/ads_client/ads_client_pref_provider.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/common/test/time_test_util.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsConditionMatcherUtilTest : public test::TestBase {
 public:
  BraveAdsConditionMatcherUtilTest() : pref_provider_() {
    // We need to set the clock to 00:00:00 UTC here to ensure the pref registry
    // in `common/test/pref_registry_test_util.cc` is initialized correctly with
    // deterministic default values.
    AdvanceClockTo(test::TimeFromUTCString("14 October 2024 00:00:00"));
  }

 protected:
  const AdsClientPrefProvider pref_provider_;
};

TEST_F(BraveAdsConditionMatcherUtilTest, MatchEmptyConditions) {
  // Act & Assert
  EXPECT_TRUE(MatchConditions(&pref_provider_, /*condition_matchers=*/{}));
}

TEST_F(BraveAdsConditionMatcherUtilTest,
       MatchConditionsIfAllConditionsAreTrue) {
  // Arrange
  const ConditionMatcherMap condition_matchers = {
      {prefs::kSubdivisionTargetingUserSelectedSubdivision, "AUTO"},
      {prefs::kOptedInToNotificationAds, "1"}};

  // Act & Assert
  EXPECT_TRUE(MatchConditions(&pref_provider_, condition_matchers));
}

TEST_F(BraveAdsConditionMatcherUtilTest, MatchEqualOperatorCondition) {
  // Arrange
  const ConditionMatcherMap condition_matchers = {{prefs::kServeAdAt, "[=]:7"}};

  AdvanceClockBy(base::Days(7));

  // Act & Assert
  EXPECT_TRUE(MatchConditions(&pref_provider_, condition_matchers));
}

TEST_F(BraveAdsConditionMatcherUtilTest, DoNotMatchEqualOperatorCondition) {
  // Arrange
  const ConditionMatcherMap condition_matchers = {{prefs::kServeAdAt, "[=]:7"}};

  AdvanceClockBy(base::Days(7) - base::Milliseconds(1));

  // Act & Assert
  EXPECT_FALSE(MatchConditions(&pref_provider_, condition_matchers));
}

TEST_F(BraveAdsConditionMatcherUtilTest, MatchPatternCondition) {
  // Arrange
  const ConditionMatcherMap condition_matchers = {
      {prefs::kSubdivisionTargetingUserSelectedSubdivision,
       "?UT*"}};  // Value is "AUTO" in the pref.

  // Act & Assert
  EXPECT_TRUE(MatchConditions(&pref_provider_, condition_matchers));
}

TEST_F(BraveAdsConditionMatcherUtilTest, DoNotMatchPatternCondition) {
  // Arrange
  const ConditionMatcherMap condition_matchers = {
      {prefs::kSubdivisionTargetingUserSelectedSubdivision,
       "?FOO*"}};  // Value is "AUTO" in the pref.

  // Act & Assert
  EXPECT_FALSE(MatchConditions(&pref_provider_, condition_matchers));
}

TEST_F(BraveAdsConditionMatcherUtilTest, MatchRegexCondition) {
  // Arrange
  const ConditionMatcherMap condition_matchers = {
      {prefs::kSubdivisionTargetingUserSelectedSubdivision,
       "^AU"}};  // Value is "AUTO" in the pref.

  // Act & Assert
  EXPECT_TRUE(MatchConditions(&pref_provider_, condition_matchers));
}

TEST_F(BraveAdsConditionMatcherUtilTest, DoNotMatchRegexCondition) {
  // Arrange
  const ConditionMatcherMap condition_matchers = {
      {prefs::kSubdivisionTargetingUserSelectedSubdivision,
       "^FOO"}};  // Value is "AUTO" in the pref.

  // Act & Assert
  EXPECT_FALSE(MatchConditions(&pref_provider_, condition_matchers));
}

TEST_F(BraveAdsConditionMatcherUtilTest,
       DoNotMatchConditionsIfPrefPathWasNotFound) {
  // Arrange
  const ConditionMatcherMap condition_matchers = {{"foo.bar", "baz"}};

  // Act & Assert
  EXPECT_FALSE(MatchConditions(&pref_provider_, condition_matchers));
}

TEST_F(BraveAdsConditionMatcherUtilTest,
       MatchConditionsWithNotOperatorWhenPrefPathNotFound) {
  // Arrange
  const ConditionMatcherMap condition_matchers = {{"[!]:foo.bar", "baz"}};

  // Act & Assert
  EXPECT_TRUE(MatchConditions(&pref_provider_, condition_matchers));
}

TEST_F(BraveAdsConditionMatcherUtilTest,
       DoNotMatchConditionsIfAllConditionsAreFalse) {
  // Arrange
  const ConditionMatcherMap condition_matchers = {
      {prefs::kOptedInToNotificationAds, "0"},  // Value is "1" in the pref.
      {prefs::kServeAdAt, "[>]:7"}};            // 5 days ago in the pref.

  AdvanceClockBy(base::Days(5));

  // Act & Assert
  EXPECT_FALSE(MatchConditions(&pref_provider_, condition_matchers));
}

}  // namespace brave_ads
