// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/p3a/rotation_scheduler.h"

#include <memory>

#include "base/containers/flat_map.h"
#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "base/time/time.h"
#include "brave/components/p3a/metric_log_type.h"
#include "brave/components/p3a/p3a_config.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace p3a {

class P3ARotationSchedulerTest : public testing::Test {
 public:
  P3ARotationSchedulerTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {
    RotationScheduler::RegisterPrefs(local_state_.registry());
  }

 protected:
  void SetUp() override {
    base::Time future_mock_time;
    if (base::Time::FromString("2050-01-07", &future_mock_time)) {
      task_environment_.AdvanceClock(future_mock_time - base::Time::Now());
    }

    scheduler_ = std::make_unique<RotationScheduler>(
        local_state_, &config_,
        base::BindLambdaForTesting(
            [&](MetricLogType log_type) { json_rotation_counts_[log_type]++; }),
        base::BindLambdaForTesting(
            [&](MetricLogType log_type) { constellation_rotation_count_++; }));
  }

  base::test::TaskEnvironment task_environment_;
  TestingPrefServiceSimple local_state_;
  P3AConfig config_;
  std::unique_ptr<RotationScheduler> scheduler_;
  base::flat_map<MetricLogType, size_t> json_rotation_counts_;
  size_t constellation_rotation_count_ = 0;
};

TEST_F(P3ARotationSchedulerTest, JsonRotation) {
  task_environment_.FastForwardBy(base::Days(60));

  // 60 days + 1 initial rotation
  EXPECT_EQ(json_rotation_counts_[MetricLogType::kExpress], 61u);
  // 9 weeks + 1 initial rotation
  EXPECT_EQ(json_rotation_counts_[MetricLogType::kTypical], 10u);
  // 2 months + 1 initial rotation
  EXPECT_EQ(json_rotation_counts_[MetricLogType::kSlow], 3u);
}

TEST_F(P3ARotationSchedulerTest, ConstellationRotation) {
  for (MetricLogType log_type : kAllMetricLogTypes) {
    constellation_rotation_count_ = 0;
    task_environment_.FastForwardBy(base::Days(7));
    // Should be 0 since the timer has not started
    EXPECT_EQ(constellation_rotation_count_, 0u);

    scheduler_->InitConstellationTimer(log_type,
                                       base::Time::Now() + base::Days(7));

    task_environment_.FastForwardBy(base::Days(7));
    EXPECT_EQ(constellation_rotation_count_, 0u);

    task_environment_.FastForwardBy(base::Seconds(5));
    EXPECT_EQ(constellation_rotation_count_, 1u);

    task_environment_.FastForwardBy(base::Days(30));
    // Should not rotate again until InitConstellationTimer sets the timer
    EXPECT_EQ(constellation_rotation_count_, 1u);

    scheduler_->InitConstellationTimer(log_type,
                                       base::Time::Now() + base::Days(7));
    task_environment_.FastForwardBy(base::Days(7));
    EXPECT_EQ(constellation_rotation_count_, 1u);

    // Should trigger at +5 seconds.
    task_environment_.FastForwardBy(base::Seconds(5));
    EXPECT_EQ(constellation_rotation_count_, 2u);
  }
}

}  // namespace p3a
