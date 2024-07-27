/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/time/time_util.h"

#include <optional>

#include "base/check.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/common/test/time_test_util.h"
#include "build/build_config.h"

#if BUILDFLAG(IS_LINUX)
#include "base/environment.h"
#endif  // BUILDFLAG(IS_LINUX)

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

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
    if (old_timezone_) {
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
  std::optional<std::string> old_timezone_;
};

constexpr char ScopedLibcTZ::kTZ[];

#endif  // BUILDFLAG(IS_LINUX)

class BraveAdsTimeUtilTest : public test::TestBase,
                             public ::testing::WithParamInterface<bool> {
 protected:
  void SetUp() override {
    test::TestBase::SetUp();

    SetFromLocalExplodedFailedForTesting(GetParam());
  }

  void TearDown() override {
    SetFromLocalExplodedFailedForTesting(false);

    test::TestBase::TearDown();
  }
};

TEST_P(BraveAdsTimeUtilTest, GetLocalTimeInMinutes) {
  // Arrange
  const base::Time time = test::TimeFromString("November 18 2020 12:34:56");

  // Act & Assert
  EXPECT_EQ((12 * base::Time::kMinutesPerHour) + 34,
            GetLocalTimeInMinutes(time));
}

TEST_P(BraveAdsTimeUtilTest, AdjustLocalTimeToBeginningOfPreviousMonth) {
  // Arrange
  const base::Time time = test::TimeFromString("November 18 2020 12:34:56.789");
  AdvanceClockTo(time);

  // Act & Assert
  const base::Time expected_adjusted_time =
      test::TimeFromString("October 1 2020 00:00:00.000");
  EXPECT_EQ(expected_adjusted_time,
            AdjustLocalTimeToBeginningOfPreviousMonth(time));
}

TEST_P(BraveAdsTimeUtilTest, AdjustLocalTimeToBeginningOfPreviousMonthOnCusp) {
  // Arrange
  const base::Time time = test::TimeFromString("January 1 2020 00:00:00.000");
  AdvanceClockTo(time);

  // Act & Assert
  const base::Time expected_adjusted_time =
      test::TimeFromString("December 1 2019 00:00:00.000");
  EXPECT_EQ(expected_adjusted_time,
            AdjustLocalTimeToBeginningOfPreviousMonth(time));
}

TEST_P(BraveAdsTimeUtilTest, AdjustLocalTimeToEndOfPreviousMonth) {
  // Arrange
  const base::Time time = test::TimeFromString("November 18 2020 12:34:56.789");
  AdvanceClockTo(time);

  // Act & Assert
  const base::Time expected_adjusted_time =
      test::TimeFromString("October 31 2020 23:59:59.999");
  EXPECT_EQ(expected_adjusted_time, AdjustLocalTimeToEndOfPreviousMonth(time));
}

TEST_P(BraveAdsTimeUtilTest, AdjustLocalTimeToEndOfPreviousMonthOnTheCusp) {
  // Arrange
  const base::Time time = test::TimeFromString("January 1 2020 00:00:00.000");
  AdvanceClockTo(time);

  // Act & Assert
  const base::Time expected_adjusted_time =
      test::TimeFromString("December 31 2019 23:59:59.999");
  EXPECT_EQ(expected_adjusted_time, AdjustLocalTimeToEndOfPreviousMonth(time));
}

TEST_P(BraveAdsTimeUtilTest, AdjustLocalTimeToBeginningOfMonth) {
  // Arrange
  const base::Time time = test::TimeFromString("November 18 2020 12:34:56.789");
  AdvanceClockTo(time);

  // Act & Assert
  const base::Time expected_adjusted_time =
      test::TimeFromString("November 1 2020 00:00:00.000");
  EXPECT_EQ(expected_adjusted_time, AdjustLocalTimeToBeginningOfMonth(time));
}

TEST_P(BraveAdsTimeUtilTest, AdjustLocalTimeToEndOfMonth) {
  // Arrange
  const base::Time time = test::TimeFromString("November 18 2020 12:34:56.789");
  AdvanceClockTo(time);

  // Act & Assert
  const base::Time expected_adjusted_time =
      test::TimeFromString("November 30 2020 23:59:59.999");
  EXPECT_EQ(expected_adjusted_time, AdjustLocalTimeToEndOfMonth(time));
}

