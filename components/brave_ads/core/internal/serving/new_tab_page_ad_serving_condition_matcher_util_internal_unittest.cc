/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/new_tab_page_ad_serving_condition_matcher_util_internal.h"

#include "base/containers/span.h"
#include "base/values.h"
#include "brave/components/brave_ads/core/internal/ads_client/ads_client_pref_provider.h"
#include "brave/components/brave_ads/core/internal/common/test/internal/local_state_pref_registry_test_util_internal.h"
#include "brave/components/brave_ads/core/internal/common/test/internal/profile_pref_registry_test_util_internal.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/common/test/time_test_util.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsNewTabPageAdServingConditionMatcherUtilInternalTest
    : public test::TestBase {
 protected:
  const AdsClientPrefProvider pref_provider_;
};

TEST_F(BraveAdsNewTabPageAdServingConditionMatcherUtilInternalTest,
       DoNotConvertNoneValueTypeToString) {
  // Act & Assert
  EXPECT_FALSE(ToString(base::Value()));
}

TEST_F(BraveAdsNewTabPageAdServingConditionMatcherUtilInternalTest,
       BooleanValueTypeToString) {
  // Act & Assert
  EXPECT_EQ("0", ToString(base::Value(false)));
  EXPECT_EQ("1", ToString(base::Value(true)));
}

TEST_F(BraveAdsNewTabPageAdServingConditionMatcherUtilInternalTest,
       IntegerValueTypeToString) {
  // Act & Assert
  EXPECT_EQ("123", ToString(base::Value(123)));
}

TEST_F(BraveAdsNewTabPageAdServingConditionMatcherUtilInternalTest,
       DoubleValueTypeToString) {
  // Act & Assert
  EXPECT_EQ("1.23", ToString(base::Value(1.23)));
}

TEST_F(BraveAdsNewTabPageAdServingConditionMatcherUtilInternalTest,
       StringValueTypeToString) {
  // Act & Assert
  EXPECT_EQ("123", ToString(base::Value("123")));
}

TEST_F(BraveAdsNewTabPageAdServingConditionMatcherUtilInternalTest,
       DoNotConvertDictValueTypeToString) {
  // Act & Assert
  EXPECT_FALSE(ToString(base::Value(base::Value::Dict().Set("foo", "bar"))));
}

TEST_F(BraveAdsNewTabPageAdServingConditionMatcherUtilInternalTest,
       DoNotConvertListValueTypeToString) {
  // Act & Assert
  EXPECT_FALSE(ToString(base::Value(base::Value::List().Append("foo"))));
}

TEST_F(BraveAdsNewTabPageAdServingConditionMatcherUtilInternalTest,
       DoNotConvertBinaryValueTypeToString) {
  // Arrange
  const base::span<const uint8_t> binary({0x48, 0x65, 0x6C, 0x6C, 0x6F, 0x2C,
                                          0x20, 0x57, 0x6F, 0x72, 0x6C, 0x64,
                                          0x21});

  // Act & Assert
  EXPECT_FALSE(ToString(base::Value(binary)));
}

TEST_F(BraveAdsNewTabPageAdServingConditionMatcherUtilInternalTest,
       DoNotParseNegativeDays) {
  // Act & Assert
  EXPECT_FALSE(ParseDays("[=]:-1"));
}

TEST_F(BraveAdsNewTabPageAdServingConditionMatcherUtilInternalTest,
       ParseDayZero) {
  // Act & Assert
  EXPECT_EQ(0, ParseDays("[=]:0"));
}

TEST_F(BraveAdsNewTabPageAdServingConditionMatcherUtilInternalTest, ParseDays) {
  // Act & Assert
  EXPECT_EQ(7, ParseDays("[=]:7"));
}

TEST_F(BraveAdsNewTabPageAdServingConditionMatcherUtilInternalTest,
       DoNotParseNonIntegerDays) {
  // Act & Assert
  EXPECT_FALSE(ParseDays("[=]:1.5"));
}

