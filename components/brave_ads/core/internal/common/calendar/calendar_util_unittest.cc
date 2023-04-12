/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/calendar/calendar_util.h"

#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace brave_ads {

TEST(BatAdsCalendarUtilTest, GetLastDayOfMonth) {
  // Arrange
  const int year = 2021;

  const int last_day_for_month[12] = {31, 28, 31, 30, 31, 30,
                                      31, 31, 30, 31, 30, 31};

  // Act
  for (int i = 0; i < 12; i++) {
    const int last_day_of_month = GetLastDayOfMonth(year, i + 1);
    const int expected_last_day_of_month = last_day_for_month[i];
    EXPECT_EQ(expected_last_day_of_month, last_day_of_month);
  }

  // Assert
}

TEST(BatAdsCalendarUtilTest, GetLastDayOfMonthForLeapYear) {
  // Arrange
  const int year = 2020;

  const int last_day_for_month[12] = {31, 29, 31, 30, 31, 30,
                                      31, 31, 30, 31, 30, 31};

  // Act
  for (int i = 0; i < 12; i++) {
    const int last_day_of_month = GetLastDayOfMonth(year, i + 1);
    const int expected_last_day_of_month = last_day_for_month[i];
    EXPECT_EQ(expected_last_day_of_month, last_day_of_month);
  }

  // Assert
}

TEST(BatAdsCalendarUtilTest, GetDayOfWeekForYearMonthAndDay) {
  // Arrange
  const int year = 2020;
  const int month = 2;
  const int day = 29;

  // Act
  const int day_of_week = GetDayOfWeek(year, month, day);

  // Assert
  EXPECT_EQ(6, day_of_week);
}

TEST(BatAdsCalendarUtilTest, GetDayOfWeek) {
  // Arrange
  const base::Time time =
      TimeFromString("November 18 1970", /*is_local*/ false);

  // Act
  const int day_of_week = GetDayOfWeek(time, /*is_local*/ false);

  // Assert
  EXPECT_EQ(3, day_of_week);
}

}  // namespace brave_ads
