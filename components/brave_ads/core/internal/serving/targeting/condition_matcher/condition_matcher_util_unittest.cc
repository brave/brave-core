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

TEST_F(BraveAdsConditionMatcherUtilTest,
       DoNotMatchConditionsIfAnyConditionIsFalse) {
  // Arrange
  test::RegisterProfileStringPref("foo", "baz");
  test::RegisterProfileIntegerPref("bar", 1);

  const ConditionMatcherMap condition_matchers = {{"foo", "baz"}, {"bar", "0"}};

  // Act & Assert
  VerifyDoesNotMatchConditionsExpectation(condition_matchers);
}

TEST_F(BraveAdsConditionMatcherUtilTest, MatchEpochEqualOperatorCondition) {
  // Arrange
  test::RegisterProfileTimePref("foo", base::Time::Now());
  AdvanceClockBy(base::Days(7));

  const ConditionMatcherMap condition_matchers = {{"foo", "[T=]:7"}};

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

  const ConditionMatcherMap condition_matchers = {{"foo", "[T=]:7"}};

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

// "M" has no glob wildcard and no regex-specific syntax, so it must only
// ever mean "the value is exactly M", never "the value contains M
// somewhere", even though RE2::PartialMatch would otherwise find it as an
// unanchored substring of "Mac OS X".
TEST_F(BraveAdsConditionMatcherUtilTest,
       DoNotMatchBareLiteralConditionViaLooseRegexFallback) {
  // Arrange.
  test::RegisterProfileStringPref("foo", "Mac OS X");

  const ConditionMatcherMap condition_matchers = {{"foo", "M"}};

  // Act & Assert
  VerifyDoesNotMatchConditionsExpectation(condition_matchers);
}

TEST_F(BraveAdsConditionMatcherUtilTest, MatchBareLiteralConditionExactly) {
  // Arrange
  test::RegisterProfileStringPref("foo", "Mac OS X");

  const ConditionMatcherMap condition_matchers = {{"foo", "Mac OS X"}};

  // Act & Assert
  VerifyDoesMatchConditionsExpectation(condition_matchers);
}

// "^1\.*" has a "*", but it's a regex quantifier on the escaped dot before
// it, not a glob wildcard. The regex-only syntax ("^"/"\") must still take
// priority, even though a wildcard character is also present; treating
// this as glob-only would require the value to literally start with the
// characters "^1\.", which it never will.
TEST_F(BraveAdsConditionMatcherUtilTest,
       MatchRegexConditionContainingWildcardAsQuantifier) {
  // Arrange.
  test::RegisterProfileStringPref("foo", "1.1.95.0");

  const ConditionMatcherMap condition_matchers = {{"foo", "^1\\.*"}};

  // Act & Assert
  VerifyDoesMatchConditionsExpectation(condition_matchers);
}

TEST_F(BraveAdsConditionMatcherUtilTest,
       MatchPatternConditionContainingDotAgainstValueStartingWithIt) {
  // Arrange
  test::RegisterProfileStringPref("foo", "1.81.99");

  const ConditionMatcherMap condition_matchers = {{"foo", "1.*"}};

  // Act & Assert
  VerifyDoesMatchConditionsExpectation(condition_matchers);
}

// "1.*" is a valid glob pattern (starts with "1.", then anything), but
// "152.1.95.0" doesn't start with "1." so the glob interpretation correctly
// fails. The regex fallback must not still match it just because "1.*" is
// also parseable as a loose, unanchored regex that happens to find "1."
// later in the string.
TEST_F(BraveAdsConditionMatcherUtilTest,
       DoNotMatchPatternConditionContainingDotViaLooseRegexFallback) {
  // Arrange.
  test::RegisterProfileStringPref("foo", "152.1.95.0");

  const ConditionMatcherMap condition_matchers = {{"foo", "1.*"}};

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
      {"foo", "0"},  // Value is "1" in the pref.
      {"bar", "[T>]:7"}};

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
  ON_CALL(ads_client_mock_, GetVirtualPrefs).WillByDefault([]() {
    return base::DictValue().Set("[virtual]:foo", 2.0);
  });

  const ConditionMatcherMap condition_matchers = {
      {"foo", "[R<]:[virtual]:foo"}};

  // Act & Assert
  VerifyDoesMatchConditionsExpectation(condition_matchers);
}