TEST_F(BraveAdsNewTabPageAdServingConditionMatcherUtilInternalTest,
       DoNotParseMalformedDays) {
  // Act & Assert
  EXPECT_FALSE(ParseDays("[=]: 7 "));
}

TEST_F(BraveAdsNewTabPageAdServingConditionMatcherUtilInternalTest,
       DoNotParseInvalidDays) {
  // Act & Assert
  EXPECT_FALSE(ParseDays("[=]:seven"));
}

TEST_F(BraveAdsNewTabPageAdServingConditionMatcherUtilInternalTest,
       IsUnixEpochTimestamp) {
  // Act & Assert
  EXPECT_TRUE(IsUnixEpochTimestamp(0));
  EXPECT_TRUE(IsUnixEpochTimestamp(2147483647));
}

TEST_F(BraveAdsNewTabPageAdServingConditionMatcherUtilInternalTest,
       IsNotUnixEpochTimestamp) {
  // Act & Assert
  EXPECT_FALSE(IsUnixEpochTimestamp(-1));
  EXPECT_FALSE(IsUnixEpochTimestamp(2147483648));
  EXPECT_FALSE(IsUnixEpochTimestamp(
      13372214400000000 /* 1st October 2024 00:00:00 UTC */));
}

TEST_F(BraveAdsNewTabPageAdServingConditionMatcherUtilInternalTest,
       TimeDeltaSinceUnixEpoch) {
  // Arrange
  AdvanceClockTo(test::TimeFromUTCString("3 October 2024"));

  // Act & Assert
  EXPECT_EQ(base::Days(2),
            TimeDeltaSinceEpoch(1727740800 /*1st October 2024 00:00:00 UTC*/));
}

TEST_F(BraveAdsNewTabPageAdServingConditionMatcherUtilInternalTest,
       TimeDeltaSinceWindowsEpoch) {
  // Arrange
  AdvanceClockTo(test::TimeFromUTCString("3 October 2024"));

  // Act & Assert
  EXPECT_EQ(base::Days(2),
            TimeDeltaSinceEpoch(
                13372214400000000 /*1st October 2024 00:00:00.000 UTC*/));
}

TEST_F(BraveAdsNewTabPageAdServingConditionMatcherUtilInternalTest,
       DoNotMatchIfNotAnOperator) {
  // Act & Assert
  EXPECT_FALSE(MatchOperator(
      "13372214400000000" /*1st October 2024 00:00:00 UTC*/, "baz"));
}

TEST_F(BraveAdsNewTabPageAdServingConditionMatcherUtilInternalTest,
       DoNotMatchMalformedOperator) {
  // Act & Assert
  EXPECT_FALSE(MatchOperator(
      "13372214400000000" /*1st October 2024 00:00:00 UTC*/, "[=]: 7 "));
}

TEST_F(BraveAdsNewTabPageAdServingConditionMatcherUtilInternalTest,
       MatchEqualOperator) {
  // Arrange
  AdvanceClockTo(test::TimeFromUTCString("3 October 2024"));

  // Act & Assert
  EXPECT_TRUE(
      MatchOperator("13372214400000000" /*1st October 2024 00:00:00 UTC*/,
                    "[=]:2"));  // Event occurred 2 days ago.
}

TEST_F(BraveAdsNewTabPageAdServingConditionMatcherUtilInternalTest,
       DoNotMatchEqualOperator) {
  // Arrange
  AdvanceClockTo(test::TimeFromUTCString("3 October 2024"));

  // Act & Assert
  EXPECT_FALSE(
      MatchOperator("13372214400000000" /*1st October 2024 00:00:00 UTC*/,
                    "[=]:3"));  // Event occurred 2 days ago.
}

