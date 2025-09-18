/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/speedreader/speedreader_metrics.h"

#include <memory>

#include "base/test/metrics/histogram_tester.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_shields/core/common/pref_names.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings_pattern.h"
#include "components/prefs/testing_pref_service.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace speedreader {

class SpeedreaderMetricsTest : public testing::Test {
 public:
  SpeedreaderMetricsTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}

  void SetUp() override {
    SpeedreaderMetrics::RegisterPrefs(local_state_.registry());
    HostContentSettingsMap::RegisterProfilePrefs(profile_prefs_.registry());
    brave_shields::RegisterProfilePrefs(profile_prefs_.registry());

    host_content_settings_map_ =
        new HostContentSettingsMap(&profile_prefs_, /*is_off_the_record*/ false,
                                   /*store_last_modified*/ false,
                                   /*restore_session*/ false,
                                   /*should_record_metrics*/ false);

    metrics_ = std::make_unique<SpeedreaderMetrics>(
        &local_state_, host_content_settings_map_.get(), false);
  }

  void TearDown() override { host_content_settings_map_->ShutdownOnUIThread(); }

 protected:
  base::test::TaskEnvironment task_environment_;
  TestingPrefServiceSimple local_state_;
  sync_preferences::TestingPrefServiceSyncable profile_prefs_;
  scoped_refptr<HostContentSettingsMap> host_content_settings_map_;
  std::unique_ptr<SpeedreaderMetrics> metrics_;
  base::HistogramTester histogram_tester_;
};

TEST_F(SpeedreaderMetricsTest, RecordPageViews) {
  for (int i = 0; i < 5; i++) {
    metrics_->RecordPageView();
  }

  histogram_tester_.ExpectUniqueSample(kSpeedreaderPageViewsHistogramName, 0,
                                       5);

  for (int i = 0; i < 5; i++) {
    metrics_->RecordPageView();
  }

  histogram_tester_.ExpectBucketCount(kSpeedreaderPageViewsHistogramName, 1, 5);
  histogram_tester_.ExpectTotalCount(kSpeedreaderPageViewsHistogramName, 10);

  task_environment_.FastForwardBy(base::Days(45));

  histogram_tester_.ExpectBucketCount(kSpeedreaderPageViewsHistogramName, 1,
                                      34);
  histogram_tester_.ExpectTotalCount(kSpeedreaderPageViewsHistogramName, 39);
}

TEST_F(SpeedreaderMetricsTest, EnabledSitesMetric) {
  histogram_tester_.ExpectUniqueSample(kSpeedreaderEnabledSitesHistogramName, 0,
                                       1);

  ContentSettingsPattern pattern =
      ContentSettingsPattern::FromString("*://example.com/*");
  host_content_settings_map_->SetContentSettingCustomScope(
      pattern, ContentSettingsPattern::Wildcard(),
      ContentSettingsType::BRAVE_SPEEDREADER, CONTENT_SETTING_ALLOW);

  metrics_->UpdateEnabledSitesMetric(false);
  histogram_tester_.ExpectBucketCount(kSpeedreaderEnabledSitesHistogramName, 1,
                                      1);

  pattern = ContentSettingsPattern::FromString("*://brave.com/*");
  host_content_settings_map_->SetContentSettingCustomScope(
      pattern, ContentSettingsPattern::Wildcard(),
      ContentSettingsType::BRAVE_SPEEDREADER, CONTENT_SETTING_ALLOW);

  metrics_->UpdateEnabledSitesMetric(false);
  histogram_tester_.ExpectBucketCount(kSpeedreaderEnabledSitesHistogramName, 2,
                                      1);

  metrics_->UpdateEnabledSitesMetric(true);
  histogram_tester_.ExpectBucketCount(kSpeedreaderEnabledSitesHistogramName, 3,
                                      1);
}

}  // namespace speedreader
