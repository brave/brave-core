/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/targeting/condition_matcher/condition_matcher_util.h"

#include "base/time/time.h"
#include "base/values.h"
#include "brave/components/brave_ads/core/internal/ads_client/ads_client_util.h"
#include "brave/components/brave_ads/core/internal/common/test/profile_pref_registry_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/profile_pref_value_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/common/test/time_test_util.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {

void VerifyDoesMatchConditionsExpectation(
    const ConditionMatcherMap& condition_matchers) {
  EXPECT_TRUE(
      MatchConditions(GetAdsClient().GetVirtualPrefs(), condition_matchers));
}

void VerifyDoesNotMatchConditionsExpectation(
    const ConditionMatcherMap& condition_matchers) {
  EXPECT_FALSE(
      MatchConditions(GetAdsClient().GetVirtualPrefs(), condition_matchers));
}

}  // namespace

class BraveAdsConditionMatcherUtilTest : public test::TestBase {
 public:
  BraveAdsConditionMatcherUtilTest() {
    // Set the clock to 00:00:00 UTC to ensure that `base::Time::Now()` returns
    // a deterministic value when registering time prefs in individual tests.
    AdvanceClockTo(test::TimeFromUTCString("14 October 2024 00:00:00"));
  }
};

TEST_F(BraveAdsConditionMatcherUtilTest, MatchEmptyConditions) {
  // Act & Assert
  VerifyDoesMatchConditionsExpectation(/*condition_matchers=*/{});
}

TEST_F(BraveAdsConditionMatcherUtilTest, MatchMultipleConditions) {
  // Arrange
  test::RegisterProfileStringPref("foo", "baz");
  test::RegisterProfileIntegerPref("bar", 1);

  const ConditionMatcherMap condition_matchers = {{"foo", "baz"}, {"bar", "1"}};

  // Act & Assert
  VerifyDoesMatchConditionsExpectation(condition_matchers);
}

TEST_F(BraveAdsConditionMatcherUtilTest, MatchEpochEqualOperatorCondition) {
  // Arrange
  test::RegisterProfileTimePref("foo", base::Time::Now());
  AdvanceClockBy(base::Days(7));

  const ConditionMatcherMap condition_matchers = {
      {"foo",
       "[T=]:7"}};  // Exactly 7 days have elapsed; condition requires = 7.

  // Act & Assert
  VerifyDoesMatchConditionsExpectation(condition_matchers);
}

TEST_F(BraveAdsConditionMatcherUtilTest,
       MatchEpochEqualOperatorConditionForUnknownPrefPath) {
  // Arrange
  const ConditionMatcherMap condition_matchers = {
      {"unknown_pref_path", "[T=]:0"}};

  // Act & Assert
  VerifyDoesMatchConditionsExpectation(condition_matchers);
}

TEST_F(BraveAdsConditionMatcherUtilTest,
       DoNotMatchEpochEqualOperatorConditionForUnknownPrefPath) {
  // Arrange
  const ConditionMatcherMap condition_matchers = {
      {"unknown_pref_path", "[T=]:1"}};

  // Act & Assert
  VerifyDoesNotMatchConditionsExpectation(condition_matchers);
}

TEST_F(BraveAdsConditionMatcherUtilTest,
       DoNotMatchEpochEqualOperatorCondition) {
  // Arrange
  test::RegisterProfileTimePref("foo", base::Time::Now());
  AdvanceClockBy(base::Days(7) - base::Milliseconds(1));

  const ConditionMatcherMap condition_matchers = {
      {"foo",
       "[T=]:7"}};  // Just under 7 days have elapsed; condition requires = 7.

  // Act & Assert
  VerifyDoesNotMatchConditionsExpectation(condition_matchers);
}

TEST_F(BraveAdsConditionMatcherUtilTest,
       MatchNumericalEqualOperatorConditionForUnknownPrefPath) {
  // Arrange
  const ConditionMatcherMap condition_matchers = {
      {"unknown_pref_path", "[R=]:0"}};

  // Act & Assert
  VerifyDoesMatchConditionsExpectation(condition_matchers);
}

