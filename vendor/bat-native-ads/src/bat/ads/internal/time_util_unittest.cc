/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/time_util.h"

#include <string>

#include "base/environment.h"
#include "base/time/time.h"
#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_time_util.h"
#include "bat/ads/internal/unittest_util.h"
#include "build/build_config.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

#if BUILDFLAG(IS_LINUX)

class ScopedLibcTZ {
 public:
  explicit ScopedLibcTZ(const std::string& timezone) {
    auto env = base::Environment::Create();
    std::string old_timezone_value;
    if (env->GetVar(kTZ, &old_timezone_value)) {
      old_timezone_ = old_timezone_value;
    }
    if (!env->SetVar(kTZ, timezone)) {
      success_ = false;
    }
    tzset();
  }

  ~ScopedLibcTZ() {
    auto env = base::Environment::Create();
    if (old_timezone_.has_value()) {
      CHECK(env->SetVar(kTZ, old_timezone_.value()));
    } else {
      CHECK(env->UnSetVar(kTZ));
    }
  }

  ScopedLibcTZ(const ScopedLibcTZ& other) = delete;
  ScopedLibcTZ& operator=(const ScopedLibcTZ& other) = delete;

  bool is_success() const { return success_; }

 private:
  static constexpr char kTZ[] = "TZ";

  bool success_ = true;
  absl::optional<std::string> old_timezone_;
};

constexpr char ScopedLibcTZ::kTZ[];

#endif  // BUILDFLAG(IS_LINUX)

class BatAdsTimeUtilTest : public UnitTestBase,
                           public testing::WithParamInterface<bool> {
 protected:
  BatAdsTimeUtilTest() = default;
  ~BatAdsTimeUtilTest() override = default;

  void SetUp() override {
    UnitTestBase::SetUp();
    SetFromLocalExplodedFailedForTesting(GetParam());
  }

  void TearDown() override {
    UnitTestBase::TearDown();
    SetFromLocalExplodedFailedForTesting(false);
  }
};

TEST_P(BatAdsTimeUtilTest, GetLocalTimeAsMinutes) {
  // Arrange
  const base::Time& time =
      TimeFromString("November 18 2020 12:34:56", /* is_local */ true);

  // Act
  const int minutes = GetLocalTimeAsMinutes(time);

  // Assert
  const int expected_minutes = (12 * base::Time::kMinutesPerHour) + 34;
  EXPECT_EQ(expected_minutes, minutes);
}

TEST_P(BatAdsTimeUtilTest, AdjustLocalTimeToBeginningOfPreviousMonth) {
  // Arrange
  const base::Time& time =
      TimeFromString("November 18 2020 12:34:56.789", /* is_local */ true);
  AdvanceClock(time);

  // Act
  const base::Time& adjusted_time =
      AdjustLocalTimeToBeginningOfPreviousMonth(time);

  // Assert
  const base::Time& expected_adjusted_time =
      TimeFromString("October 1 2020 00:00:00.000", /* is_local */ true);

  EXPECT_EQ(expected_adjusted_time, adjusted_time);
}

TEST_P(BatAdsTimeUtilTest, AdjustLocalTimeToBeginningOfPreviousMonthOnCusp) {
  // Arrange
  const base::Time& time =
      TimeFromString("January 1 2020 00:00:00.000", /* is_local */ true);
  AdvanceClock(time);

  // Act
  const base::Time& adjusted_time =
      AdjustLocalTimeToBeginningOfPreviousMonth(time);

  // Assert
  const base::Time& expected_adjusted_time =
      TimeFromString("December 1 2019 00:00:00.000", /* is_local */ true);

  EXPECT_EQ(expected_adjusted_time, adjusted_time);
}

TEST_P(BatAdsTimeUtilTest, AdjustLocalTimeToEndOfPreviousMonth) {
  // Arrange
  const base::Time& time =
      TimeFromString("November 18 2020 12:34:56.789", /* is_local */ true);
  AdvanceClock(time);

  // Act
  const base::Time& adjusted_time = AdjustLocalTimeToEndOfPreviousMonth(time);

  // Assert
  const base::Time& expected_adjusted_time =
      TimeFromString("October 31 2020 23:59:59.999", /* is_local */ true);

  EXPECT_EQ(expected_adjusted_time, adjusted_time);
}