TEST_F(BraveAdsConditionMatcherUtilTest,
       DoNotMatchNumericalOperatorWithVirtualPrefPathOperand) {
  // Arrange
  ON_CALL(ads_client_mock_, GetVirtualPrefs).WillByDefault([]() {
    return base::DictValue().Set("[virtual]:foo", 1.0);
  });

  const ConditionMatcherMap condition_matchers = {
      {"foo", "[R>]:[virtual]:foo"}};

  // Act & Assert
  VerifyDoesNotMatchConditionsExpectation(condition_matchers);
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
  ON_CALL(ads_client_mock_, GetVirtualPrefs).WillByDefault([]() {
    return base::DictValue().Set("[virtual]:foo", "bar");
  });

  const ConditionMatcherMap condition_matchers = {
      {"foo", "[R=]:[virtual]:foo"}};

  // Act & Assert
  VerifyDoesNotMatchConditionsExpectation(condition_matchers);
}

TEST_F(BraveAdsConditionMatcherUtilTest,
       MatchNumericalOperatorConditionWithTimePeriodStorage) {
  // Arrange
  ON_CALL(ads_client_mock_, GetVirtualPrefs).WillByDefault([]() {
    return base::DictValue().Set(
        "[virtual]:foo",
        base::DictValue().Set(
            "bar",
            base::ListValue()
                .Append(base::DictValue()
                            .Set("day", test::Now().InSecondsFSinceUnixEpoch())
                            .Set("value", 5.0))
                .Append(base::DictValue()
                            .Set("day",
                                 test::DistantPast().InSecondsFSinceUnixEpoch())
                            .Set("value", 3.0))));
  });

  const ConditionMatcherMap condition_matchers = {
      {"[virtual]:foo|bar|time_period_storage=7d", "[R>]:4"}};

  // Act & Assert
  VerifyDoesMatchConditionsExpectation(condition_matchers);
}

TEST_F(BraveAdsConditionMatcherUtilTest,
       DoNotMatchNumericalOperatorConditionWithTimePeriodStorage) {
  // Arrange
  ON_CALL(ads_client_mock_, GetVirtualPrefs).WillByDefault([]() {
    return base::DictValue().Set(
        "[virtual]:foo",
        base::DictValue().Set(
            "bar",
            base::ListValue()
                .Append(base::DictValue()
                            .Set("day", test::Now().InSecondsFSinceUnixEpoch())
                            .Set("value", 5.0))
                .Append(base::DictValue()
                            .Set("day",
                                 test::DistantPast().InSecondsFSinceUnixEpoch())
                            .Set("value", 3.0))));
  });

  const ConditionMatcherMap condition_matchers = {
      {"[virtual]:foo|bar|time_period_storage=7d", "[R>]:6"}};

  // Act & Assert
  VerifyDoesNotMatchConditionsExpectation(condition_matchers);
}

TEST_F(BraveAdsConditionMatcherUtilTest,
       MatchNumericalOperatorConditionWithTimePeriodStorageOperand) {
  // Arrange
  ON_CALL(ads_client_mock_, GetVirtualPrefs).WillByDefault([]() {
    return base::DictValue().Set(
        "[virtual]:foo",
        base::DictValue()
            .Set("bar",
                 base::ListValue()
                     .Append(
                         base::DictValue()
                             .Set("day", test::Now().InSecondsFSinceUnixEpoch())
                             .Set("value", 5.0))
                     .Append(base::DictValue()
                                 .Set("day", test::DistantPast()
                                                 .InSecondsFSinceUnixEpoch())
                                 .Set("value", 3.0)))
            .Set("baz",
                 base::ListValue()
                     .Append(
                         base::DictValue()
                             .Set("day", test::Now().InSecondsFSinceUnixEpoch())
                             .Set("value", 3.0))
                     .Append(base::DictValue()
                                 .Set("day", test::DistantPast()
                                                 .InSecondsFSinceUnixEpoch())
                                 .Set("value", 5.0))));
  });

  const ConditionMatcherMap condition_matchers = {
      {"[virtual]:foo|bar|time_period_storage=7d",
       "[R>]:[virtual]:foo|baz|time_period_storage=7d"}};

  // Act & Assert
  VerifyDoesMatchConditionsExpectation(condition_matchers);
}