TEST_F(BraveAdsConditionMatcherUtilTest,
       DoNotMatchNumericalEqualOperatorConditionForUnknownPrefPath) {
  // Arrange
  const ConditionMatcherMap condition_matchers = {
      {"unknown_pref_path", "[R=]:1"}};

  // Act & Assert
  VerifyDoesNotMatchConditionsExpectation(condition_matchers);
}

TEST_F(BraveAdsConditionMatcherUtilTest, MatchNumericalEqualOperatorCondition) {
  // Arrange
  test::RegisterProfileIntegerPref("foo", 5);
  const ConditionMatcherMap condition_matchers = {{"foo", "[R=]:5"}};

  // Act & Assert
  VerifyDoesMatchConditionsExpectation(condition_matchers);
}

TEST_F(BraveAdsConditionMatcherUtilTest,
       DoNotMatchNumericalEqualOperatorCondition) {
  // Arrange
  test::RegisterProfileIntegerPref("foo", 5);
  const ConditionMatcherMap condition_matchers = {{"foo", "[R=]:4"}};

  // Act & Assert
  VerifyDoesNotMatchConditionsExpectation(condition_matchers);
}

TEST_F(BraveAdsConditionMatcherUtilTest,
       MatchNumericalNotEqualOperatorCondition) {
  // Arrange
  test::RegisterProfileIntegerPref("foo", 5);
  const ConditionMatcherMap condition_matchers = {{"foo", "[R≠]:4"}};

  // Act & Assert
  VerifyDoesMatchConditionsExpectation(condition_matchers);
}

TEST_F(BraveAdsConditionMatcherUtilTest,
       DoNotMatchNumericalNotEqualOperatorCondition) {
  // Arrange
  test::RegisterProfileIntegerPref("foo", 5);
  const ConditionMatcherMap condition_matchers = {{"foo", "[R≠]:5"}};

  // Act & Assert
  VerifyDoesNotMatchConditionsExpectation(condition_matchers);
}

TEST_F(BraveAdsConditionMatcherUtilTest,
       MatchNumericalGreaterThanOperatorCondition) {
  // Arrange
  test::RegisterProfileIntegerPref("foo", 5);
  const ConditionMatcherMap condition_matchers = {{"foo", "[R>]:4"}};

  // Act & Assert
  VerifyDoesMatchConditionsExpectation(condition_matchers);
}

TEST_F(BraveAdsConditionMatcherUtilTest,
       DoNotMatchNumericalGreaterThanOperatorCondition) {
  // Arrange
  test::RegisterProfileIntegerPref("foo", 5);
  const ConditionMatcherMap condition_matchers = {{"foo", "[R>]:5"}};

  // Act & Assert
  VerifyDoesNotMatchConditionsExpectation(condition_matchers);
}

TEST_F(BraveAdsConditionMatcherUtilTest,
       MatchNumericalGreaterThanOrEqualOperatorCondition) {
  // Arrange
  test::RegisterProfileIntegerPref("foo", 5);
  const ConditionMatcherMap condition_matchers = {{"foo", "[R≥]:5"}};

  // Act & Assert
  VerifyDoesMatchConditionsExpectation(condition_matchers);
}

TEST_F(BraveAdsConditionMatcherUtilTest,
       DoNotMatchNumericalGreaterThanOrEqualOperatorCondition) {
  // Arrange
  test::RegisterProfileIntegerPref("foo", 5);
  const ConditionMatcherMap condition_matchers = {{"foo", "[R≥]:6"}};

  // Act & Assert
  VerifyDoesNotMatchConditionsExpectation(condition_matchers);
}

TEST_F(BraveAdsConditionMatcherUtilTest,
       MatchNumericalLessThanOperatorCondition) {
  // Arrange
  test::RegisterProfileIntegerPref("foo", 5);
  const ConditionMatcherMap condition_matchers = {{"foo", "[R<]:6"}};

  // Act & Assert
  VerifyDoesMatchConditionsExpectation(condition_matchers);
}

