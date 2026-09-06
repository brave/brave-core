/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/misc_metrics/captcha_metrics.h"

#include <memory>

#include "base/test/metrics/histogram_tester.h"
#include "base/test/task_environment.h"
#include "base/time/time.h"
#include "brave/components/misc_metrics/common/histogram_names.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace misc_metrics {

class CaptchaMetricsTest : public testing::Test {
 public:
  CaptchaMetricsTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}

  void SetUp() override {
    CaptchaMetrics::RegisterPrefs(pref_service_.registry());
    metrics_ = std::make_unique<CaptchaMetrics>(&pref_service_);
  }

 protected:
  base::test::TaskEnvironment task_environment_;
  TestingPrefServiceSimple pref_service_;
  std::unique_ptr<CaptchaMetrics> metrics_;
  base::HistogramTester histogram_tester_;
};

TEST_F(CaptchaMetricsTest, ReportsZeroOnConstruction) {
  histogram_tester_.ExpectUniqueSample(kCaptchaCountHistogramName, 0, 1);
  histogram_tester_.ExpectUniqueSample(kCaptchaGoogleCountHistogramName, 0, 1);
  histogram_tester_.ExpectUniqueSample(kCaptchaCloudflareCountHistogramName, 0,
                                       1);
  histogram_tester_.ExpectUniqueSample(kCaptchaHCaptchaCountHistogramName, 0,
                                       1);
}

TEST_F(CaptchaMetricsTest, BucketsDailyCounts) {
  // 1
  metrics_->RecordCaptcha(CaptchaProvider::kOther);
  histogram_tester_.ExpectBucketCount(kCaptchaCountHistogramName, 1, 1);

  // 2
  metrics_->RecordCaptcha(CaptchaProvider::kOther);
  histogram_tester_.ExpectBucketCount(kCaptchaCountHistogramName, 2, 1);

  // 3-5
  for (int i = 0; i < 3; ++i) {
    metrics_->RecordCaptcha(CaptchaProvider::kOther);
  }
  histogram_tester_.ExpectBucketCount(kCaptchaCountHistogramName, 3, 3);

  // 6-10
  for (int i = 0; i < 5; ++i) {
    metrics_->RecordCaptcha(CaptchaProvider::kOther);
  }
  histogram_tester_.ExpectBucketCount(kCaptchaCountHistogramName, 4, 5);

  // 11+
  metrics_->RecordCaptcha(CaptchaProvider::kOther);
  histogram_tester_.ExpectBucketCount(kCaptchaCountHistogramName, 5, 1);
}

TEST_F(CaptchaMetricsTest, RecordsProviderCounts) {
  metrics_->RecordCaptcha(CaptchaProvider::kGoogle);
  metrics_->RecordCaptcha(CaptchaProvider::kGoogle);
  histogram_tester_.ExpectBucketCount(kCaptchaGoogleCountHistogramName, 2, 1);

  metrics_->RecordCaptcha(CaptchaProvider::kCloudflare);
  histogram_tester_.ExpectBucketCount(kCaptchaCloudflareCountHistogramName, 1,
                                      1);

  metrics_->RecordCaptcha(CaptchaProvider::kHCaptcha);
  histogram_tester_.ExpectBucketCount(kCaptchaHCaptchaCountHistogramName, 1, 1);

  metrics_->RecordCaptcha(CaptchaProvider::kOther);
  histogram_tester_.ExpectBucketCount(kCaptchaCountHistogramName, 3, 3);

  for (int i = 0; i < 4; ++i) {
    metrics_->RecordCaptcha(CaptchaProvider::kGoogle);
  }
  histogram_tester_.ExpectBucketCount(kCaptchaCountHistogramName, 4, 4);
  histogram_tester_.ExpectBucketCount(kCaptchaGoogleCountHistogramName, 4, 1);
}

TEST_F(CaptchaMetricsTest, ExpiresAfterOneDay) {
  for (int i = 0; i < 6; ++i) {
    metrics_->RecordCaptcha(CaptchaProvider::kGoogle);
  }
  histogram_tester_.ExpectBucketCount(kCaptchaCountHistogramName, 4, 1);
  histogram_tester_.ExpectBucketCount(kCaptchaGoogleCountHistogramName, 4, 1);

  task_environment_.FastForwardBy(base::Days(1));
  histogram_tester_.ExpectBucketCount(kCaptchaCountHistogramName, 0, 2);
  histogram_tester_.ExpectBucketCount(kCaptchaGoogleCountHistogramName, 0, 2);
}

}  // namespace misc_metrics
