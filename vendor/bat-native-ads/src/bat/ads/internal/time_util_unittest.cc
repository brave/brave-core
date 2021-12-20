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

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

#if defined(OS_LINUX)

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

#endif  // defined(OS_LINUX)

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

TEST_P(BatAdsTimeUtilTest, AdjustTimeToBeginningOfPreviousMonth) {
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

TEST_P(BatAdsTimeUtilTest, AdjustTimeToBeginningOfPreviousMonthOnCusp) {
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

TEST_P(BatAdsTimeUtilTest, AdjustTimeToEndOfPreviousMonth) {
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

TEST_P(BatAdsTimeUtilTest, AdjustTimeToEndOfPreviousMonthOnTheCusp) {
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

TEST_P(BatAdsTimeUtilTest, AdjustTimeToBeginningOfMonth) {
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

TEST_P(BatAdsTimeUtilTest, AdjustTimeToEndOfMonth) {
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

TEST_P(BatAdsTimeUtilTest, GetTimeAtBeginningOfLastMonth) {
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

TEST_P(BatAdsTimeUtilTest, GetTimeAtBeginningOfLastMonthOnTheCusp) {
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

TEST_P(BatAdsTimeUtilTest, GetTimeAtEndOfLastMonth) {
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

TEST_P(BatAdsTimeUtilTest, GetTimeAtEndOfLastMonthOnTheCusp) {
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

TEST_P(BatAdsTimeUtilTest, GetTimeAtBeginningOfThisMonth) {
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

TEST_P(BatAdsTimeUtilTest, GetTimeAtEndOfThisMonth) {
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

#if defined(OS_LINUX)
TEST_P(BatAdsTimeUtilTest, CheckLocalMidnightWithDaylightSavingStarted) {
  ScopedLibcTZ scoped_libc_tz("Australia/Sydney");
  // Arrange
  const base::Time& time =
      TimeFromString("October 3 2021 9:34:56.789", /* is_local */ true);
  AdvanceClock(time);

  // Act
  base::Time::Exploded exploded;
  time.LocalExplode(&exploded);
  EXPECT_TRUE(exploded.HasValidValues());

  base::Time adjusted_time = GetLocalMidnight(time, exploded);

  // Assert
  const base::Time& expected_adjusted_time =
      TimeFromString("October 3 2021 0:0:0.000", /* is_local */ true);

  EXPECT_EQ(expected_adjusted_time, adjusted_time);
}

TEST_P(BatAdsTimeUtilTest, CheckLocalMidnightWithDaylightSavingEnded) {
  ScopedLibcTZ scoped_libc_tz("Australia/Sydney");
  // Arrange
  const base::Time& time =
      TimeFromString("April 4 2021 9:34:56.789", /* is_local */ true);
  AdvanceClock(time);

  // Act
  base::Time::Exploded exploded;
  time.LocalExplode(&exploded);
  EXPECT_TRUE(exploded.HasValidValues());

  base::Time adjusted_time = GetLocalMidnight(time, exploded);

  // Assert
  const base::Time& expected_adjusted_time =
      TimeFromString("April 4 2021 0:0:0.000", /* is_local */ true);

  EXPECT_EQ(expected_adjusted_time, adjusted_time);
}
#endif  // defined(OS_LINUX)

INSTANTIATE_TEST_SUITE_P(, BatAdsTimeUtilTest, ::testing::Bool());

}  // namespace ads
