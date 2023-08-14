/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/calendar/calendar_leap_year_util.h"

#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsCalendarLeapYearUtilTest, IsLeapYear) {
  // Arrange

  // Act

  // Assert
  for (int year = 2000; year < 2050; year++) {
    EXPECT_EQ(year % 4 == 0, IsLeapYear(year));
  }
}

}  // namespace brave_ads