TEST_F(BraveAdsNewTabPageAdServingConditionMatcherUtilInternalTest,
       MatchGreaterThanOperator) {
  // Arrange
  AdvanceClockTo(test::TimeFromUTCString("3 October 2024"));

  // Act & Assert
  EXPECT_TRUE(
      MatchOperator("13372214400000000" /*1st October 2024 00:00:00 UTC*/,
                    "[>]:1"));  // Event occurred 2 days ago.
}

TEST_F(BraveAdsNewTabPageAdServingConditionMatcherUtilInternalTest,
       DoNotMatchGreaterThanOperator) {
  // Arrange
  AdvanceClockTo(test::TimeFromUTCString("3 October 2024"));

  // Act & Assert
  EXPECT_FALSE(
      MatchOperator("13372214400000000" /*1st October 2024 00:00:00 UTC*/,
                    "[>]:2"));  // Event occurred 2 days ago.
}

TEST_F(BraveAdsNewTabPageAdServingConditionMatcherUtilInternalTest,
       MatchGreaterThanOrEqualOperator) {
  // Arrange
  AdvanceClockTo(test::TimeFromUTCString("3 October 2024"));

  // Act & Assert
  EXPECT_TRUE(
      MatchOperator("13372214400000000" /*1st October 2024 00:00:00 UTC*/,
                    "[≥]:1"));  // Event occurred 2 days ago.
  EXPECT_TRUE(
      MatchOperator("13372214400000000" /*1st October 2024 00:00:00 UTC*/,
                    "[≥]:2"));  // Event occurred 2 days ago.
}

TEST_F(BraveAdsNewTabPageAdServingConditionMatcherUtilInternalTest,
       DoNotMatchGreaterThanOrEqualOperator) {
  // Arrange
  AdvanceClockTo(test::TimeFromUTCString("3 October 2024"));

  // Act & Assert
  EXPECT_FALSE(
      MatchOperator("13372214400000000" /*1st October 2024 00:00:00 UTC*/,
                    "[≥]:3"));  // Event occurred 2 days ago.
}

TEST_F(BraveAdsNewTabPageAdServingConditionMatcherUtilInternalTest,
       MatchLessThanOperator) {
  // Arrange
  AdvanceClockTo(test::TimeFromUTCString("3 October 2024"));

  // Act & Assert
  EXPECT_TRUE(
      MatchOperator("13372214400000000" /*1st October 2024 00:00:00 UTC*/,
                    "[<]:3"));  // Event occurred 2 days ago.
}

TEST_F(BraveAdsNewTabPageAdServingConditionMatcherUtilInternalTest,
       DoNotMatchLessThanOperator) {
  // Arrange
  AdvanceClockTo(test::TimeFromUTCString("3 October 2024"));

  // Act & Assert
  EXPECT_FALSE(
      MatchOperator("13372214400000000" /*1st October 2024 00:00:00 UTC*/,
                    "[<]:2"));  // Event occurred 2 days ago.
}

TEST_F(BraveAdsNewTabPageAdServingConditionMatcherUtilInternalTest,
       MatchLessThanOrEqualOperator) {
  // Arrange
  AdvanceClockTo(test::TimeFromUTCString("3 October 2024"));

  // Act & Assert
  EXPECT_TRUE(
      MatchOperator("13372214400000000" /*1st October 2024 00:00:00 UTC*/,
                    "[≤]:3"));  // Event occurred 2 days ago.
  EXPECT_TRUE(
      MatchOperator("13372214400000000" /*1st October 2024 00:00:00 UTC*/,
                    "[≤]:2"));  // Event occurred 2 days ago.
}

TEST_F(BraveAdsNewTabPageAdServingConditionMatcherUtilInternalTest,
       DoNotMatchLessThanOrEqualOperator) {
  // Arrange
  AdvanceClockTo(test::TimeFromUTCString("3 October 2024"));

  // Act & Assert
  EXPECT_FALSE(
      MatchOperator("13372214400000000" /*1st October 2024 00:00:00 UTC*/,
                    "[≤]:1"));  // Event occurred 2 days ago.
}