TEST_P(BatAdsTimeUtilTest, AdjustLocalTimeToEndOfPreviousMonthOnTheCusp) {
  // Arrange
  const base::Time& time =
      TimeFromString("January 1 2020 00:00:00.000", /* is_local */ true);
  AdvanceClock(time);

  // Act
  const base::Time& adjusted_time = AdjustLocalTimeToEndOfPreviousMonth(time);

  // Assert
  const base::Time& expected_adjusted_time =
      TimeFromString("December 31 2019 23:59:59.999", /* is_local */ true);

  EXPECT_EQ(expected_adjusted_time, adjusted_time);
}

TEST_P(BatAdsTimeUtilTest, AdjustLocalTimeToBeginningOfMonth) {
  // Arrange
  const base::Time& time =
      TimeFromString("November 18 2020 12:34:56.789", /* is_local */ true);
  AdvanceClock(time);

  // Act
  const base::Time& adjusted_time = AdjustLocalTimeToBeginningOfMonth(time);

  // Assert
  const base::Time& expected_adjusted_time =
      TimeFromString("November 1 2020 00:00:00.000", /* is_local */ true);

  EXPECT_EQ(expected_adjusted_time, adjusted_time);
}

TEST_P(BatAdsTimeUtilTest, AdjustLocalTimeToEndOfMonth) {
  // Arrange
  const base::Time& time =
      TimeFromString("November 18 2020 12:34:56.789", /* is_local */ true);
  AdvanceClock(time);

  // Act
  const base::Time& adjusted_time = AdjustLocalTimeToEndOfMonth(time);

  // Assert
  const base::Time& expected_adjusted_time =
      TimeFromString("November 30 2020 23:59:59.999", /* is_local */ true);

  EXPECT_EQ(expected_adjusted_time, adjusted_time);
}

TEST_P(BatAdsTimeUtilTest, GetLocalTimeAtBeginningOfLastMonth) {
  // Arrange
  const base::Time& time =
      TimeFromString("November 18 2020 12:34:56.789", /* is_local */ true);
  AdvanceClock(time);

  // Act
  const base::Time& adjusted_time = GetLocalTimeAtBeginningOfLastMonth();

  // Assert
  const base::Time& expected_adjusted_time =
      TimeFromString("October 1 2020 00:00:00.000", /* is_local */ true);

  EXPECT_EQ(expected_adjusted_time, adjusted_time);
}

TEST_P(BatAdsTimeUtilTest, GetLocalTimeAtBeginningOfLastMonthOnTheCusp) {
  // Arrange
  const base::Time& time =
      TimeFromString("January 1 2020 00:00:00.000", /* is_local */ true);
  AdvanceClock(time);

  // Act
  const base::Time& adjusted_time = GetLocalTimeAtBeginningOfLastMonth();

  // Assert
  const base::Time& expected_adjusted_time =
      TimeFromString("December 1 2019 00:00:00.000", /* is_local */ true);

  EXPECT_EQ(expected_adjusted_time, adjusted_time);
}

TEST_P(BatAdsTimeUtilTest, GetLocalTimeAtEndOfLastMonth) {
  // Arrange
  const base::Time& time =
      TimeFromString("November 18 2020 12:34:56.789", /* is_local */ true);
  AdvanceClock(time);

  // Act
  const base::Time& adjusted_time = GetLocalTimeAtEndOfLastMonth();

  // Assert
  const base::Time& expected_adjusted_time =
      TimeFromString("October 31 2020 23:59:59.999", /* is_local */ true);

  EXPECT_EQ(expected_adjusted_time, adjusted_time);
}

TEST_P(BatAdsTimeUtilTest, GetLocalTimeAtEndOfLastMonthOnTheCusp) {
  // Arrange
  const base::Time& time =
      TimeFromString("January 1 2020 00:00:00.000", /* is_local */ true);
  AdvanceClock(time);

  // Act
  const base::Time& adjusted_time = GetLocalTimeAtEndOfLastMonth();

  // Assert
  const base::Time& expected_adjusted_time =
      TimeFromString("December 31 2019 23:59:59.999", /* is_local */ true);

  EXPECT_EQ(expected_adjusted_time, adjusted_time);
}

