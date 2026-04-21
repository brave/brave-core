/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/serp_metrics/test/serp_metrics_test_base.h"

#include <memory>

#include "brave/components/constants/pref_names.h"
#include "brave/components/serp_metrics/serp_metrics_feature.h"
#include "brave/components/serp_metrics/test/fake_time_period_store_factory.h"
#include "components/prefs/pref_registry_simple.h"

namespace serp_metrics {

SerpMetricsTestBase::SerpMetricsTestBase()
    : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}

SerpMetricsTestBase::~SerpMetricsTestBase() = default;

void SerpMetricsTestBase::SetUp() {
  local_state_.registry()->RegisterStringPref(kLastCheckYMD, "");
  local_state_.registry()->RegisterIntegerPref(kLastCheckWOY, 0);
  local_state_.registry()->RegisterIntegerPref(kLastCheckMonth, 0);

  // 62 days covers two back-to-back 31-day months, ensuring month-boundary
  // metrics are not dropped before reporting.
  scoped_feature_list_.InitAndEnableFeatureWithParameters(
      kSerpMetricsFeature, {{"time_period_in_days", "62"}});

  serp_metrics_ = std::make_unique<SerpMetrics>(&local_state_,
                                                FakeTimePeriodStoreFactory());
}

void SerpMetricsTestBase::AdvanceClockByRetentionPeriod() {
  task_environment_.AdvanceClock(
      base::Days(kSerpMetricsTimePeriodInDays.Get()));
}

}  // namespace serp_metrics