TEST_F(BraveAdsNewTabPageAdServingConditionMatcherUtilInternalTest,
       DoNotMatchUnknownOperator) {
  // Arrange
  AdvanceClockTo(test::TimeFromUTCString("3 October 2024"));

  // Act & Assert
  EXPECT_FALSE(MatchOperator(
      "13372214400000000" /*1st October 2024 00:00:00 UTC*/, "[_]:2"));
}

TEST_F(BraveAdsNewTabPageAdServingConditionMatcherUtilInternalTest,
       MatchRegex) {
  // Act & Assert
  EXPECT_TRUE(MatchRegex("foo.baz.bar", "(foo|bar)"));
}

TEST_F(BraveAdsNewTabPageAdServingConditionMatcherUtilInternalTest,
       MatchEmptyRegex) {
  // Act & Assert
  EXPECT_TRUE(MatchRegex("", ""));
}

TEST_F(BraveAdsNewTabPageAdServingConditionMatcherUtilInternalTest,
       DoNotMatchRegex) {
  // Act & Assert
  EXPECT_FALSE(MatchRegex("foo.baz.bar", "(waldo|fred)"));
}

TEST_F(BraveAdsNewTabPageAdServingConditionMatcherUtilInternalTest,
       DoNotMatchMalformedRegex) {
  // Act & Assert
  EXPECT_FALSE(MatchRegex("foo.baz.bar", "* ?"));
}

TEST_F(BraveAdsNewTabPageAdServingConditionMatcherUtilInternalTest,
       MatchPattern) {
  // Act & Assert
  EXPECT_TRUE(MatchPattern("foo.baz.bar", "foo?baz.*"));
}

TEST_F(BraveAdsNewTabPageAdServingConditionMatcherUtilInternalTest,
       MatchEmptyPattern) {
  // Act & Assert
  EXPECT_TRUE(MatchPattern("", ""));
}

TEST_F(BraveAdsNewTabPageAdServingConditionMatcherUtilInternalTest,
       MatchEscapedPattern) {
  // Act & Assert
  EXPECT_TRUE(MatchRegex(R"(*.bar.?)", R"(\*.bar.\?)"));
}

TEST_F(BraveAdsNewTabPageAdServingConditionMatcherUtilInternalTest,
       DoNotMatchPattern) {
  // Act & Assert
  EXPECT_FALSE(MatchRegex("foo.baz.bar", "bar.*.foo"));
}

TEST_F(BraveAdsNewTabPageAdServingConditionMatcherUtilInternalTest,
       GetVirtualPrefValue) {
  // Arrange
  ON_CALL(ads_client_mock_, GetVirtualPrefs).WillByDefault([]() {
    return base::Value::Dict().Set("[virtual]:matrix", /*room*/ 303);
  });

  // Act & Assert
  EXPECT_EQ(base::Value(/*room*/ 303),
            MaybeGetPrefValue(&pref_provider_, "[virtual]:matrix"));
}

TEST_F(BraveAdsNewTabPageAdServingConditionMatcherUtilInternalTest,
       DoNotGetUnknownVirtualPrefValue) {
  // Arrange
  ON_CALL(ads_client_mock_, GetVirtualPrefs).WillByDefault([]() {
    return base::Value::Dict().Set("[virtual]:inverse.matrices", /*room*/ 101);
  });

  // Act & Assert
  EXPECT_FALSE(MaybeGetPrefValue(&pref_provider_, "[virtual]:matrix"));
}

TEST_F(BraveAdsNewTabPageAdServingConditionMatcherUtilInternalTest,
       GetBooleanProfilePrefValue) {
  // Arrange
  test::RegisterProfileBooleanPref("boolean", true);

  // Act & Assert
  EXPECT_EQ(base::Value(true), MaybeGetPrefValue(&pref_provider_, "boolean"));
}

