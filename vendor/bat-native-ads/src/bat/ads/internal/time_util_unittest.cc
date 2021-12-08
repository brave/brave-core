/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/time_util.h"

#include "base/time/time.h"
#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_time_util.h"
#include "bat/ads/internal/unittest_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsTimeUtilTest : public UnitTestBase {
 protected:
  BatAdsTimeUtilTest() = default;

  ~BatAdsTimeUtilTest() override = default;
};

TEST_F(BatAdsTimeUtilTest, GetLocalTimeAsMinutes) {
  // Arrange
  const base::Time& time =
      TimeFromString("November 18 2020 12:34:56", /* is_local */ true);

  // Act
  const int minutes = GetLocalTimeAsMinutes(time);

  // Assert
  const int expected_minutes = (12 * base::Time::kMinutesPerHour) + 34;
  EXPECT_EQ(expected_minutes, minutes);
}

TEST_F(BatAdsTimeUtilTest, AdjustTimeToBeginningOfPreviousMonth) {
  // Arrange
  const base::Time& time =
      TimeFromString("November 18 2020 12:34:56.789", /* is_local */ true);
  AdvanceClock(time);

  // Act
  const base::Time& adjusted_time = AdjustTimeToBeginningOfPreviousMonth(time);

  // Assert
  const base::Time& expected_adjusted_time =
      TimeFromString("October 1 2020 00:00:00.000", /* is_local */ true);

  EXPECT_EQ(expected_adjusted_time, adjusted_time);
}

TEST_F(BatAdsTimeUtilTest, AdjustTimeToBeginningOfPreviousMonthOnCusp) {
  // Arrange
  const base::Time& time =
      TimeFromString("January 1 2020 00:00:00.000", /* is_local */ true);
  AdvanceClock(time);

  // Act
  const base::Time& adjusted_time = AdjustTimeToBeginningOfPreviousMonth(time);

  // Assert
  const base::Time& expected_adjusted_time =
      TimeFromString("December 1 2019 00:00:00.000", /* is_local */ true);

  EXPECT_EQ(expected_adjusted_time, adjusted_time);
}

TEST_F(BatAdsTimeUtilTest, AdjustTimeToEndOfPreviousMonth) {
  // Arrange
  const base::Time& time =
      TimeFromString("November 18 2020 12:34:56.789", /* is_local */ true);
  AdvanceClock(time);

  // Act
  const base::Time& adjusted_time = AdjustTimeToEndOfPreviousMonth(time);

  // Assert
  const base::Time& expected_adjusted_time =
      TimeFromString("October 31 2020 23:59:59.999", /* is_local */ true);

  EXPECT_EQ(expected_adjusted_time, adjusted_time);
}

TEST_F(BatAdsTimeUtilTest, AdjustTimeToEndOfPreviousMonthOnTheCusp) {
  // Arrange
  const base::Time& time =
      TimeFromString("January 1 2020 00:00:00.000", /* is_local */ true);
  AdvanceClock(time);

  // Act
  const base::Time& adjusted_time = AdjustTimeToEndOfPreviousMonth(time);

  // Assert
  const base::Time& expected_adjusted_time =
      TimeFromString("December 31 2019 23:59:59.999", /* is_local */ true);

  EXPECT_EQ(expected_adjusted_time, adjusted_time);
}

TEST_F(BatAdsTimeUtilTest, AdjustTimeToBeginningOfMonth) {
  // Arrange
  const base::Time& time =
      TimeFromString("November 18 2020 12:34:56.789", /* is_local */ true);
  AdvanceClock(time);

  // Act
  const base::Time& adjusted_time = AdjustTimeToBeginningOfMonth(time);

  // Assert
  const base::Time& expected_adjusted_time =
      TimeFromString("November 1 2020 00:00:00.000", /* is_local */ true);

  EXPECT_EQ(expected_adjusted_time, adjusted_time);
}

