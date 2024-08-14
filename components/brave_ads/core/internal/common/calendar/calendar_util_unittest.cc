/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/calendar/calendar_util.h"

#include "base/time/time.h"  // IWYU pragma: keep
#include "brave/components/brave_ads/core/internal/common/test/time_test_util.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsCalendarUtilTest, GetDayOfWeekForYearMonthAndDay) {
  // Act & Assert
  EXPECT_EQ(/*friday*/ 6, DayOfWeek(/*year=*/2020, /*month=*/2, /*day=*/29));
}

TEST(BraveAdsCalendarUtilTest, DayOfWeek) {
  // Act & Assert
  EXPECT_EQ(/*wednesday*/ 3, DayOfWeek(test::TimeFromString("November 18 1970"),
                                       /*is_local=*/true));
}

TEST(BraveAdsCalendarUtilTest, IsLeapYear) {
  // Act & Assert
  for (int year = 2000; year < 2050; ++year) {
    EXPECT_EQ(year % 4 == 0, IsLeapYear(year));
  }
}

TEST(BraveAdsCalendarUtilTest, DaysPerMonth) {
  // Arrange
  constexpr int kLastDayForMonth[12] = {31, 28, 31, 30, 31, 30,
                                        31, 31, 30, 31, 30, 31};

  // Act & Assert
  for (int i = 0; i < 12; ++i) {
    EXPECT_EQ(kLastDayForMonth[i],
              DaysPerMonth(/*year=*/2021, /*month=*/i + 1));
  }
}

TEST(BraveAdsCalendarUtilTest, DaysPerMonthForLeapYear) {
  // Arrange
  constexpr int kDaysPerMonth[12] = {31, 29, 31, 30, 31, 30,
                                     31, 31, 30, 31, 30, 31};

  // Act & Assert
  for (int i = 0; i < 12; ++i) {
    EXPECT_EQ(kDaysPerMonth[i], DaysPerMonth(/*year=*/2020, /*month=*/i + 1));
  }
}

}  // namespace brave_ads