TEST_P(BraveAdsTimeUtilTest, GetLocalTimeAtBeginningOfLastMonth) {
  // Arrange
  const base::Time time = test::TimeFromString("November 18 2020 12:34:56.789");
  AdvanceClockTo(time);

  // Act & Assert
  const base::Time expected_adjusted_time =
      test::TimeFromString("October 1 2020 00:00:00.000");
  EXPECT_EQ(expected_adjusted_time, GetLocalTimeAtBeginningOfLastMonth());
}

TEST_P(BraveAdsTimeUtilTest, GetLocalTimeAtBeginningOfLastMonthOnTheCusp) {
  // Arrange
  const base::Time time = test::TimeFromString("January 1 2020 00:00:00.000");
  AdvanceClockTo(time);

  // Act & Assert
  const base::Time expected_adjusted_time =
      test::TimeFromString("December 1 2019 00:00:00.000");
  EXPECT_EQ(expected_adjusted_time, GetLocalTimeAtBeginningOfLastMonth());
}

TEST_P(BraveAdsTimeUtilTest, GetLocalTimeAtEndOfLastMonth) {
  // Arrange
  const base::Time time = test::TimeFromString("November 18 2020 12:34:56.789");
  AdvanceClockTo(time);

  // Act & Assert
  const base::Time expected_adjusted_time =
      test::TimeFromString("October 31 2020 23:59:59.999");
  EXPECT_EQ(expected_adjusted_time, GetLocalTimeAtEndOfLastMonth());
}

TEST_P(BraveAdsTimeUtilTest, GetLocalTimeAtEndOfLastMonthOnTheCusp) {
  // Arrange
  const base::Time time = test::TimeFromString("January 1 2020 00:00:00.000");
  AdvanceClockTo(time);

  // Act & Assert
  const base::Time expected_adjusted_time =
      test::TimeFromString("December 31 2019 23:59:59.999");
  EXPECT_EQ(expected_adjusted_time, GetLocalTimeAtEndOfLastMonth());
}

TEST_P(BraveAdsTimeUtilTest, GetLocalTimeAtBeginningOfThisMonth) {
  // Arrange
  const base::Time time = test::TimeFromString("November 18 2020 12:34:56.789");
  AdvanceClockTo(time);

  // Act & Assert
  const base::Time expected_adjusted_time =
      test::TimeFromString("November 1 2020 00:00:00.000");
  EXPECT_EQ(expected_adjusted_time, GetLocalTimeAtBeginningOfThisMonth());
}

TEST_P(BraveAdsTimeUtilTest, GetLocalTimeAtEndOfThisMonth) {
  // Arrange
  const base::Time time = test::TimeFromString("November 18 2020 12:34:56.789");
  AdvanceClockTo(time);

  // Act & Assert
  const base::Time expected_adjusted_time =
      test::TimeFromString("November 30 2020 23:59:59.999");
  EXPECT_EQ(expected_adjusted_time, GetLocalTimeAtEndOfThisMonth());
}

TEST_P(BraveAdsTimeUtilTest, TimeToPrivacyPreservingIso8601) {
  // Arrange
  const base::Time time =
      test::TimeFromUTCString("November 18 2020 23:45:12.345");
  AdvanceClockTo(time);

  // Act & Assert
  EXPECT_EQ("2020-11-18T23:00:00.000Z",
            TimeToPrivacyPreservingIso8601(test::Now()));
}

#if BUILDFLAG(IS_LINUX)
TEST_P(BraveAdsTimeUtilTest, CheckLocalMidnightUSPacificTimezone) {
  ScopedLibcTZ scoped_libc_tz("US/Pacific");
  // Arrange
  const base::Time daylight_saving_started_day =
      test::TimeFromString("March 14 2021 23:34:56.789");
  const base::Time daylight_saving_ended_day =
      test::TimeFromString("November 7 2021 23:34:56.789");

  // Act
  const base::Time adjusted_daylight_saving_started_day =
      GetLocalMidnight(daylight_saving_started_day);
  const base::Time adjusted_daylight_saving_ended_day =
      GetLocalMidnight(daylight_saving_ended_day);

  // Assert
  const base::Time expected_daylight_saving_started_day =
      test::TimeFromString("March 14 2021 0:0:0.000");
  EXPECT_EQ(expected_daylight_saving_started_day,
            adjusted_daylight_saving_started_day);

  const base::Time expected_daylight_saving_ended_day =
      test::TimeFromString("November 7 2021 0:0:0.000");
  EXPECT_EQ(expected_daylight_saving_ended_day,
            adjusted_daylight_saving_ended_day);
}