TEST_F(BraveAdsNewTabPageAdServingConditionMatcherUtilInternalTest,
       GetIntegerProfilePrefValue) {
  // Arrange
  test::RegisterProfileIntegerPref("integer", 123);

  // Act & Assert
  EXPECT_EQ(base::Value(123), MaybeGetPrefValue(&pref_provider_, "integer"));
}

TEST_F(BraveAdsNewTabPageAdServingConditionMatcherUtilInternalTest,
       GetDoubleProfilePrefValue) {
  // Arrange
  test::RegisterProfileDoublePref("double", 1.23);

  // Act & Assert
  EXPECT_EQ(base::Value(1.23), MaybeGetPrefValue(&pref_provider_, "double"));
}

TEST_F(BraveAdsNewTabPageAdServingConditionMatcherUtilInternalTest,
       GetStringProfilePrefValue) {
  // Arrange
  test::RegisterProfileStringPref("string", "foo");

  // Act & Assert
  EXPECT_EQ(base::Value("foo"), MaybeGetPrefValue(&pref_provider_, "string"));
}

TEST_F(BraveAdsNewTabPageAdServingConditionMatcherUtilInternalTest,
       GetDictProfilePrefValue) {
  // Arrange
  test::RegisterProfileDictPref("dict", base::Value::Dict().Set("foo", "bar"));

  // Act & Assert
  EXPECT_EQ(base::Value("bar"), MaybeGetPrefValue(&pref_provider_, "dict|foo"));
}

TEST_F(BraveAdsNewTabPageAdServingConditionMatcherUtilInternalTest,
       GetNestedDictProfilePrefValue) {
  // Arrange
  test::RegisterProfileDictPref(
      "dict",
      base::Value::Dict().Set("foo", base::Value::Dict().Set("bar", "qux")));

  // Act & Assert
  EXPECT_EQ(base::Value("qux"),
            MaybeGetPrefValue(&pref_provider_, "dict|foo|bar"));
}

TEST_F(BraveAdsNewTabPageAdServingConditionMatcherUtilInternalTest,
       GetDeeplyNestedDictProfilePrefValue) {
  // Arrange
  test::RegisterProfileDictPref(
      "dict", base::Value::Dict().Set(
                  "foo", base::Value::List().Append("bar").Append(
                             base::Value::Dict().Set("baz", "qux"))));

  // Act & Assert
  EXPECT_EQ(base::Value("qux"),
            MaybeGetPrefValue(&pref_provider_, "dict|foo|1|baz"));
}

TEST_F(BraveAdsNewTabPageAdServingConditionMatcherUtilInternalTest,
       GetNestedDictProfilePrefValueWithDotSeparatedPathComponents) {
  // Arrange
  test::RegisterProfileDictPref(
      "dict", base::Value::Dict().Set(
                  "foo.bar", base::Value::Dict().Set("baz.qux", "quux")));

  // Act & Assert
  EXPECT_EQ(base::Value("quux"),
            MaybeGetPrefValue(&pref_provider_, "dict|foo.bar|baz.qux"));
}

TEST_F(BraveAdsNewTabPageAdServingConditionMatcherUtilInternalTest,
       DoNotGetMalformedDictProfilePrefValue) {
  test::RegisterProfileDictPref("dict",
                                base::Value::Dict().Set("foo.bar", "baz"));

  // Act & Assert
  EXPECT_FALSE(MaybeGetPrefValue(&pref_provider_, "dict|foo.bar|baz"));
  EXPECT_FALSE(MaybeGetPrefValue(&pref_provider_, "dict|"));
  EXPECT_FALSE(MaybeGetPrefValue(&pref_provider_, "|"));
  EXPECT_FALSE(MaybeGetPrefValue(&pref_provider_, ""));
}

TEST_F(BraveAdsNewTabPageAdServingConditionMatcherUtilInternalTest,
       GetListProfilePrefValue) {
  // Arrange
  test::RegisterProfileListPref(
      "list", base::Value::List().Append("foo").Append("bar"));

  // Act & Assert
  EXPECT_EQ(base::Value("bar"), MaybeGetPrefValue(&pref_provider_, "list|1"));
}