TEST_P(BatAdsTimeUtilTest, GetLocalTimeAtBeginningOfThisMonth) {
  // Arrange
  const base::Time& time =
      TimeFromString("November 18 2020 12:34:56.789", /* is_local */ true);
  AdvanceClock(time);

  // Act
  const base::Time& adjusted_time = GetLocalTimeAtBeginningOfThisMonth();

  // Assert
  const base::Time& expected_adjusted_time =
      TimeFromString("November 1 2020 00:00:00.000", /* is_local */ true);

  EXPECT_EQ(expected_adjusted_time, adjusted_time);
}

TEST_P(BatAdsTimeUtilTest, GetLocalTimeAtEndOfThisMonth) {
  // Arrange
  const base::Time& time =
      TimeFromString("November 18 2020 12:34:56.789", /* is_local */ true);
  AdvanceClock(time);

  // Act
  const base::Time& adjusted_time = GetLocalTimeAtEndOfThisMonth();

  // Assert
  const base::Time& expected_adjusted_time =
      TimeFromString("November 30 2020 23:59:59.999", /* is_local */ true);

  EXPECT_EQ(expected_adjusted_time, adjusted_time);
}

#if BUILDFLAG(IS_LINUX)
TEST_P(BatAdsTimeUtilTest, CheckLocalMidnightUSPaicficTimezone) {
  ScopedLibcTZ scoped_libc_tz("US/Pacific");
  // Arrange
  const base::Time& daylight_saving_started_day =
      TimeFromString("March 14 2021 23:34:56.789", /* is_local */ true);
  const base::Time& daylight_saving_ended_day =
      TimeFromString("November 7 2021 23:34:56.789", /* is_local */ true);

  // Act
  const base::Time& adjusted_daylight_saving_started_day =
      GetLocalMidnight(daylight_saving_started_day);
  const base::Time& adjusted_daylight_saving_ended_day =
      GetLocalMidnight(daylight_saving_ended_day);

  // Assert
  const base::Time& expected_daylight_saving_started_day =
      TimeFromString("March 14 2021 0:0:0.000", /* is_local */ true);
  const base::Time& expected_daylight_saving_ended_day =
      TimeFromString("November 7 2021 0:0:0.000", /* is_local */ true);

  EXPECT_EQ(expected_daylight_saving_started_day,
            adjusted_daylight_saving_started_day);
  EXPECT_EQ(expected_daylight_saving_ended_day,
            adjusted_daylight_saving_ended_day);
}

TEST_P(BatAdsTimeUtilTest, CheckLocalMidnightEuropeLondonTimezone) {
  ScopedLibcTZ scoped_libc_tz("Europe/London");
  // Arrange
  const base::Time& daylight_saving_started_day =
      TimeFromString("March 28 2021 23:34:56.789", /* is_local */ true);
  const base::Time& daylight_saving_ended_day =
      TimeFromString("October 31 2021 23:34:56.789", /* is_local */ true);

  // Act
  const base::Time& adjusted_daylight_saving_started_day =
      GetLocalMidnight(daylight_saving_started_day);
  const base::Time& adjusted_daylight_saving_ended_day =
      GetLocalMidnight(daylight_saving_ended_day);

  // Assert
  const base::Time& expected_daylight_saving_started_day =
      TimeFromString("March 28 2021 0:0:0.000", /* is_local */ true);
  const base::Time& expected_daylight_saving_ended_day =
      TimeFromString("October 31 2021 0:0:0.000", /* is_local */ true);

  EXPECT_EQ(expected_daylight_saving_started_day,
            adjusted_daylight_saving_started_day);
  EXPECT_EQ(expected_daylight_saving_ended_day,
            adjusted_daylight_saving_ended_day);
}