TEST_F(BraveAdsConditionMatcherUtilTest,
       DoNotMatchNumericalLessThanOperatorCondition) {
  // Arrange
  test::RegisterProfileIntegerPref("foo", 5);
  const ConditionMatcherMap condition_matchers = {{"foo", "[R<]:5"}};

  // Act & Assert
  VerifyDoesNotMatchConditionsExpectation(condition_matchers);
}

TEST_F(BraveAdsConditionMatcherUtilTest,
       MatchNumericalLessThanOrEqualOperatorCondition) {
  // Arrange
  test::RegisterProfileIntegerPref("foo", 5);
  const ConditionMatcherMap condition_matchers = {{"foo", "[R≤]:5"}};

  // Act & Assert
  VerifyDoesMatchConditionsExpectation(condition_matchers);
}

TEST_F(BraveAdsConditionMatcherUtilTest,
       DoNotMatchNumericalLessThanOrEqualOperatorCondition) {
  // Arrange
  test::RegisterProfileIntegerPref("foo", 5);
  const ConditionMatcherMap condition_matchers = {{"foo", "[R≤]:4"}};

  // Act & Assert
  VerifyDoesNotMatchConditionsExpectation(condition_matchers);
}

TEST_F(BraveAdsConditionMatcherUtilTest, MatchPatternCondition) {
  // Arrange
  test::RegisterProfileStringPref("foo", "baz");

  const ConditionMatcherMap condition_matchers = {{"foo", "?az"}};

  // Act & Assert
  VerifyDoesMatchConditionsExpectation(condition_matchers);
}

TEST_F(BraveAdsConditionMatcherUtilTest, DoNotMatchPatternCondition) {
  // Arrange
  test::RegisterProfileStringPref("foo", "baz");

  const ConditionMatcherMap condition_matchers = {{"foo", "?qux*"}};

  // Act & Assert
  VerifyDoesNotMatchConditionsExpectation(condition_matchers);
}

TEST_F(BraveAdsConditionMatcherUtilTest, MatchRegexCondition) {
  // Arrange
  test::RegisterProfileStringPref("foo", "baz");

  const ConditionMatcherMap condition_matchers = {{"foo", "^ba"}};

  // Act & Assert
  VerifyDoesMatchConditionsExpectation(condition_matchers);
}

TEST_F(BraveAdsConditionMatcherUtilTest, DoNotMatchRegexCondition) {
  // Arrange
  test::RegisterProfileStringPref("foo", "baz");

  const ConditionMatcherMap condition_matchers = {{"foo", "^qux"}};

  // Act & Assert
  VerifyDoesNotMatchConditionsExpectation(condition_matchers);
}

TEST_F(BraveAdsConditionMatcherUtilTest,
       DoNotMatchConditionsIfPrefPathWasNotFound) {
  // Arrange
  const ConditionMatcherMap condition_matchers = {{"foo.bar", "baz"}};

  // Act & Assert
  VerifyDoesNotMatchConditionsExpectation(condition_matchers);
}

TEST_F(BraveAdsConditionMatcherUtilTest,
       MatchConditionsWithNotOperatorWhenPrefPathNotFound) {
  // Arrange
  const ConditionMatcherMap condition_matchers = {{"[!]:foo.bar", "baz"}};

  // Act & Assert
  VerifyDoesMatchConditionsExpectation(condition_matchers);
}

TEST_F(BraveAdsConditionMatcherUtilTest,
       DoNotMatchConditionsWithNotOperatorWhenPrefExists) {
  // Arrange
  test::RegisterProfileIntegerPref("foo", 0);
  test::SetProfileIntegerPrefValue("foo", 5);

  const ConditionMatcherMap condition_matchers = {{"[!]:foo", "baz"}};

  // Act & Assert
  VerifyDoesNotMatchConditionsExpectation(condition_matchers);
}