TEST_F(BraveAdsNewTabPageAdServingConditionMatcherUtilInternalTest,
       GetNestedListProfilePrefValue) {
  // Arrange
  test::RegisterProfileListPref(
      "list", base::Value::List().Append(
                  base::Value::List().Append("foo").Append("bar")));

  // Act & Assert
  EXPECT_EQ(base::Value("bar"), MaybeGetPrefValue(&pref_provider_, "list|0|1"));
}

TEST_F(BraveAdsNewTabPageAdServingConditionMatcherUtilInternalTest,
       GetDeeplyNestedListProfilePrefValue) {
  // Arrange
  test::RegisterProfileListPref(
      "list", base::Value::List().Append(base::Value::Dict().Set(
                  "foo", base::Value::List().Append("bar").Append("baz"))));

  // Act & Assert
  EXPECT_EQ(base::Value("baz"),
            MaybeGetPrefValue(&pref_provider_, "list|0|foo|1"));
}

TEST_F(BraveAdsNewTabPageAdServingConditionMatcherUtilInternalTest,
       GetNestedListProfilePrefValueWithDotSeparatedPathComponents) {
  // Arrange
  test::RegisterProfileListPref(
      "list", base::Value::List().Append(base::Value::Dict().Set(
                  "foo.bar",
                  base::Value::List().Append("baz.qux").Append("quux.corge"))));

  // Act & Assert
  EXPECT_EQ(base::Value("quux.corge"),
            MaybeGetPrefValue(&pref_provider_, "list|0|foo.bar|1"));
}

TEST_F(BraveAdsNewTabPageAdServingConditionMatcherUtilInternalTest,
       DoNotGetListProfilePrefValueWithOutOfBoundsListIndicies) {
  // Arrange
  test::RegisterProfileListPref(
      "list", base::Value::List().Append("foo").Append("bar"));

  // Act & Assert
  EXPECT_FALSE(MaybeGetPrefValue(&pref_provider_, "list|-1"));
  EXPECT_FALSE(MaybeGetPrefValue(&pref_provider_, "list|2"));
}

TEST_F(BraveAdsNewTabPageAdServingConditionMatcherUtilInternalTest,
       DoNotGetMalformedListProfilePrefValue) {
  test::RegisterProfileListPref("list", base::Value::List().Append("foo"));

  // Act & Assert
  EXPECT_FALSE(MaybeGetPrefValue(&pref_provider_, "list|0|foo"));
  EXPECT_FALSE(MaybeGetPrefValue(&pref_provider_, "list|bar"));
  EXPECT_FALSE(MaybeGetPrefValue(&pref_provider_, "list|"));
  EXPECT_FALSE(MaybeGetPrefValue(&pref_provider_, "|"));
  EXPECT_FALSE(MaybeGetPrefValue(&pref_provider_, ""));
}

TEST_F(BraveAdsNewTabPageAdServingConditionMatcherUtilInternalTest,
       GetBooleanLocalStatePrefValue) {
  // Arrange
  test::RegisterLocalStateBooleanPref("boolean", true);

  // Act & Assert
  EXPECT_EQ(base::Value(true), MaybeGetPrefValue(&pref_provider_, "boolean"));
}

TEST_F(BraveAdsNewTabPageAdServingConditionMatcherUtilInternalTest,
       GetIntegerLocalStatePrefValue) {
  // Arrange
  test::RegisterLocalStateIntegerPref("integer", 123);

  // Act & Assert
  EXPECT_EQ(base::Value(123), MaybeGetPrefValue(&pref_provider_, "integer"));
}

TEST_F(BraveAdsNewTabPageAdServingConditionMatcherUtilInternalTest,
       GetDoubleLocalStatePrefValue) {
  // Arrange
  test::RegisterLocalStateDoublePref("double", 1.23);

  // Act & Assert
  EXPECT_EQ(base::Value(1.23), MaybeGetPrefValue(&pref_provider_, "double"));
}