TEST_P(BatAdsTimeUtilTest, CheckLocalMidnightAustaliaSydneyTimezone) {
  ScopedLibcTZ scoped_libc_tz("Australia/Sydney");
  // Arrange
  const base::Time& daylight_saving_started_day =
      TimeFromString("October 3 2021 12:34:56.789", /* is_local */ true);
  const base::Time& daylight_saving_ended_day =
      TimeFromString("April 4 2021 12:34:56.789", /* is_local */ true);

  // Act
  const base::Time& adjusted_daylight_saving_started_day =
      GetLocalMidnight(daylight_saving_started_day);
  const base::Time& adjusted_daylight_saving_ended_day =
      GetLocalMidnight(daylight_saving_ended_day);

  // Assert
  const base::Time& expected_daylight_saving_started_day =
      TimeFromString("October 3 2021 0:0:0.000", /* is_local */ true);
  const base::Time& expected_daylight_saving_ended_day =
      TimeFromString("April 4 2021 0:0:0.000", /* is_local */ true);

  EXPECT_EQ(expected_daylight_saving_started_day,
            adjusted_daylight_saving_started_day);
  EXPECT_EQ(expected_daylight_saving_ended_day,
            adjusted_daylight_saving_ended_day);
}

TEST_P(BatAdsTimeUtilTest, CheckLocalMidnightNoDSTTimezone) {
  ScopedLibcTZ scoped_libc_tz("America/Cayman");
  // Arrange
  const base::Time& time =
      TimeFromString("November 7 2021 23:34:56.789", /* is_local */ true);

  // Act
  const base::Time& adjusted_time = GetLocalMidnight(time);

  // Assert
  const base::Time& expected_adjusted_time =
      TimeFromString("November 7 2021 0:0:0.000", /* is_local */ true);

  EXPECT_EQ(expected_adjusted_time, adjusted_time);
}

TEST_P(BatAdsTimeUtilTest,
       AdjustLocalTimeToBeginningOfMonthForUSPacificTimezone) {
  ScopedLibcTZ scoped_libc_tz("US/Pacific");
  // Arrange
  const base::Time& daylight_saving_started_time1 =
      TimeFromString("March 20 2021 00:34:56.789", /* is_local */ true);
  const base::Time& daylight_saving_started_time2 =
      TimeFromString("March 20 2021 23:34:56.789", /* is_local */ true);
  const base::Time& daylight_saving_ended_time1 =
      TimeFromString("November 18 2021 00:34:56.789", /* is_local */ true);
  const base::Time& daylight_saving_ended_time2 =
      TimeFromString("November 18 2021 23:34:56.789", /* is_local */ true);

  // Act
  const base::Time& adjusted_daylight_saving_started_time1 =
      AdjustLocalTimeToBeginningOfMonth(daylight_saving_started_time1);
  const base::Time& adjusted_daylight_saving_started_time2 =
      AdjustLocalTimeToBeginningOfMonth(daylight_saving_started_time2);
  const base::Time& adjusted_daylight_saving_ended_time1 =
      AdjustLocalTimeToBeginningOfMonth(daylight_saving_ended_time1);
  const base::Time& adjusted_daylight_saving_ended_time2 =
      AdjustLocalTimeToBeginningOfMonth(daylight_saving_ended_time2);

  // Assert
  const base::Time& expected_daylight_saving_started_time =
      TimeFromString("March 1 2021 00:00:00.000", /* is_local */ true);
  const base::Time& expected_daylight_saving_ended_time =
      TimeFromString("November 1 2021 00:00:00.000", /* is_local */ true);

  EXPECT_EQ(expected_daylight_saving_started_time,
            adjusted_daylight_saving_started_time1);
  EXPECT_EQ(expected_daylight_saving_started_time,
            adjusted_daylight_saving_started_time2);
  EXPECT_EQ(expected_daylight_saving_ended_time,
            adjusted_daylight_saving_ended_time1);
  EXPECT_EQ(expected_daylight_saving_ended_time,
            adjusted_daylight_saving_ended_time2);
}

