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
  void TearDown() override { ResetShouldUseOmaha4(); }
  base::test::ScopedFeatureList scoped_feature_list_;
};

TEST_F(ShouldUseOmaha4Test, ReturnsFalseWhenFeatureDisabled) {
  scoped_feature_list_.InitAndDisableFeature(kBraveUseOmaha4Alpha);
  EXPECT_FALSE(ShouldUseOmaha4(0));
}

TEST_F(ShouldUseOmaha4Test, ReturnsTrueWhenFeatureEnabled) {
  scoped_feature_list_.InitAndEnableFeature(kBraveUseOmaha4Alpha);
  EXPECT_TRUE(ShouldUseOmaha4(0));
}

TEST_F(ShouldUseOmaha4Test, LetsLegacyImplRunEvenWhenFeatureEnabled) {
  scoped_feature_list_.InitAndEnableFeature(kBraveUseOmaha4Alpha);
  EXPECT_FALSE(ShouldUseOmaha4(4));
}

TEST_F(ShouldUseOmaha4Test, StaysConstantWhenFeatureDisabled) {
  scoped_feature_list_.InitAndDisableFeature(kBraveUseOmaha4Alpha);
  for (int day = 0; day < 1000; day++) {
    EXPECT_FALSE(ShouldUseOmaha4(day));
  }
}

TEST_F(ShouldUseOmaha4Test, StaysConstantWhenFeatureEnabled) {
  scoped_feature_list_.InitAndEnableFeature(kBraveUseOmaha4Alpha);
  for (int day = 0; day < 1000; day++) {
    EXPECT_TRUE(ShouldUseOmaha4(day));
  }
}

TEST_F(ShouldUseOmaha4Test, StaysConstantWhenLegacyImplRuns) {
  scoped_feature_list_.InitAndEnableFeature(kBraveUseOmaha4Alpha);
  for (int day = 4; day < 1000; day++) {
    EXPECT_FALSE(ShouldUseOmaha4(day));
  }
}

}  // namespace brave_updater