TEST_F(BraveAdsConditionMatcherUtilTest,
       DoNotMatchConditionsWithNotOperatorWhenPrefExistsWithDefaultValue) {
  // Arrange
  test::RegisterProfileIntegerPref("foo", 5);

  const ConditionMatcherMap condition_matchers = {{"[!]:foo", "baz"}};

  // Act & Assert
  VerifyDoesNotMatchConditionsExpectation(condition_matchers);
}

TEST_F(BraveAdsConditionMatcherUtilTest,
       DoNotMatchConditionsIfAllConditionsAreFalse) {
  // Arrange
  test::RegisterProfileIntegerPref("foo", 1);
  test::RegisterProfileTimePref("bar", base::Time::Now());
  AdvanceClockBy(base::Days(5));

  const ConditionMatcherMap condition_matchers = {
      {"foo", "0"},        // Value is "1" in the pref.
      {"bar", "[T>]:7"}};  // Only 5 days have elapsed; condition requires > 7.

  // Act & Assert
  VerifyDoesNotMatchConditionsExpectation(condition_matchers);
}

TEST_F(BraveAdsConditionMatcherUtilTest,
       MatchNumericalOperatorComparingTwoPrefs) {
  // Arrange
  test::RegisterProfileIntegerPref("foo", 5);
  test::RegisterProfileIntegerPref("bar", 3);

  const ConditionMatcherMap condition_matchers = {{"foo", "[R>]:bar"}};

  // Act & Assert
  VerifyDoesMatchConditionsExpectation(condition_matchers);
}

TEST_F(BraveAdsConditionMatcherUtilTest,
       DoNotMatchNumericalOperatorComparingTwoPrefs) {
  // Arrange
  test::RegisterProfileIntegerPref("foo", 3);
  test::RegisterProfileIntegerPref("bar", 5);

  const ConditionMatcherMap condition_matchers = {{"foo", "[R>]:bar"}};

  // Act & Assert
  VerifyDoesNotMatchConditionsExpectation(condition_matchers);
}

TEST_F(BraveAdsConditionMatcherUtilTest,
       MatchNumericalOperatorWithVirtualPrefPathOperand) {
  // Arrange
  base::DictValue virtual_prefs;
  virtual_prefs.Set("[virtual]:foo", 2.0);

  const ConditionMatcherMap condition_matchers = {
      {"foo", "[R<]:[virtual]:foo"}};

  // Act & Assert
  EXPECT_TRUE(MatchConditions(virtual_prefs, condition_matchers));
}

TEST_F(BraveAdsConditionMatcherUtilTest,
       DoNotMatchNumericalOperatorWithVirtualPrefPathOperand) {
  // Arrange
  base::DictValue virtual_prefs;
  virtual_prefs.Set("[virtual]:foo", 1.0);

  const ConditionMatcherMap condition_matchers = {
      {"foo", "[R>]:[virtual]:foo"}};

  // Act & Assert
  EXPECT_FALSE(MatchConditions(virtual_prefs, condition_matchers));
}

TEST_F(BraveAdsConditionMatcherUtilTest,
       DoNotMatchNumericalOperatorWithMissingPrefPathOperand) {
  // Arrange
  const ConditionMatcherMap condition_matchers = {
      {"foo", "[R=]:[virtual]:bar"}};

  // Act & Assert
  VerifyDoesNotMatchConditionsExpectation(condition_matchers);
}

TEST_F(BraveAdsConditionMatcherUtilTest,
       DoNotMatchNumericalOperatorWithNonNumericVirtualPrefPathOperand) {
  // Arrange
  base::DictValue virtual_prefs;
  virtual_prefs.Set("[virtual]:foo", "bar");

  const ConditionMatcherMap condition_matchers = {
      {"foo", "[R=]:[virtual]:foo"}};

  // Act & Assert
  EXPECT_FALSE(MatchConditions(virtual_prefs, condition_matchers));
}

}  // namespace brave_ads
