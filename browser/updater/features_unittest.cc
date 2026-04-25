/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/updater/features.h"

#include <optional>

#include "base/test/scoped_feature_list.h"
#include "base/time/time.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_updater {

class ShouldUseOmaha4Test : public testing::Test {
 protected:
  bool ShouldUseOmaha4(int on_day) {
    base::Time now = base::Time() + base::Days(on_day);
    return ShouldUseOmaha4ForTesting(now, state_);
  }
  void TearDown() override { state_.reset(); }
  base::test::ScopedFeatureList scoped_feature_list_;
  std::optional<bool> state_;
};

TEST_F(ShouldUseOmaha4Test, ReturnsFalseWhenFeatureDisabled) {
  scoped_feature_list_.InitAndDisableFeature(kBraveUseOmaha4);
  EXPECT_FALSE(ShouldUseOmaha4(1));
}

TEST_F(ShouldUseOmaha4Test, ReturnsTrueWhenFeatureEnabled) {
  scoped_feature_list_.InitAndEnableFeature(kBraveUseOmaha4);
  EXPECT_TRUE(ShouldUseOmaha4(1));
}

TEST_F(ShouldUseOmaha4Test, LetsLegacyImplRunEvenWhenFeatureEnabled) {
  scoped_feature_list_.InitAndEnableFeature(kBraveUseOmaha4);
  EXPECT_FALSE(ShouldUseOmaha4(5));
}

TEST_F(ShouldUseOmaha4Test, StaysConstantWhenFeatureDisabled) {
  scoped_feature_list_.InitAndDisableFeature(kBraveUseOmaha4);
  for (int day = 1; day < 10; day++) {
    EXPECT_FALSE(ShouldUseOmaha4(day));
  }
}

TEST_F(ShouldUseOmaha4Test, StaysConstantWhenFeatureEnabled) {
  scoped_feature_list_.InitAndEnableFeature(kBraveUseOmaha4);
  for (int day = 1; day < 10; day++) {
    EXPECT_TRUE(ShouldUseOmaha4(day));
  }
}

TEST_F(ShouldUseOmaha4Test, StaysConstantWhenLegacyImplRuns) {
  scoped_feature_list_.InitAndEnableFeature(kBraveUseOmaha4);
  // Trigger the `% 5 == 0` case on day 0.
  for (int day = 0; day < 10; day++) {
    EXPECT_FALSE(ShouldUseOmaha4(day));
  }
}

}  // namespace brave_updater