TEST_P(BatAdsTimeUtilTest,
       AdjustLocalTimeToBeginningOfMonthForEuropeLondonTimezone) {
  ScopedLibcTZ scoped_libc_tz("Europe/London");
  // Arrange
  const base::Time& daylight_saving_started_time1 =
      TimeFromString("March 30 2021 00:34:56.789", /* is_local */ true);
  const base::Time& daylight_saving_started_time2 =
      TimeFromString("March 30 2021 23:34:56.789", /* is_local */ true);
  const base::Time& daylight_saving_ended_time1 =
      TimeFromString("October 31 2021 12:34:56.789", /* is_local */ true);
  const base::Time& daylight_saving_ended_time2 =
      TimeFromString("October 31 2021 23:34:56.789", /* is_local */ true);

  // Act
  const base::Time& adjusted_daylight_saving_started_time1 =
      AdjustLocalTimeToBeginningOfMonth(daylight_saving_started_time1);
  const base::Time& adjusted_daylight_saving_started_time2 =
      AdjustLocalTimeToBeginningOfMonth(daylight_saving_started_time2);
  const base::Time& adjusted_daylight_saving_ended_time1 =
      AdjustLocalTimeToBeginningOfMonth(daylight_saving_ended_time1);
  const base::Time& adjusted_daylight_saving_ended_time2 =
      AdjustLocalTimeToBeginningOfMonth(daylight_saving_ended_time2);

  // Assert
  const base::Time& expected_daylight_saving_started_time =
      TimeFromString("March 1 2021 00:00:00.000", /* is_local */ true);
  const base::Time& expected_daylight_saving_ended_time =
      TimeFromString("October 1 2021 00:00:00.000", /* is_local */ true);

  EXPECT_EQ(expected_daylight_saving_started_time,
            adjusted_daylight_saving_started_time1);
  EXPECT_EQ(expected_daylight_saving_started_time,
            adjusted_daylight_saving_started_time2);
  EXPECT_EQ(expected_daylight_saving_ended_time,
            adjusted_daylight_saving_ended_time1);
  EXPECT_EQ(expected_daylight_saving_ended_time,
            adjusted_daylight_saving_ended_time2);
}

TEST_P(BatAdsTimeUtilTest,
       AdjustLocalTimeToBeginningOfMonthForAustraliaSydneyTimezone) {
  ScopedLibcTZ scoped_libc_tz("Australia/Sydney");
  // Arrange
  const base::Time& daylight_saving_started_time1 =
      TimeFromString("October 3 2021 00:34:56.789", /* is_local */ true);
  const base::Time& daylight_saving_started_time2 =
      TimeFromString("October 3 2021 23:34:56.789", /* is_local */ true);
  const base::Time& daylight_saving_ended_time1 =
      TimeFromString("April 4 2021 12:34:56.789", /* is_local */ true);
  const base::Time& daylight_saving_ended_time2 =
      TimeFromString("April 4 2021 23:34:56.789", /* is_local */ true);

  // Act
  const base::Time& adjusted_daylight_saving_started_time1 =
      AdjustLocalTimeToBeginningOfMonth(daylight_saving_started_time1);
  const base::Time& adjusted_daylight_saving_started_time2 =
      AdjustLocalTimeToBeginningOfMonth(daylight_saving_started_time2);
  const base::Time& adjusted_daylight_saving_ended_time1 =
      AdjustLocalTimeToBeginningOfMonth(daylight_saving_ended_time1);
  const base::Time& adjusted_daylight_saving_ended_time2 =
      AdjustLocalTimeToBeginningOfMonth(daylight_saving_ended_time2);

  // Assert
  const base::Time& expected_daylight_saving_started_time =
      TimeFromString("October 1 2021 00:00:00.000", /* is_local */ true);
  const base::Time& expected_daylight_saving_ended_time =
      TimeFromString("April 1 2021 00:00:00.000", /* is_local */ true);

  EXPECT_EQ(expected_daylight_saving_started_time,
            adjusted_daylight_saving_started_time1);
  EXPECT_EQ(expected_daylight_saving_started_time,
            adjusted_daylight_saving_started_time2);
  EXPECT_EQ(expected_daylight_saving_ended_time,
            adjusted_daylight_saving_ended_time1);
  EXPECT_EQ(expected_daylight_saving_ended_time,
            adjusted_daylight_saving_ended_time2);
}