TEST_P(BraveAdsTimeUtilTest, CheckLocalMidnightEuropeLondonTimezone) {
  ScopedLibcTZ scoped_libc_tz("Europe/London");
  // Arrange
  const base::Time daylight_saving_started_day =
      test::TimeFromString("March 28 2021 23:34:56.789");
  const base::Time daylight_saving_ended_day =
      test::TimeFromString("October 31 2021 23:34:56.789");

  // Act
  const base::Time adjusted_daylight_saving_started_day =
      GetLocalMidnight(daylight_saving_started_day);
  const base::Time adjusted_daylight_saving_ended_day =
      GetLocalMidnight(daylight_saving_ended_day);

  // Assert
  const base::Time expected_daylight_saving_started_day =
      test::TimeFromString("March 28 2021 0:0:0.000");
  EXPECT_EQ(expected_daylight_saving_started_day,
            adjusted_daylight_saving_started_day);

  const base::Time expected_daylight_saving_ended_day =
      test::TimeFromString("October 31 2021 0:0:0.000");
  EXPECT_EQ(expected_daylight_saving_ended_day,
            adjusted_daylight_saving_ended_day);
}

TEST_P(BraveAdsTimeUtilTest, CheckLocalMidnightAustaliaSydneyTimezone) {
  ScopedLibcTZ scoped_libc_tz("Australia/Sydney");
  // Arrange
  const base::Time daylight_saving_started_day =
      test::TimeFromString("October 3 2021 12:34:56.789");
  const base::Time daylight_saving_ended_day =
      test::TimeFromString("April 4 2021 12:34:56.789");

  // Act
  const base::Time adjusted_daylight_saving_started_day =
      GetLocalMidnight(daylight_saving_started_day);
  const base::Time adjusted_daylight_saving_ended_day =
      GetLocalMidnight(daylight_saving_ended_day);

  // Assert
  const base::Time expected_daylight_saving_started_day =
      test::TimeFromString("October 3 2021 0:0:0.000");
  EXPECT_EQ(expected_daylight_saving_started_day,
            adjusted_daylight_saving_started_day);

  const base::Time expected_daylight_saving_ended_day =
      test::TimeFromString("April 4 2021 0:0:0.000");
  EXPECT_EQ(expected_daylight_saving_ended_day,
            adjusted_daylight_saving_ended_day);
}

TEST_P(BraveAdsTimeUtilTest, CheckLocalMidnightNoDSTTimezone) {
  ScopedLibcTZ scoped_libc_tz("America/Cayman");
  // Arrange
  const base::Time time = test::TimeFromString("November 7 2021 23:34:56.789");

  // Act
  const base::Time adjusted_time = GetLocalMidnight(time);

  // Assert
  const base::Time expected_adjusted_time =
      test::TimeFromString("November 7 2021 0:0:0.000");
  EXPECT_EQ(expected_adjusted_time, adjusted_time);
}

TEST_P(BraveAdsTimeUtilTest,
       AdjustLocalTimeToBeginningOfMonthForUSPacificTimezone) {
  ScopedLibcTZ scoped_libc_tz("US/Pacific");
  // Arrange
  const base::Time daylight_saving_started_time1 =
      test::TimeFromString("March 20 2021 00:34:56.789");
  const base::Time daylight_saving_started_time2 =
      test::TimeFromString("March 20 2021 23:34:56.789");
  const base::Time daylight_saving_ended_time1 =
      test::TimeFromString("November 18 2021 00:34:56.789");
  const base::Time daylight_saving_ended_time2 =
      test::TimeFromString("November 18 2021 23:34:56.789");

  // Act
  const base::Time adjusted_daylight_saving_started_time1 =
      AdjustLocalTimeToBeginningOfMonth(daylight_saving_started_time1);
  const base::Time adjusted_daylight_saving_started_time2 =
      AdjustLocalTimeToBeginningOfMonth(daylight_saving_started_time2);
  const base::Time adjusted_daylight_saving_ended_time1 =
      AdjustLocalTimeToBeginningOfMonth(daylight_saving_ended_time1);
  const base::Time adjusted_daylight_saving_ended_time2 =
      AdjustLocalTimeToBeginningOfMonth(daylight_saving_ended_time2);

  // Assert
  const base::Time expected_daylight_saving_started_time =
      test::TimeFromString("March 1 2021 00:00:00.000");
  EXPECT_EQ(expected_daylight_saving_started_time,
            adjusted_daylight_saving_started_time1);
  EXPECT_EQ(expected_daylight_saving_started_time,
            adjusted_daylight_saving_started_time2);

  const base::Time expected_daylight_saving_ended_time =
      test::TimeFromString("November 1 2021 00:00:00.000");
  EXPECT_EQ(expected_daylight_saving_ended_time,
            adjusted_daylight_saving_ended_time1);
  EXPECT_EQ(expected_daylight_saving_ended_time,
            adjusted_daylight_saving_ended_time2);
}

