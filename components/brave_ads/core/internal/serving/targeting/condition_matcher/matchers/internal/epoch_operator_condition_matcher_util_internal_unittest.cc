/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/targeting/condition_matcher/matchers/internal/epoch_operator_condition_matcher_util_internal.h"

#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/common/test/time_test_util.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsOperatorConditionMatcherUtilInternalTest : public test::TestBase {
};

TEST_F(BraveAdsOperatorConditionMatcherUtilInternalTest,
       DoNotParseNegativeDays) {
  // Act & Assert
  EXPECT_FALSE(ParseDays("[=]:-1"));
}

TEST_F(BraveAdsOperatorConditionMatcherUtilInternalTest, ParseDayZero) {
  // Act & Assert
  EXPECT_EQ(0, ParseDays("[=]:0"));
}

TEST_F(BraveAdsOperatorConditionMatcherUtilInternalTest, ParseDays) {
  // Act & Assert
  EXPECT_EQ(7, ParseDays("[=]:7"));
}

TEST_F(BraveAdsOperatorConditionMatcherUtilInternalTest,
       DoNotParseNonIntegerDays) {
  // Act & Assert
  EXPECT_FALSE(ParseDays("[=]:1.5"));
}

TEST_F(BraveAdsOperatorConditionMatcherUtilInternalTest,
       DoNotParseMalformedDays) {
  // Act & Assert
  EXPECT_FALSE(ParseDays("[=]: 7 "));
}

TEST_F(BraveAdsOperatorConditionMatcherUtilInternalTest,
       DoNotParseInvalidDays) {
  // Act & Assert
  EXPECT_FALSE(ParseDays("[=]:seven"));
}

TEST_F(BraveAdsOperatorConditionMatcherUtilInternalTest, IsUnixEpochTimestamp) {
  // Act & Assert
  EXPECT_TRUE(IsUnixEpochTimestamp(0));
  EXPECT_TRUE(IsUnixEpochTimestamp(2147483647));
}

TEST_F(BraveAdsOperatorConditionMatcherUtilInternalTest,
       IsNotUnixEpochTimestamp) {
  // Act & Assert
  EXPECT_FALSE(IsUnixEpochTimestamp(-1));
  EXPECT_FALSE(IsUnixEpochTimestamp(2147483648));
  EXPECT_FALSE(IsUnixEpochTimestamp(
      13372214400000000 /* 1st October 2024 00:00:00 UTC */));
}

TEST_F(BraveAdsOperatorConditionMatcherUtilInternalTest, WindowsToUnixEpoch) {
  // Arrange
  AdvanceClockTo(test::TimeFromUTCString("3 October 2024"));

  // Act & Assert
  EXPECT_EQ(1727740800,
            WindowsToUnixEpoch(
                13372214400000000 /* 1st October 2024 00:00:00 UTC */));
}

TEST_F(BraveAdsOperatorConditionMatcherUtilInternalTest,
       TimeDeltaSinceUnixEpoch) {
  // Arrange
  AdvanceClockTo(test::TimeFromUTCString("3 October 2024"));

  // Act & Assert
  EXPECT_EQ(base::Days(2),
            TimeDeltaSinceEpoch(1727740800 /*1st October 2024 00:00:00 UTC*/));
}

TEST_F(BraveAdsOperatorConditionMatcherUtilInternalTest,
       TimeDeltaSinceWindowsEpoch) {
  // Arrange
  AdvanceClockTo(test::TimeFromUTCString("3 October 2024"));

  // Act & Assert
  EXPECT_EQ(base::Days(2),
            TimeDeltaSinceEpoch(
                13372214400000000 /*1st October 2024 00:00:00.000 UTC*/));
}

TEST_F(BraveAdsOperatorConditionMatcherUtilInternalTest,
       ParseWindowsEpochTimeDelta) {
  // Arrange
  AdvanceClockTo(test::TimeFromUTCString("3 October 2024"));

  // Act & Assert
  EXPECT_EQ(
      base::Days(2),
      ParseTimeDelta("13372214400000000" /*1st October 2024 00:00:00 UTC*/));
}

TEST_F(BraveAdsOperatorConditionMatcherUtilInternalTest,
       ParseUnixEpochWithFractionalSecondsTimeDelta) {
  // Arrange
  AdvanceClockTo(test::TimeFromUTCString("3 October 2024"));

  // Act & Assert
  EXPECT_EQ(
      base::Days(2),
      ParseTimeDelta("1727740800.3237710" /*1st October 2024 00:00:00 UTC*/));
}

TEST_F(BraveAdsOperatorConditionMatcherUtilInternalTest,
       ParseUnixEpochTimeDelta) {
  // Arrange
  AdvanceClockTo(test::TimeFromUTCString("3 October 2024"));

  // Act & Assert
  EXPECT_EQ(base::Days(2),
            ParseTimeDelta("1727740800" /*1st October 2024 00:00:00 UTC*/));
}

TEST_F(BraveAdsOperatorConditionMatcherUtilInternalTest, DoNotParseTimeDelta) {
  // Act & Assert
  EXPECT_FALSE(ParseTimeDelta("broken time"));
}

}  // namespace brave_ads