TEST_P(BatAdsTimeUtilTest, AdjustLocalTimeToBeginningOfMonthForNoDSTTimezone) {
  ScopedLibcTZ scoped_libc_tz("America/Cayman");
  // Arrange
  const base::Time& time =
      TimeFromString("November 7 2021 23:34:56.789", /* is_local */ true);

  // Act
  const base::Time& adjusted_time = AdjustLocalTimeToBeginningOfMonth(time);

  // Assert
  const base::Time& expected_adjusted_time =
      TimeFromString("November 1 2021 0:0:0.000", /* is_local */ true);

  EXPECT_EQ(expected_adjusted_time, adjusted_time);
}

TEST_P(BatAdsTimeUtilTest, AdjustLocalTimeToEndOfMonthForUSPacificTimezone) {
  ScopedLibcTZ scoped_libc_tz("US/Pacific");
  // Arrange
  const base::Time& daylight_saving_started_time1 =
      TimeFromString("March 3 2021 12:34:56.789", /* is_local */ true);
  const base::Time& daylight_saving_started_time2 =
      TimeFromString("March 20 2021 12:34:56.789", /* is_local */ true);
  const base::Time& daylight_saving_ended_time1 =
      TimeFromString("November 3 2021 12:34:56.789", /* is_local */ true);
  const base::Time& daylight_saving_ended_time2 =
      TimeFromString("November 20 2021 12:34:56.789", /* is_local */ true);

  // Act
  const base::Time& adjusted_daylight_saving_started_time1 =
      AdjustLocalTimeToEndOfMonth(daylight_saving_started_time1);
  const base::Time& adjusted_daylight_saving_started_time2 =
      AdjustLocalTimeToEndOfMonth(daylight_saving_started_time2);
  const base::Time& adjusted_daylight_saving_ended_time1 =
      AdjustLocalTimeToEndOfMonth(daylight_saving_ended_time1);
  const base::Time& adjusted_daylight_saving_ended_time2 =
      AdjustLocalTimeToEndOfMonth(daylight_saving_ended_time2);

  // Assert
  const base::Time& expected_daylight_saving_started_time =
      TimeFromString("March 31 2021 23:59:59.999", /* is_local */ true);
  const base::Time& expected_daylight_saving_ended_time =
      TimeFromString("November 30 2021 23:59:59.999", /* is_local */ true);

  EXPECT_EQ(expected_daylight_saving_started_time,
            adjusted_daylight_saving_started_time1);
  EXPECT_EQ(expected_daylight_saving_started_time,
            adjusted_daylight_saving_started_time2);
  EXPECT_EQ(expected_daylight_saving_ended_time,
            adjusted_daylight_saving_ended_time1);
  EXPECT_EQ(expected_daylight_saving_ended_time,
            adjusted_daylight_saving_ended_time2);
}

TEST_P(BatAdsTimeUtilTest, AdjustLocalTimeToEndOfMonthForEuropeLondonTimezone) {
  ScopedLibcTZ scoped_libc_tz("Europe/London");
  // Arrange
  const base::Time& daylight_saving_started_time1 =
      TimeFromString("March 3 2021 12:34:56.789", /* is_local */ true);
  const base::Time& daylight_saving_started_time2 =
      TimeFromString("March 29 2021 12:34:56.789", /* is_local */ true);
  const base::Time& daylight_saving_ended_time1 =
      TimeFromString("October 3 2021 12:34:56.789", /* is_local */ true);
  const base::Time& daylight_saving_ended_time2 =
      TimeFromString("October 31 2021 12:34:56.789", /* is_local */ true);

  // Act
  const base::Time& adjusted_daylight_saving_started_time1 =
      AdjustLocalTimeToEndOfMonth(daylight_saving_started_time1);
  const base::Time& adjusted_daylight_saving_started_time2 =
      AdjustLocalTimeToEndOfMonth(daylight_saving_started_time2);
  const base::Time& adjusted_daylight_saving_ended_time1 =
      AdjustLocalTimeToEndOfMonth(daylight_saving_ended_time1);
  const base::Time& adjusted_daylight_saving_ended_time2 =
      AdjustLocalTimeToEndOfMonth(daylight_saving_ended_time2);

  // Assert
  const base::Time& expected_daylight_saving_started_time =
      TimeFromString("March 31 2021 23:59:59.999", /* is_local */ true);
  const base::Time& expected_daylight_saving_ended_time =
      TimeFromString("October 31 2021 23:59:59.999", /* is_local */ true);

  EXPECT_EQ(expected_daylight_saving_started_time,
            adjusted_daylight_saving_started_time1);
  EXPECT_EQ(expected_daylight_saving_started_time,
            adjusted_daylight_saving_started_time2);
  EXPECT_EQ(expected_daylight_saving_ended_time,
            adjusted_daylight_saving_ended_time1);
  EXPECT_EQ(expected_daylight_saving_ended_time,
            adjusted_daylight_saving_ended_time2);
}