TEST_F(BatAdsTimeUtilTest, AdjustTimeToEndOfMonth) {
  // Arrange
  const base::Time& time =
      TimeFromString("November 18 2020 12:34:56.789", /* is_local */ true);
  AdvanceClock(time);

  // Act
  const base::Time& adjusted_time = AdjustTimeToEndOfMonth(time);

  // Assert
  const base::Time& expected_adjusted_time =
      TimeFromString("November 30 2020 23:59:59.999", /* is_local */ true);

  EXPECT_EQ(expected_adjusted_time, adjusted_time);
}

TEST_F(BatAdsTimeUtilTest, GetTimeAtBeginningOfLastMonth) {
  // Arrange
  const base::Time& time =
      TimeFromString("November 18 2020 12:34:56.789", /* is_local */ true);
  AdvanceClock(time);

  // Act
  const base::Time& adjusted_time = GetTimeAtBeginningOfLastMonth();

  // Assert
  const base::Time& expected_adjusted_time =
      TimeFromString("October 1 2020 00:00:00.000", /* is_local */ true);

  EXPECT_EQ(expected_adjusted_time, adjusted_time);
}

TEST_F(BatAdsTimeUtilTest, GetTimeAtBeginningOfLastMonthOnTheCusp) {
  // Arrange
  const base::Time& time =
      TimeFromString("January 1 2020 00:00:00.000", /* is_local */ true);
  AdvanceClock(time);

  // Act
  const base::Time& adjusted_time = GetTimeAtBeginningOfLastMonth();

  // Assert
  const base::Time& expected_adjusted_time =
      TimeFromString("December 1 2019 00:00:00.000", /* is_local */ true);

  EXPECT_EQ(expected_adjusted_time, adjusted_time);
}

TEST_F(BatAdsTimeUtilTest, GetTimeAtEndOfLastMonth) {
  // Arrange
  const base::Time& time =
      TimeFromString("November 18 2020 12:34:56.789", /* is_local */ true);
  AdvanceClock(time);

  // Act
  const base::Time& adjusted_time = GetTimeAtEndOfLastMonth();

  // Assert
  const base::Time& expected_adjusted_time =
      TimeFromString("October 31 2020 23:59:59.999", /* is_local */ true);

  EXPECT_EQ(expected_adjusted_time, adjusted_time);
}

TEST_F(BatAdsTimeUtilTest, GetTimeAtEndOfLastMonthOnTheCusp) {
  // Arrange
  const base::Time& time =
      TimeFromString("January 1 2020 00:00:00.000", /* is_local */ true);
  AdvanceClock(time);

  // Act
  const base::Time& adjusted_time = GetTimeAtEndOfLastMonth();

  // Assert
  const base::Time& expected_adjusted_time =
      TimeFromString("December 31 2019 23:59:59.999", /* is_local */ true);

  EXPECT_EQ(expected_adjusted_time, adjusted_time);
}

TEST_F(BatAdsTimeUtilTest, GetTimeAtBeginningOfThisMonth) {
  // Arrange
  const base::Time& time =
      TimeFromString("November 18 2020 12:34:56.789", /* is_local */ true);
  AdvanceClock(time);

  // Act
  const base::Time& adjusted_time = GetTimeAtBeginningOfThisMonth();

  // Assert
  const base::Time& expected_adjusted_time =
      TimeFromString("November 1 2020 00:00:00.000", /* is_local */ true);

  EXPECT_EQ(expected_adjusted_time, adjusted_time);
}

TEST_F(BatAdsTimeUtilTest, GetTimeAtEndOfThisMonth) {
  // Arrange
  const base::Time& time =
      TimeFromString("November 18 2020 12:34:56.789", /* is_local */ true);
  AdvanceClock(time);

  // Act
  const base::Time& adjusted_time = GetTimeAtEndOfThisMonth();

  // Assert
  const base::Time& expected_adjusted_time =
      TimeFromString("November 30 2020 23:59:59.999", /* is_local */ true);

  EXPECT_EQ(expected_adjusted_time, adjusted_time);
}

}  // namespace ads
