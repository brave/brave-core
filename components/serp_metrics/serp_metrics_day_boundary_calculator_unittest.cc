/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/serp_metrics/serp_metrics_day_boundary_calculator.h"

#include <string_view>

#include "base/test/icu_test_util.h"
#include "base/test/scoped_libc_timezone_override.h"
#include "base/test/task_environment.h"
#include "base/time/time.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/serp_metrics/timezone_test_util.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace serp_metrics {

class SerpMetricsDayBoundaryCalculatorTestBase : public testing::Test {
 public:
  SerpMetricsDayBoundaryCalculatorTestBase(std::string_view timezone,
                                           bool report_in_utc)
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME),
        libc_timezone_(std::string(timezone)),
        icu_timezone_(timezone.data()),
        report_in_utc_(report_in_utc) {
    local_state_.registry()->RegisterStringPref(kLastCheckYMD, "");
    calculator_ = std::make_unique<SerpMetricsDayBoundaryCalculator>(
        &local_state_, report_in_utc_);
  }

  base::Time MidnightOn(int year, int month, int day) const {
    const base::Time::Exploded exploded = {
        .year = year, .month = month, .day_of_month = day};

    base::Time time;
    if (report_in_utc_) {
      CHECK(base::Time::FromUTCExploded(exploded, &time));
      return time.UTCMidnight();
    }
    CHECK(base::Time::FromLocalExploded(exploded, &time));
    return time.LocalMidnight();
  }

 protected:
  base::test::TaskEnvironment task_environment_;

  base::test::ScopedLibcTimezoneOverride libc_timezone_;
  base::test::ScopedRestoreDefaultTimezone icu_timezone_;
  const bool report_in_utc_;

  TestingPrefServiceSimple local_state_;

  std::unique_ptr<SerpMetricsDayBoundaryCalculator> calculator_;
};

class SerpMetricsDayBoundaryCalculatorTest
    : public SerpMetricsDayBoundaryCalculatorTestBase,
      public testing::WithParamInterface<test::TimezoneAndReportInUTCParam> {
 public:
  SerpMetricsDayBoundaryCalculatorTest()
      : SerpMetricsDayBoundaryCalculatorTestBase(
            /*timezone=*/std::get<0>(GetParam()),
            /*report_in_utc=*/std::get<1>(GetParam())) {}
};

INSTANTIATE_TEST_SUITE_P(SerpMetricsDayBoundaryCalculatorTimezones,
                         SerpMetricsDayBoundaryCalculatorTest,
                         ::testing::Combine(test::kTimezones,
                                            ::testing::Bool()),
                         test::TimezoneAndReportInUtcParamName);

TEST_P(SerpMetricsDayBoundaryCalculatorTest,
       GetStartOfStalePeriodReturnsNullTimeWhenPrefIsEmpty) {
  EXPECT_TRUE(calculator_->GetStartOfStalePeriod().is_null());
}

TEST_P(SerpMetricsDayBoundaryCalculatorTest,
       GetStartOfStalePeriodReturnsNullTimeWhenPrefIsInvalid) {
  local_state_.SetString(kLastCheckYMD, "invalid");
  EXPECT_TRUE(calculator_->GetStartOfStalePeriod().is_null());
}

TEST_P(SerpMetricsDayBoundaryCalculatorTest,
       GetStartOfStalePeriodReturnsMidnightOfPrefDay) {
  local_state_.SetString(kLastCheckYMD, "2026-01-15");
  EXPECT_EQ(MidnightOn(/*year=*/2026, /*month=*/1, /*day=*/15),
            calculator_->GetStartOfStalePeriod());
}

TEST_P(SerpMetricsDayBoundaryCalculatorTest,
       GetStartOfYesterdayReturnsPreviousDayStart) {
  EXPECT_EQ(MidnightOn(/*year=*/2026, /*month=*/1, /*day=*/14),
            calculator_->GetStartOfYesterday(
                MidnightOn(/*year=*/2026, /*month=*/1, /*day=*/15) +
                base::Hours(12)));
}

TEST_P(SerpMetricsDayBoundaryCalculatorTest,
       GetEndOfYesterdayReturnsLastMillisecondOfPreviousDay) {
  EXPECT_EQ(MidnightOn(/*year=*/2026, /*month=*/1, /*day=*/15) -
                base::Milliseconds(1),
            calculator_->GetEndOfYesterday(
                MidnightOn(/*year=*/2026, /*month=*/1, /*day=*/15) +
                base::Hours(12)));
}

TEST_P(SerpMetricsDayBoundaryCalculatorTest,
       GetEndOfStalePeriodReturnsLastMillisecondTwoDaysAgo) {
  EXPECT_EQ(MidnightOn(/*year=*/2026, /*month=*/1, /*day=*/14) -
                base::Milliseconds(1),
            calculator_->GetEndOfStalePeriod(
                MidnightOn(/*year=*/2026, /*month=*/1, /*day=*/15) +
                base::Hours(12)));
}

TEST_P(SerpMetricsDayBoundaryCalculatorTest,
       GetStartOfYesterdayReturnsPreviousYearDecember31WhenInputIsJanuary1) {
  EXPECT_EQ(
      MidnightOn(/*year=*/2025, /*month=*/12, /*day=*/31),
      calculator_->GetStartOfYesterday(
          MidnightOn(/*year=*/2026, /*month=*/1, /*day=*/1) + base::Hours(12)));
}

class SerpMetricsDayBoundaryCalculatorDSTTransitionTest
    : public SerpMetricsDayBoundaryCalculatorTestBase {
 public:
  SerpMetricsDayBoundaryCalculatorDSTTransitionTest()
      : SerpMetricsDayBoundaryCalculatorTestBase(
            /*timezone=*/"America/New_York",
            /*report_in_utc=*/false) {}
};

TEST_F(SerpMetricsDayBoundaryCalculatorDSTTransitionTest,
       GetStartOfYesterdayReturnsMidnightOnDayBeforeSpringForwardTransition) {
  // 2026-03-08 03:30 local time (EDT), just after America/New_York
  // spring-forward.
  const base::Time::Exploded exploded = {
      .year = 2026, .month = 3, .day_of_month = 8, .hour = 3, .minute = 30};
  base::Time now;
  ASSERT_TRUE(base::Time::FromLocalExploded(exploded, &now));

  EXPECT_EQ(MidnightOn(/*year=*/2026, /*month=*/3, /*day=*/7),
            calculator_->GetStartOfYesterday(now));
}

}  // namespace serp_metrics