TEST_P(BraveAdsTimeUtilTest,
       AdjustLocalTimeToBeginningOfMonthForEuropeLondonTimezone) {
  ScopedLibcTZ scoped_libc_tz("Europe/London");
  // Arrange
  const base::Time daylight_saving_started_time1 =
      test::TimeFromString("March 30 2021 00:34:56.789");
  const base::Time daylight_saving_started_time2 =
      test::TimeFromString("March 30 2021 23:34:56.789");
  const base::Time daylight_saving_ended_time1 =
      test::TimeFromString("October 31 2021 12:34:56.789");
  const base::Time daylight_saving_ended_time2 =
      test::TimeFromString("October 31 2021 23:34:56.789");

  // Act
  const base::Time adjusted_daylight_saving_started_time1 =
      AdjustLocalTimeToBeginningOfMonth(daylight_saving_started_time1);
  const base::Time adjusted_daylight_saving_started_time2 =
      AdjustLocalTimeToBeginningOfMonth(daylight_saving_started_time2);
  const base::Time adjusted_daylight_saving_ended_time1 =
      AdjustLocalTimeToBeginningOfMonth(daylight_saving_ended_time1);
  const base::Time adjusted_daylight_saving_ended_time2 =
      AdjustLocalTimeToBeginningOfMonth(daylight_saving_ended_time2);

  // Assert
  const base::Time expected_daylight_saving_started_time =
      test::TimeFromString("March 1 2021 00:00:00.000");
  EXPECT_EQ(expected_daylight_saving_started_time,
            adjusted_daylight_saving_started_time1);
  EXPECT_EQ(expected_daylight_saving_started_time,
            adjusted_daylight_saving_started_time2);
  EXPECT_EQ(expected_daylight_saving_started_time,
            adjusted_daylight_saving_started_time1);

  const base::Time expected_daylight_saving_ended_time =
      test::TimeFromString("October 1 2021 00:00:00.000");
  EXPECT_EQ(expected_daylight_saving_started_time,
            adjusted_daylight_saving_started_time2);
  EXPECT_EQ(expected_daylight_saving_ended_time,
            adjusted_daylight_saving_ended_time1);
  EXPECT_EQ(expected_daylight_saving_ended_time,
            adjusted_daylight_saving_ended_time2);
}

TEST_P(BraveAdsTimeUtilTest,
       AdjustLocalTimeToBeginningOfMonthForAustraliaSydneyTimezone) {
  ScopedLibcTZ scoped_libc_tz("Australia/Sydney");
  // Arrange
  const base::Time daylight_saving_started_time1 =
      test::TimeFromString("October 3 2021 00:34:56.789");
  const base::Time daylight_saving_started_time2 =
      test::TimeFromString("October 3 2021 23:34:56.789");
  const base::Time daylight_saving_ended_time1 =
      test::TimeFromString("April 4 2021 12:34:56.789");
  const base::Time daylight_saving_ended_time2 =
      test::TimeFromString("April 4 2021 23:34:56.789");

  // Act
  const base::Time adjusted_daylight_saving_started_time1 =
      AdjustLocalTimeToBeginningOfMonth(daylight_saving_started_time1);
  const base::Time adjusted_daylight_saving_started_time2 =
      AdjustLocalTimeToBeginningOfMonth(daylight_saving_started_time2);
  const base::Time adjusted_daylight_saving_ended_time1 =
      AdjustLocalTimeToBeginningOfMonth(daylight_saving_ended_time1);
  const base::Time adjusted_daylight_saving_ended_time2 =
      AdjustLocalTimeToBeginningOfMonth(daylight_saving_ended_time2);

  // Assert
  const base::Time expected_daylight_saving_started_time =
      test::TimeFromString("October 1 2021 00:00:00.000");
  EXPECT_EQ(expected_daylight_saving_started_time,
            adjusted_daylight_saving_started_time1);
  EXPECT_EQ(expected_daylight_saving_started_time,
            adjusted_daylight_saving_started_time2);

  const base::Time expected_daylight_saving_ended_time =
      test::TimeFromString("April 1 2021 00:00:00.000");
  EXPECT_EQ(expected_daylight_saving_ended_time,
            adjusted_daylight_saving_ended_time1);
  EXPECT_EQ(expected_daylight_saving_ended_time,
            adjusted_daylight_saving_ended_time2);
}

