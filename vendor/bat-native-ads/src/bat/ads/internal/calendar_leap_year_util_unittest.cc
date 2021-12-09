/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/calendar_leap_year_util.h"

#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

TEST(BatAdsCalendarLeapYearUtilTest, IsLeapYear) {
  // Arrange
  const int leap_years[] = {2000, 2004, 2008, 2012, 2016, 2020, 2024,
                            2028, 2032, 2036, 2040, 2044, 2048};

  // Act
  for (const int year : leap_years) {
    const bool is_leap_year = IsLeapYear(year);
    EXPECT_TRUE(is_leap_year);
  }

  // Assert
}

TEST(BatAdsCalendarLeapYearUtilTest, IsCommonYear) {
  // Arrange

  // Act
  for (int year = 2000; year < 2050; year++) {
    if (year % 4 == 0) {
      ASSERT_TRUE(IsLeapYear(year));
      continue;
    }

    const bool is_leap_year = IsLeapYear(year);
    EXPECT_FALSE(is_leap_year);
  }

  // Assert
}

}  // namespace ads
