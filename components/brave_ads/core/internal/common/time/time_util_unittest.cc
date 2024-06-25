/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/time/time_util.h"

#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_converter_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsTimeUtilTest : public UnitTestBase,
                             public ::testing::WithParamInterface<bool> {};

TEST_P(BraveAdsTimeUtilTest, GetLocalTimeInMinutes) {
  // Arrange
  const base::Time time = TimeFromString("November 18 2020 12:34:56");

  // Act & Assert
  EXPECT_EQ((12 * base::Time::kMinutesPerHour) + 34,
            GetLocalTimeInMinutes(time));
}

TEST_P(BraveAdsTimeUtilTest, AdjustLocalTimeToBeginningOfPreviousMonth) {
  // Arrange
  const base::Time time = TimeFromString("November 18 2020 12:34:56.789");
  AdvanceClockTo(time);

  // Act & Assert
  const base::Time expected_adjusted_time =
      TimeFromString("October 1 2020 00:00:00.000");
  EXPECT_EQ(expected_adjusted_time,
            AdjustLocalTimeToBeginningOfPreviousMonth(time));
}

TEST_P(BraveAdsTimeUtilTest, AdjustLocalTimeToBeginningOfPreviousMonthOnCusp) {
  // Arrange
  const base::Time time = TimeFromString("January 1 2020 00:00:00.000");
  AdvanceClockTo(time);

  // Act & Assert
  const base::Time expected_adjusted_time =
      TimeFromString("December 1 2019 00:00:00.000");
  EXPECT_EQ(expected_adjusted_time,
            AdjustLocalTimeToBeginningOfPreviousMonth(time));
}

TEST_P(BraveAdsTimeUtilTest, AdjustLocalTimeToEndOfPreviousMonth) {
  // Arrange
  const base::Time time = TimeFromString("November 18 2020 12:34:56.789");
  AdvanceClockTo(time);

  // Act & Assert
  const base::Time expected_adjusted_time =
      TimeFromString("October 31 2020 23:59:59.999");
  EXPECT_EQ(expected_adjusted_time, AdjustLocalTimeToEndOfPreviousMonth(time));
}

TEST_P(BraveAdsTimeUtilTest, AdjustLocalTimeToEndOfPreviousMonthOnTheCusp) {
  // Arrange
  const base::Time time = TimeFromString("January 1 2020 00:00:00.000");
  AdvanceClockTo(time);

  // Act & Assert
  const base::Time expected_adjusted_time =
      TimeFromString("December 31 2019 23:59:59.999");
  EXPECT_EQ(expected_adjusted_time, AdjustLocalTimeToEndOfPreviousMonth(time));
}

TEST_P(BraveAdsTimeUtilTest, AdjustLocalTimeToBeginningOfMonth) {
  // Arrange
  const base::Time time = TimeFromString("November 18 2020 12:34:56.789");
  AdvanceClockTo(time);

  // Act & Assert
  const base::Time expected_adjusted_time =
      TimeFromString("November 1 2020 00:00:00.000");
  EXPECT_EQ(expected_adjusted_time, AdjustLocalTimeToBeginningOfMonth(time));
}

TEST_P(BraveAdsTimeUtilTest, AdjustLocalTimeToEndOfMonth) {
  // Arrange
  const base::Time time = TimeFromString("November 18 2020 12:34:56.789");
  AdvanceClockTo(time);

  // Act & Assert
  const base::Time expected_adjusted_time =
      TimeFromString("November 30 2020 23:59:59.999");
  EXPECT_EQ(expected_adjusted_time, AdjustLocalTimeToEndOfMonth(time));
}

TEST_P(BraveAdsTimeUtilTest, GetLocalTimeAtBeginningOfLastMonth) {
  // Arrange
  const base::Time time = TimeFromString("November 18 2020 12:34:56.789");
  AdvanceClockTo(time);

  // Act & Assert
  const base::Time expected_adjusted_time =
      TimeFromString("October 1 2020 00:00:00.000");
  EXPECT_EQ(expected_adjusted_time, GetLocalTimeAtBeginningOfLastMonth());
}

TEST_P(BraveAdsTimeUtilTest, GetLocalTimeAtBeginningOfLastMonthOnTheCusp) {
  // Arrange
  const base::Time time = TimeFromString("January 1 2020 00:00:00.000");
  AdvanceClockTo(time);

  // Act & Assert
  const base::Time expected_adjusted_time =
      TimeFromString("December 1 2019 00:00:00.000");
  EXPECT_EQ(expected_adjusted_time, GetLocalTimeAtBeginningOfLastMonth());
}

TEST_P(BraveAdsTimeUtilTest, GetLocalTimeAtEndOfLastMonth) {
  // Arrange
  const base::Time time = TimeFromString("November 18 2020 12:34:56.789");
  AdvanceClockTo(time);

  // Act & Assert
  const base::Time expected_adjusted_time =
      TimeFromString("October 31 2020 23:59:59.999");
  EXPECT_EQ(expected_adjusted_time, GetLocalTimeAtEndOfLastMonth());
}

TEST_P(BraveAdsTimeUtilTest, GetLocalTimeAtEndOfLastMonthOnTheCusp) {
  // Arrange
  const base::Time time = TimeFromString("January 1 2020 00:00:00.000");
  AdvanceClockTo(time);

  // Act & Assert
  const base::Time expected_adjusted_time =
      TimeFromString("December 31 2019 23:59:59.999");
  EXPECT_EQ(expected_adjusted_time, GetLocalTimeAtEndOfLastMonth());
}

TEST_P(BraveAdsTimeUtilTest, GetLocalTimeAtBeginningOfThisMonth) {
  // Arrange
  const base::Time time = TimeFromString("November 18 2020 12:34:56.789");
  AdvanceClockTo(time);

  // Act & Assert
  const base::Time expected_adjusted_time =
      TimeFromString("November 1 2020 00:00:00.000");
  EXPECT_EQ(expected_adjusted_time, GetLocalTimeAtBeginningOfThisMonth());
}

TEST_P(BraveAdsTimeUtilTest, GetLocalTimeAtEndOfThisMonth) {
  // Arrange
  const base::Time time = TimeFromString("November 18 2020 12:34:56.789");
  AdvanceClockTo(time);

  // Act & Assert
  const base::Time expected_adjusted_time =
      TimeFromString("November 30 2020 23:59:59.999");
  EXPECT_EQ(expected_adjusted_time, GetLocalTimeAtEndOfThisMonth());
}

TEST_P(BraveAdsTimeUtilTest, TimeToPrivacyPreservingIso8601) {
  // Arrange
  const base::Time time = TimeFromUTCString("November 18 2020 23:45:12.345");
  AdvanceClockTo(time);

  // Act & Assert
  EXPECT_EQ("2020-11-18T23:00:00.000Z", TimeToPrivacyPreservingIso8601(Now()));
}

INSTANTIATE_TEST_SUITE_P(, BraveAdsTimeUtilTest, ::testing::Bool());

}  // namespace brave_ads