TEST_P(BraveAdsTimeUtilTest,
       AdjustLocalTimeToBeginningOfMonthForNoDSTTimezone) {
  ScopedLibcTZ scoped_libc_tz("America/Cayman");
  // Arrange
  const base::Time time = test::TimeFromString("November 7 2021 23:34:56.789");

  // Act
  const base::Time adjusted_time = AdjustLocalTimeToBeginningOfMonth(time);

  // Assert
  const base::Time expected_adjusted_time =
      test::TimeFromString("November 1 2021 0:0:0.000");
  EXPECT_EQ(expected_adjusted_time, adjusted_time);
}

TEST_P(BraveAdsTimeUtilTest, AdjustLocalTimeToEndOfMonthForUSPacificTimezone) {
  ScopedLibcTZ scoped_libc_tz("US/Pacific");
  // Arrange
  const base::Time daylight_saving_started_time1 =
      test::TimeFromString("March 3 2021 12:34:56.789");
  const base::Time daylight_saving_started_time2 =
      test::TimeFromString("March 20 2021 12:34:56.789");
  const base::Time daylight_saving_ended_time1 =
      test::TimeFromString("November 3 2021 12:34:56.789");
  const base::Time daylight_saving_ended_time2 =
      test::TimeFromString("November 20 2021 12:34:56.789");

  // Act
  const base::Time adjusted_daylight_saving_started_time1 =
      AdjustLocalTimeToEndOfMonth(daylight_saving_started_time1);
  const base::Time adjusted_daylight_saving_started_time2 =
      AdjustLocalTimeToEndOfMonth(daylight_saving_started_time2);
  const base::Time adjusted_daylight_saving_ended_time1 =
      AdjustLocalTimeToEndOfMonth(daylight_saving_ended_time1);
  const base::Time adjusted_daylight_saving_ended_time2 =
      AdjustLocalTimeToEndOfMonth(daylight_saving_ended_time2);

  // Assert
  const base::Time expected_daylight_saving_started_time =
      test::TimeFromString("March 31 2021 23:59:59.999");
  EXPECT_EQ(expected_daylight_saving_started_time,
            adjusted_daylight_saving_started_time1);
  EXPECT_EQ(expected_daylight_saving_started_time,
            adjusted_daylight_saving_started_time2);

  const base::Time expected_daylight_saving_ended_time =
      test::TimeFromString("November 30 2021 23:59:59.999");
  EXPECT_EQ(expected_daylight_saving_ended_time,
            adjusted_daylight_saving_ended_time1);
  EXPECT_EQ(expected_daylight_saving_ended_time,
            adjusted_daylight_saving_ended_time2);
}

