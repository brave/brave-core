/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/time/time_delta_util.h"

#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/common/test/time_test_util.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsTimeDeltaUtilTest : public test::TestBase {};

// https://calculat.io/en/date/count was used to validate the following
// expectations.

TEST_F(BraveAdsTimeDeltaUtilTest, Months) {
  // Arrange
  AdvanceClockTo(test::TimeFromUTCString(
      "Wed, 31 Jan 2025 16:28"));  // Happy 1st Birthday Florrie!

  // Act & Assert
  EXPECT_EQ(test::TimeDeltaFromUTCString("Thu, 1 May 2025 16:28"), Months(3));
}

TEST_F(BraveAdsTimeDeltaUtilTest, MonthsIfLeapYear) {
  // Arrange
  AdvanceClockTo(test::TimeFromUTCString(
      /*National Static Electricity Day*/
      "Tue, 9 Jan 2024 16:10:19"));  // Did you know that the charge on a single
                                     // electron is 1.6 X 10-19 C.

  // Act & Assert
  EXPECT_EQ(test::TimeDeltaFromUTCString("Sat, 9 Mar 2024 16:10:19"),
            Months(2));
}

TEST_F(BraveAdsTimeDeltaUtilTest, MonthsOnCuspOfYear) {
  // Arrange
  AdvanceClockTo(test::TimeFromUTCString("Wed, 25 Dec 2024"));

  // Act & Assert
  EXPECT_EQ(test::TimeDeltaFromUTCString("Sat, 25 Jan 2025"), Months(1));
}

TEST_F(BraveAdsTimeDeltaUtilTest, MonthsOverMultipleYears) {
  // Arrange
  AdvanceClockTo(test::TimeFromUTCString("Sun, 18 Nov 2029"));

  // Act & Assert
  EXPECT_EQ(test::TimeDeltaFromUTCString("Sun, 18 May 2031"), Months(18));
}

}  // namespace brave_ads
