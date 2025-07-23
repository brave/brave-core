/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/updater/features.h"

#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "base/time/time.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_updater {

class GetBuildAgeInDaysTest : public testing::Test {
 protected:
  base::test::TaskEnvironment task_environment_{
      base::test::TaskEnvironment::TimeSource::MOCK_TIME};
};

TEST_F(GetBuildAgeInDaysTest, IncreasesAsTimePasses) {
  int age_before = GetBuildAgeInDays();
  task_environment_.AdvanceClock(base::Days(1));
  EXPECT_EQ(GetBuildAgeInDays(), age_before + 1);
}

class ShouldUseOmaha4Test : public testing::Test {
 protected:
  base::test::ScopedFeatureList scoped_feature_list_;
};

TEST_F(ShouldUseOmaha4Test, ReturnsFalseWhenFeatureDisabled) {
  scoped_feature_list_.InitAndDisableFeature(kBraveUseOmaha4Alpha);

  for (int day = 0; day < 1000; day++) {
    EXPECT_FALSE(ShouldUseOmaha4(day))
        << "ShouldUseOmaha4(" << day
        << ") should return false when kBraveUseOmaha4Alpha is disabled";
  }
}

TEST_F(ShouldUseOmaha4Test, ReturnsTrueWhenFeatureEnabled) {
  scoped_feature_list_.InitAndEnableFeature(kBraveUseOmaha4Alpha);

  for (int day = 0; day < 4; day++) {
    EXPECT_TRUE(ShouldUseOmaha4(day))
        << "ShouldUseOmaha4(" << day
        << ") should return true for the first 4 days when "
           "kBraveUseOmaha4Alpha is enabled";
  }
}

TEST_F(ShouldUseOmaha4Test, ReturnsFalseOnEveryFifthDayWhenFeatureEnabled) {
  scoped_feature_list_.InitAndEnableFeature(kBraveUseOmaha4Alpha);

  int num_false_days = 0;

  for (int day = 0; day < 1000; day++) {
    if (!ShouldUseOmaha4(day)) {
      num_false_days++;
    }
  }

  EXPECT_EQ(num_false_days, 200)
      << "ShouldUseOmaha4() should return false on 20% of days even when "
         "kBraveUseOmaha4Alpha is enabled";
}

}  // namespace brave_updater
