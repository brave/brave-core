/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifdef UNSAFE_BUFFERS_BUILD
// TODO(https://github.com/brave/brave-browser/issues/41661): Remove this and
// convert code to safer constructs.
#pragma allow_unsafe_buffers
#endif

#include "brave/components/brave_ads/core/internal/common/calendar/calendar_util.h"

#include "base/time/time.h"  // IWYU pragma: keep
#include "brave/components/brave_ads/core/internal/common/test/time_test_util.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsCalendarUtilTest, IsLeapYear) {
  // Act & Assert
  for (int year = 2000; year < 2050; ++year) {
    EXPECT_EQ(year % 4 == 0, IsLeapYear(year));
  }
}

TEST(BraveAdsCalendarUtilTest, LocalDayOfWeek) {
  // Act & Assert
  EXPECT_EQ(/*wednesday*/ 3, DayOfWeek(test::TimeFromString("18 November 1970"),
                                       /*is_local*/ true));
}

TEST(BraveAdsCalendarUtilTest, UTCDayOfWeek) {
  // Act & Assert
  EXPECT_EQ(/*monday*/ 1, DayOfWeek(test::TimeFromUTCString("18 November 1991"),
                                    /*is_local*/ false));
}

TEST(BraveAdsCalendarUtilTest, DaysInMonth) {
  // Arrange
  constexpr int kLastDayInMonth[12] = {31, 28, 31, 30, 31, 30,
                                       31, 31, 30, 31, 30, 31};

  // Act & Assert
  for (int i = 0; i < 12; ++i) {
    EXPECT_EQ(kLastDayInMonth[i], DaysInMonth(/*year=*/2021, /*month=*/i + 1));
  }
}

TEST(BraveAdsCalendarUtilTest, DaysInMonthForLeapYear) {
  // Arrange
  constexpr int kDaysInMonth[12] = {31, 29, 31, 30, 31, 30,
                                    31, 31, 30, 31, 30, 31};

  // Act & Assert
  for (int i = 0; i < 12; ++i) {
    EXPECT_EQ(kDaysInMonth[i], DaysInMonth(/*year=*/2020, /*month=*/i + 1));
  }
}

}  // namespace brave_ads