TEST_F(BraveAdsNewTabPageAdServingConditionMatcherUtilInternalTest,
       GetStringLocalStatePrefValue) {
  // Arrange
  test::RegisterLocalStateStringPref("string", "foo");

  // Act & Assert
  EXPECT_EQ(base::Value("foo"), MaybeGetPrefValue(&pref_provider_, "string"));
}

TEST_F(BraveAdsNewTabPageAdServingConditionMatcherUtilInternalTest,
       GetDictLocalStatePrefValue) {
  // Arrange
  test::RegisterLocalStateDictPref("dict",
                                   base::Value::Dict().Set("foo.bar", "baz"));

  // Act & Assert
  EXPECT_EQ(base::Value("baz"),
            MaybeGetPrefValue(&pref_provider_, "dict|foo.bar"));
}

TEST_F(BraveAdsNewTabPageAdServingConditionMatcherUtilInternalTest,
       GetNestedDictLocalStatePrefValue) {
  // Arrange
  test::RegisterLocalStateDictPref(
      "dict",
      base::Value::Dict().Set("foo", base::Value::Dict().Set("bar", "qux")));

  // Act & Assert
  EXPECT_EQ(base::Value("qux"),
            MaybeGetPrefValue(&pref_provider_, "dict|foo|bar"));
}

TEST_F(BraveAdsNewTabPageAdServingConditionMatcherUtilInternalTest,
       GetDeeplyNestedDictLocalStatePrefValue) {
  // Arrange
  test::RegisterLocalStateDictPref(
      "dict", base::Value::Dict().Set(
                  "foo", base::Value::List().Append("bar").Append(
                             base::Value::Dict().Set("baz", "qux"))));

  // Act & Assert
  EXPECT_EQ(base::Value("qux"),
            MaybeGetPrefValue(&pref_provider_, "dict|foo|1|baz"));
}

TEST_F(BraveAdsNewTabPageAdServingConditionMatcherUtilInternalTest,
       GetNestedDictLocalStatePrefValueWithDotSeparatedPathComponents) {
  // Arrange
  test::RegisterLocalStateDictPref(
      "dict", base::Value::Dict().Set(
                  "foo.bar", base::Value::Dict().Set("baz.qux", "quux")));

  // Act & Assert
  EXPECT_EQ(base::Value("quux"),
            MaybeGetPrefValue(&pref_provider_, "dict|foo.bar|baz.qux"));
}

TEST_F(BraveAdsNewTabPageAdServingConditionMatcherUtilInternalTest,
       DoNotGetMalformedDictLocalStatePrefValue) {
  test::RegisterLocalStateDictPref("dict",
                                   base::Value::Dict().Set("foo.bar", "baz"));

  // Act & Assert
  EXPECT_FALSE(MaybeGetPrefValue(&pref_provider_, "dict|foo.bar|baz"));
  EXPECT_FALSE(MaybeGetPrefValue(&pref_provider_, "dict|"));
  EXPECT_FALSE(MaybeGetPrefValue(&pref_provider_, "|"));
  EXPECT_FALSE(MaybeGetPrefValue(&pref_provider_, ""));
}

TEST_F(BraveAdsNewTabPageAdServingConditionMatcherUtilInternalTest,
       GetListLocalStatePrefValue) {
  // Arrange
  test::RegisterLocalStateListPref(
      "list", base::Value::List().Append("foo").Append("bar"));

  // Act & Assert
  EXPECT_EQ(base::Value("bar"), MaybeGetPrefValue(&pref_provider_, "list|1"));
}

TEST_F(BraveAdsNewTabPageAdServingConditionMatcherUtilInternalTest,
       GetNestedListLocalStatePrefValue) {
  // Arrange
  test::RegisterLocalStateListPref(
      "list", base::Value::List().Append(
                  base::Value::List().Append("foo").Append("bar")));

  // Act & Assert
  EXPECT_EQ(base::Value("bar"), MaybeGetPrefValue(&pref_provider_, "list|0|1"));
}