TEST_P(BraveAdsTimeUtilTest,
       AdjustLocalTimeToEndOfMonthForEuropeLondonTimezone) {
  ScopedLibcTZ scoped_libc_tz("Europe/London");
  // Arrange
  const base::Time daylight_saving_started_time1 =
      test::TimeFromString("March 3 2021 12:34:56.789");
  const base::Time daylight_saving_started_time2 =
      test::TimeFromString("March 29 2021 12:34:56.789");
  const base::Time daylight_saving_ended_time1 =
      test::TimeFromString("October 3 2021 12:34:56.789");
  const base::Time daylight_saving_ended_time2 =
      test::TimeFromString("October 31 2021 12:34:56.789");

  // Act
  const base::Time adjusted_daylight_saving_started_time1 =
      AdjustLocalTimeToEndOfMonth(daylight_saving_started_time1);
  const base::Time adjusted_daylight_saving_started_time2 =
      AdjustLocalTimeToEndOfMonth(daylight_saving_started_time2);
  const base::Time adjusted_daylight_saving_ended_time1 =
      AdjustLocalTimeToEndOfMonth(daylight_saving_ended_time1);
  const base::Time adjusted_daylight_saving_ended_time2 =
      AdjustLocalTimeToEndOfMonth(daylight_saving_ended_time2);

  // Assert
  const base::Time expected_daylight_saving_started_time =
      test::TimeFromString("March 31 2021 23:59:59.999");
  EXPECT_EQ(expected_daylight_saving_started_time,
            adjusted_daylight_saving_started_time1);
  EXPECT_EQ(expected_daylight_saving_started_time,
            adjusted_daylight_saving_started_time2);

  const base::Time expected_daylight_saving_ended_time =
      test::TimeFromString("October 31 2021 23:59:59.999");
  EXPECT_EQ(expected_daylight_saving_ended_time,
            adjusted_daylight_saving_ended_time1);
  EXPECT_EQ(expected_daylight_saving_ended_time,
            adjusted_daylight_saving_ended_time2);
}

TEST_P(BraveAdsTimeUtilTest,
       AdjustLocalTimeToEndOfMonthForAustraliaSydneyTimezone) {
  ScopedLibcTZ scoped_libc_tz("Australia/Sydney");
  const base::Time daylight_saving_started_time1 =
      test::TimeFromString("October 1 2021 00:34:56.789");
  const base::Time daylight_saving_started_time2 =
      test::TimeFromString("October 1 2021 23:34:56.789");
  const base::Time daylight_saving_ended_time1 =
      test::TimeFromString("April 1 2021 12:34:56.789");
  const base::Time daylight_saving_ended_time2 =
      test::TimeFromString("April 1 2021 23:34:56.789");

  // Act
  const base::Time adjusted_daylight_saving_started_time1 =
      AdjustLocalTimeToEndOfMonth(daylight_saving_started_time1);
  const base::Time adjusted_daylight_saving_started_time2 =
      AdjustLocalTimeToEndOfMonth(daylight_saving_started_time2);
  const base::Time adjusted_daylight_saving_ended_time1 =
      AdjustLocalTimeToEndOfMonth(daylight_saving_ended_time1);
  const base::Time adjusted_daylight_saving_ended_time2 =
      AdjustLocalTimeToEndOfMonth(daylight_saving_ended_time2);

  // Assert
  const base::Time expected_daylight_saving_started_time =
      test::TimeFromString("October 31 2021 23:59:59.999");
  EXPECT_EQ(expected_daylight_saving_started_time,
            adjusted_daylight_saving_started_time1);
  EXPECT_EQ(expected_daylight_saving_started_time,
            adjusted_daylight_saving_started_time2);

  const base::Time expected_daylight_saving_ended_time =
      test::TimeFromString("April 30 2021 23:59:59.999");
  EXPECT_EQ(expected_daylight_saving_ended_time,
            adjusted_daylight_saving_ended_time1);
  EXPECT_EQ(expected_daylight_saving_ended_time,
            adjusted_daylight_saving_ended_time2);
}

TEST_P(BraveAdsTimeUtilTest, AdjustLocalTimeToEndOfMonthForNoDSTTimezone) {
  ScopedLibcTZ scoped_libc_tz("America/Cayman");
  // Arrange
  const base::Time time = test::TimeFromString("November 7 2021 23:34:56.789");

  // Act
  const base::Time adjusted_time = AdjustLocalTimeToEndOfMonth(time);

  // Assert
  const base::Time expected_adjusted_time =
      test::TimeFromString("November 30 2021 23:59:59.999");
  EXPECT_EQ(expected_adjusted_time, adjusted_time);
}