TEST_P(BatAdsTimeUtilTest,
       AdjustLocalTimeToEndOfMonthForAustraliaSydneyTimezone) {
  ScopedLibcTZ scoped_libc_tz("Australia/Sydney");
  const base::Time& daylight_saving_started_time1 =
      TimeFromString("October 1 2021 00:34:56.789", /* is_local */ true);
  const base::Time& daylight_saving_started_time2 =
      TimeFromString("October 1 2021 23:34:56.789", /* is_local */ true);
  const base::Time& daylight_saving_ended_time1 =
      TimeFromString("April 1 2021 12:34:56.789", /* is_local */ true);
  const base::Time& daylight_saving_ended_time2 =
      TimeFromString("April 1 2021 23:34:56.789", /* is_local */ true);

  // Act
  const base::Time& adjusted_daylight_saving_started_time1 =
      AdjustLocalTimeToEndOfMonth(daylight_saving_started_time1);
  const base::Time& adjusted_daylight_saving_started_time2 =
      AdjustLocalTimeToEndOfMonth(daylight_saving_started_time2);
  const base::Time& adjusted_daylight_saving_ended_time1 =
      AdjustLocalTimeToEndOfMonth(daylight_saving_ended_time1);
  const base::Time& adjusted_daylight_saving_ended_time2 =
      AdjustLocalTimeToEndOfMonth(daylight_saving_ended_time2);

  // Assert
  const base::Time& expected_daylight_saving_started_time =
      TimeFromString("October 31 2021 23:59:59.999", /* is_local */ true);
  const base::Time& expected_daylight_saving_ended_time =
      TimeFromString("April 30 2021 23:59:59.999", /* is_local */ true);

  EXPECT_EQ(expected_daylight_saving_started_time,
            adjusted_daylight_saving_started_time1);
  EXPECT_EQ(expected_daylight_saving_started_time,
            adjusted_daylight_saving_started_time2);
  EXPECT_EQ(expected_daylight_saving_ended_time,
            adjusted_daylight_saving_ended_time1);
  EXPECT_EQ(expected_daylight_saving_ended_time,
            adjusted_daylight_saving_ended_time2);
}

TEST_P(BatAdsTimeUtilTest, AdjustLocalTimeToEndOfMonthForNoDSTTimezone) {
  ScopedLibcTZ scoped_libc_tz("America/Cayman");
  // Arrange
  const base::Time& time =
      TimeFromString("November 7 2021 23:34:56.789", /* is_local */ true);

  // Act
  const base::Time& adjusted_time = AdjustLocalTimeToEndOfMonth(time);

  // Assert
  const base::Time& expected_adjusted_time =
      TimeFromString("November 30 2021 23:59:59.999", /* is_local */ true);

  EXPECT_EQ(expected_adjusted_time, adjusted_time);
}

TEST_P(BatAdsTimeUtilTest,
       AdjustLocalTimeToBeginningOfLastMonthForUSPacificTimezone) {
  ScopedLibcTZ scoped_libc_tz("US/Pacific");
  // Arrange
  const base::Time& daylight_saving_started_time =
      TimeFromString("April 5 2021 12:34:56.789", /* is_local */ true);
  const base::Time& daylight_saving_ended_time =
      TimeFromString("December 20 2021 12:34:56.789", /* is_local */ true);

  // Act
  const base::Time& adjusted_daylight_saving_started_time =
      AdjustLocalTimeToBeginningOfPreviousMonth(daylight_saving_started_time);
  const base::Time& adjusted_daylight_saving_ended_time =
      AdjustLocalTimeToBeginningOfPreviousMonth(daylight_saving_ended_time);

  // Assert
  const base::Time& expected_daylight_saving_started_time =
      TimeFromString("March 1 2021 00:00:00.000", /* is_local */ true);
  const base::Time& expected_daylight_saving_ended_time =
      TimeFromString("November 1 2021 00:00:00.000", /* is_local */ true);

  EXPECT_EQ(expected_daylight_saving_started_time,
            adjusted_daylight_saving_started_time);
  EXPECT_EQ(expected_daylight_saving_ended_time,
            adjusted_daylight_saving_ended_time);
}