TEST_F(BraveAdsConditionMatcherUtilTest,
       MatchConditionReturnsInvalidForMalformedEpochOperator) {
  // Arrange
  test::RegisterProfileTimePref("foo", base::Time::Now());

  // Act & Assert
  EXPECT_EQ(
      ConditionMatchResult::kInvalid,
      MatchCondition(GetAdsClient().GetVirtualPrefs(), "foo", "[T=]: 7 "));
}

TEST_F(BraveAdsConditionMatcherUtilTest,
       MatchConditionReturnsInvalidForUnresolvableNumericalOperand) {
  // Act & Assert
  EXPECT_EQ(ConditionMatchResult::kInvalid,
            MatchCondition(GetAdsClient().GetVirtualPrefs(), "foo",
                           "[R=]:[virtual]:bar"));
}

TEST_F(BraveAdsConditionMatcherUtilTest,
       MatchConditionReturnsInvalidForNonNumericMatchedValue) {
  // Arrange
  test::RegisterProfileStringPref("foo", "not_a_number");

  // Act & Assert
  EXPECT_EQ(ConditionMatchResult::kInvalid,
            MatchCondition(GetAdsClient().GetVirtualPrefs(), "foo", "[R=]:5"));
}

// "[R>=]:5" isn't a recognized operator prefix. Only a single operator
// character is supported, e.g. "[R≥]:5", so this must not silently fall
// through to the Pattern/Regex matcher.
TEST_F(BraveAdsConditionMatcherUtilTest,
       MatchConditionReturnsInvalidForMalformedNumericalOperatorPrefix) {
  // Arrange
  test::RegisterProfileIntegerPref("foo", 5);

  // Act & Assert
  EXPECT_EQ(ConditionMatchResult::kInvalid,
            MatchCondition(GetAdsClient().GetVirtualPrefs(), "foo", "[R>=]:5"));
}

TEST_F(BraveAdsConditionMatcherUtilTest,
       MatchConditionReturnsInvalidForMalformedEpochOperatorPrefix) {
  // Arrange
  test::RegisterProfileTimePref("foo", base::Time::Now());

  // Act & Assert
  EXPECT_EQ(ConditionMatchResult::kInvalid,
            MatchCondition(GetAdsClient().GetVirtualPrefs(), "foo", "[T>=]:3"));
}

TEST_F(BraveAdsConditionMatcherUtilTest,
       MatchConditionsFoldsInvalidConditionsToFalse) {
  // Explicit regression test: `MatchConditions` (the plural, real ad-serving
  // path) must treat an invalid/malformed condition exactly like a
  // legitimate non-match, never as a match.
  test::RegisterProfileStringPref("foo", "not_a_number");

  const ConditionMatcherMap condition_matchers = {{"foo", "[R=]:5"}};

  // Act & Assert
  VerifyDoesNotMatchConditionsExpectation(condition_matchers);
}

TEST_F(BraveAdsConditionMatcherUtilTest,
       DoNotMatchNumericalOperatorConditionWithTimePeriodStorageOperand) {
  // Arrange
  ON_CALL(ads_client_mock_, GetVirtualPrefs).WillByDefault([]() {
    return base::DictValue().Set(
        "[virtual]:foo",
        base::DictValue()
            .Set("bar",
                 base::ListValue()
                     .Append(
                         base::DictValue()
                             .Set("day", test::Now().InSecondsFSinceUnixEpoch())
                             .Set("value", 5.0))
                     .Append(base::DictValue()
                                 .Set("day", test::DistantPast()
                                                 .InSecondsFSinceUnixEpoch())
                                 .Set("value", 3.0)))
            .Set("baz",
                 base::ListValue()
                     .Append(
                         base::DictValue()
                             .Set("day", test::Now().InSecondsFSinceUnixEpoch())
                             .Set("value", 3.0))
                     .Append(base::DictValue()
                                 .Set("day", test::DistantPast()
                                                 .InSecondsFSinceUnixEpoch())
                                 .Set("value", 5.0))));
  });

  const ConditionMatcherMap condition_matchers = {
      {"[virtual]:foo|baz|time_period_storage=7d",
       "[R>]:[virtual]:foo|bar|time_period_storage=7d"}};

  // Act & Assert
  VerifyDoesNotMatchConditionsExpectation(condition_matchers);
}

}  // namespace brave_ads