TEST_P(BraveAdsTimeUtilTest,
       AdjustLocalTimeToBeginningOfLastMonthForUSPacificTimezone) {
  ScopedLibcTZ scoped_libc_tz("US/Pacific");
  // Arrange
  const base::Time daylight_saving_started_time =
      test::TimeFromString("April 5 2021 12:34:56.789");
  const base::Time daylight_saving_ended_time =
      test::TimeFromString("December 20 2021 12:34:56.789");

  // Act
  const base::Time adjusted_daylight_saving_started_time =
      AdjustLocalTimeToBeginningOfPreviousMonth(daylight_saving_started_time);
  const base::Time adjusted_daylight_saving_ended_time =
      AdjustLocalTimeToBeginningOfPreviousMonth(daylight_saving_ended_time);

  // Assert
  const base::Time expected_daylight_saving_started_time =
      test::TimeFromString("March 1 2021 00:00:00.000");
  EXPECT_EQ(expected_daylight_saving_started_time,
            adjusted_daylight_saving_started_time);

  const base::Time expected_daylight_saving_ended_time =
      test::TimeFromString("November 1 2021 00:00:00.000");
  EXPECT_EQ(expected_daylight_saving_ended_time,
            adjusted_daylight_saving_ended_time);
}

TEST_P(BraveAdsTimeUtilTest,
       AdjustLocalTimeToBeginningOfLastMonthForEuropeLondonTimezone) {
  ScopedLibcTZ scoped_libc_tz("Europe/London");
  // Arrange
  const base::Time daylight_saving_started_time =
      test::TimeFromString("April 5 2021 12:34:56.789");
  const base::Time daylight_saving_ended_time =
      test::TimeFromString("November 20 2021 12:34:56.789");

  // Act
  const base::Time adjusted_daylight_saving_started_time =
      AdjustLocalTimeToBeginningOfPreviousMonth(daylight_saving_started_time);
  const base::Time adjusted_daylight_saving_ended_time =
      AdjustLocalTimeToBeginningOfPreviousMonth(daylight_saving_ended_time);

  // Assert
  const base::Time expected_daylight_saving_started_time =
      test::TimeFromString("March 1 2021 00:00:00.000");
  EXPECT_EQ(expected_daylight_saving_started_time,
            adjusted_daylight_saving_started_time);

  const base::Time expected_daylight_saving_ended_time =
      test::TimeFromString("October 1 2021 00:00:00.000");
  EXPECT_EQ(expected_daylight_saving_ended_time,
            adjusted_daylight_saving_ended_time);
}

TEST_P(BraveAdsTimeUtilTest,
       AdjustLocalTimeToBeginningOfLastMonthForAustraliaSydneyTimezone) {
  ScopedLibcTZ scoped_libc_tz("Australia/Sydney");
  // Arrange
  const base::Time daylight_saving_started_time =
      test::TimeFromString("November 5 2021 12:34:56.789");
  const base::Time daylight_saving_ended_time =
      test::TimeFromString("May 20 2021 12:34:56.789");

  // Act
  const base::Time adjusted_daylight_saving_started_time =
      AdjustLocalTimeToBeginningOfPreviousMonth(daylight_saving_started_time);
  const base::Time adjusted_daylight_saving_ended_time =
      AdjustLocalTimeToBeginningOfPreviousMonth(daylight_saving_ended_time);

  // Assert
  const base::Time expected_daylight_saving_started_time =
      test::TimeFromString("October 1 2021 00:00:00.000");
  EXPECT_EQ(expected_daylight_saving_started_time,
            adjusted_daylight_saving_started_time);

  const base::Time expected_daylight_saving_ended_time =
      test::TimeFromString("April 1 2021 00:00:00.000");
  EXPECT_EQ(expected_daylight_saving_ended_time,
            adjusted_daylight_saving_ended_time);
}

TEST_P(BraveAdsTimeUtilTest,
       AdjustLocalTimeToBeginningOfLastMonthForNoDSTTimezone) {
  ScopedLibcTZ scoped_libc_tz("America/Cayman");
  // Arrange
  const base::Time time = test::TimeFromString("November 7 2021 23:34:56.789");

  // Act
  const base::Time adjusted_time =
      AdjustLocalTimeToBeginningOfPreviousMonth(time);

  // Assert
  const base::Time expected_adjusted_time =
      test::TimeFromString("October 1 2021 00:00:00.000");
  EXPECT_EQ(expected_adjusted_time, adjusted_time);
}
#endif  // BUILDFLAG(IS_LINUX)

INSTANTIATE_TEST_SUITE_P(, BraveAdsTimeUtilTest, ::testing::Bool());

}  // namespace brave_ads