TEST_P(BatAdsTimeUtilTest,
       AdjustLocalTimeToBeginningOfLastMonthForEuropeLondonTimezone) {
  ScopedLibcTZ scoped_libc_tz("Europe/London");
  // Arrange
  const base::Time& daylight_saving_started_time =
      TimeFromString("April 5 2021 12:34:56.789", /* is_local */ true);
  const base::Time& daylight_saving_ended_time =
      TimeFromString("November 20 2021 12:34:56.789", /* is_local */ true);

  // Act
  const base::Time& adjusted_daylight_saving_started_time =
      AdjustLocalTimeToBeginningOfPreviousMonth(daylight_saving_started_time);
  const base::Time& adjusted_daylight_saving_ended_time =
      AdjustLocalTimeToBeginningOfPreviousMonth(daylight_saving_ended_time);

  // Assert
  const base::Time& expected_daylight_saving_started_time =
      TimeFromString("March 1 2021 00:00:00.000", /* is_local */ true);
  const base::Time& expected_daylight_saving_ended_time =
      TimeFromString("October 1 2021 00:00:00.000", /* is_local */ true);

  EXPECT_EQ(expected_daylight_saving_started_time,
            adjusted_daylight_saving_started_time);
  EXPECT_EQ(expected_daylight_saving_ended_time,
            adjusted_daylight_saving_ended_time);
}

TEST_P(BatAdsTimeUtilTest,
       AdjustLocalTimeToBeginningOfLastMonthForAustraliaSydneyTimezone) {
  ScopedLibcTZ scoped_libc_tz("Australia/Sydney");
  // Arrange
  const base::Time& daylight_saving_started_time =
      TimeFromString("November 5 2021 12:34:56.789", /* is_local */ true);
  const base::Time& daylight_saving_ended_time =
      TimeFromString("May 20 2021 12:34:56.789", /* is_local */ true);

  // Act
  const base::Time& adjusted_daylight_saving_started_time =
      AdjustLocalTimeToBeginningOfPreviousMonth(daylight_saving_started_time);
  const base::Time& adjusted_daylight_saving_ended_time =
      AdjustLocalTimeToBeginningOfPreviousMonth(daylight_saving_ended_time);

  // Assert
  const base::Time& expected_daylight_saving_started_time =
      TimeFromString("October 1 2021 00:00:00.000", /* is_local */ true);
  const base::Time& expected_daylight_saving_ended_time =
      TimeFromString("April 1 2021 00:00:00.000", /* is_local */ true);

  EXPECT_EQ(expected_daylight_saving_started_time,
            adjusted_daylight_saving_started_time);
  EXPECT_EQ(expected_daylight_saving_ended_time,
            adjusted_daylight_saving_ended_time);
}

TEST_P(BatAdsTimeUtilTest,
       AdjustLocalTimeToBeginningOfLastMonthForNoDSTTimezone) {
  ScopedLibcTZ scoped_libc_tz("America/Cayman");
  // Arrange
  const base::Time& time =
      TimeFromString("November 7 2021 23:34:56.789", /* is_local */ true);

  // Act
  const base::Time& adjusted_time =
      AdjustLocalTimeToBeginningOfPreviousMonth(time);

  // Assert
  const base::Time& expected_adjusted_time =
      TimeFromString("October 1 2021 00:00:00.000", /* is_local */ true);

  EXPECT_EQ(expected_adjusted_time, adjusted_time);
}
#endif  // BUILDFLAG(IS_LINUX)

INSTANTIATE_TEST_SUITE_P(, BatAdsTimeUtilTest, ::testing::Bool());

}  // namespace ads