TEST_F(BraveAdsNewTabPageAdServingConditionMatcherUtilInternalTest,
       GetDeeplyNestedListLocalStatePrefValue) {
  // Arrange
  test::RegisterLocalStateListPref(
      "list", base::Value::List().Append(base::Value::Dict().Set(
                  "foo", base::Value::List().Append("bar").Append("baz"))));

  // Act & Assert
  EXPECT_EQ(base::Value("baz"),
            MaybeGetPrefValue(&pref_provider_, "list|0|foo|1"));
}

TEST_F(BraveAdsNewTabPageAdServingConditionMatcherUtilInternalTest,
       GetNestedListLocalStatePrefValueWithDotSeparatedPathComponents) {
  // Arrange
  test::RegisterLocalStateListPref(
      "list", base::Value::List().Append(base::Value::Dict().Set(
                  "foo.bar",
                  base::Value::List().Append("baz.qux").Append("quux.corge"))));

  // Act & Assert
  EXPECT_EQ(base::Value("quux.corge"),
            MaybeGetPrefValue(&pref_provider_, "list|0|foo.bar|1"));
}

TEST_F(BraveAdsNewTabPageAdServingConditionMatcherUtilInternalTest,
       DoNotGetMalformedListLocalStatePrefValue) {
  // Arrange
  test::RegisterLocalStateListPref("list", base::Value::List().Append("foo"));

  // Act & Assert
  EXPECT_FALSE(MaybeGetPrefValue(&pref_provider_, "list|0|foo"));
  EXPECT_FALSE(MaybeGetPrefValue(&pref_provider_, "list|bar"));
  EXPECT_FALSE(MaybeGetPrefValue(&pref_provider_, "list|"));
  EXPECT_FALSE(MaybeGetPrefValue(&pref_provider_, "|"));
  EXPECT_FALSE(MaybeGetPrefValue(&pref_provider_, ""));
}

TEST_F(BraveAdsNewTabPageAdServingConditionMatcherUtilInternalTest,
       DoNotGetListLocalStatePrefValueWithOutOfBoundsListIndicies) {
  // Arrange
  test::RegisterLocalStateListPref(
      "list", base::Value::List().Append("foo").Append("bar"));

  // Act & Assert
  EXPECT_FALSE(MaybeGetPrefValue(&pref_provider_, "list|-1"));
  EXPECT_FALSE(MaybeGetPrefValue(&pref_provider_, "list|2"));
}

TEST_F(BraveAdsNewTabPageAdServingConditionMatcherUtilInternalTest,
       DoNotGetUnknownPrefValue) {
  // Act & Assert
  EXPECT_FALSE(MaybeGetPrefValue(&pref_provider_, "foo.bar"));
}

TEST_F(BraveAdsNewTabPageAdServingConditionMatcherUtilInternalTest,
       GetProfilePrefValueAsString) {
  // Arrange
  test::RegisterProfileBooleanPref("boolean", true);

  // Act & Assert
  EXPECT_EQ("1", MaybeGetPrefValueAsString(&pref_provider_, "boolean"));
}

TEST_F(BraveAdsNewTabPageAdServingConditionMatcherUtilInternalTest,
       GetLocalStatePrefValueAsString) {
  // Arrange
  test::RegisterLocalStateBooleanPref("boolean", true);

  // Act & Assert
  EXPECT_EQ("1", MaybeGetPrefValueAsString(&pref_provider_, "boolean"));
}

TEST_F(BraveAdsNewTabPageAdServingConditionMatcherUtilInternalTest,
       DoNotGetUnknownPrefValueAsString) {
  // Act & Assert
  EXPECT_FALSE(MaybeGetPrefValueAsString(&pref_provider_, "foo.bar"));
  EXPECT_FALSE(MaybeGetPrefValueAsString(&pref_provider_, ""));
}

}  // namespace brave_ads
