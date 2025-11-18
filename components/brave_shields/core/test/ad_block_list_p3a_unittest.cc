// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_shields/core/browser/ad_block_list_p3a.h"

#include <memory>

#include "base/test/metrics/histogram_tester.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_shields/core/common/features.h"
#include "brave/components/brave_shields/core/common/pref_names.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_shields {

class AdBlockListP3ATest : public testing::Test {
 public:
  AdBlockListP3ATest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}

 protected:
  void SetUp() override {
    scoped_feature_list_.InitAndEnableFeature(features::kAdblockOnlyMode);

    PrefRegistrySimple* registry = local_state_.registry();
    registry->RegisterBooleanPref(prefs::kAdBlockOnlyModeEnabled, false);

    ad_block_list_p3a_ = std::make_unique<AdBlockListP3A>(&local_state_);
  }

  void TearDown() override { ad_block_list_p3a_ = nullptr; }

  content::BrowserTaskEnvironment task_environment_;
  base::test::ScopedFeatureList scoped_feature_list_;
  base::HistogramTester histogram_tester_;
  TestingPrefServiceSimple local_state_;
  std::unique_ptr<AdBlockListP3A> ad_block_list_p3a_;
};

TEST_F(AdBlockListP3ATest, ReportsMetricsOnlyWhenEnabled) {
  histogram_tester_.ExpectTotalCount(kAdBlockOnlyModeEnabledHistogramName, 0);

  local_state_.SetBoolean(prefs::kAdBlockOnlyModeEnabled, true);
  histogram_tester_.ExpectUniqueSample(kAdBlockOnlyModeEnabledHistogramName, 1,
                                       1);

  task_environment_.FastForwardBy(base::Hours(5));
  histogram_tester_.ExpectUniqueSample(kAdBlockOnlyModeEnabledHistogramName, 1,
                                       2);

  local_state_.SetBoolean(prefs::kAdBlockOnlyModeEnabled, false);
  task_environment_.FastForwardBy(base::Hours(5));
  histogram_tester_.ExpectTotalCount(kAdBlockOnlyModeEnabledHistogramName, 2);
}

}  // namespace brave_shields
