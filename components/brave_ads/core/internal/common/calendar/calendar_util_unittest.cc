/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/calendar/calendar_util.h"

#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsCalendarUtilTest, GetLastDayOfMonth) {
  // Arrange
  constexpr int kLastDayForMonth[12] = {31, 28, 31, 30, 31, 30,
                                        31, 31, 30, 31, 30, 31};

  // Act & Assert
  for (int i = 0; i < 12; i++) {
    EXPECT_EQ(kLastDayForMonth[i],
              GetLastDayOfMonth(/*year*/ 2021, /*month*/ i + 1));
  }
}

TEST(BraveAdsCalendarUtilTest, GetLastDayOfMonthForLeapYear) {
  // Arrange
  constexpr int kLastDayForMonth[12] = {31, 29, 31, 30, 31, 30,
                                        31, 31, 30, 31, 30, 31};

  // Act & Assert
  for (int i = 0; i < 12; i++) {
    EXPECT_EQ(kLastDayForMonth[i],
              GetLastDayOfMonth(/*year*/ 2020, /*month*/ i + 1));
  }
}

TEST(BraveAdsCalendarUtilTest, GetDayOfWeekForYearMonthAndDay) {
  // Act & Assert
  EXPECT_EQ(6, GetDayOfWeek(/*year*/ 2020, /*month*/ 2, /*day*/ 29));
}

TEST(BraveAdsCalendarUtilTest, GetDayOfWeek) {
  // Act & Assert
  EXPECT_EQ(3,
            GetDayOfWeek(TimeFromString("November 18 1970", /*is_local*/ false),
                         /*is_local*/ false));
}

}  // namespace brave_ads
