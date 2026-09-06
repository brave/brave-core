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

    // Construction sets last-report time to t=0 without emitting. Events
    // recorded at t=0 would be dropped when the timer fires at t=24h
    // (DailyStorage excludes timestamps <= now-24h). Advance, so later
    // RecordCaptcha() calls stay inside [1,24*60*60] window.
    task_environment_.FastForwardBy(base::Seconds(1));
  }

  void ExpectNoSamples() {
    histogram_tester_.ExpectTotalCount(kCaptchaTotalCountHistogramName, 0);
    histogram_tester_.ExpectTotalCount(kCaptchaGoogleCountHistogramName, 0);
    histogram_tester_.ExpectTotalCount(kCaptchaCloudflareCountHistogramName, 0);
    histogram_tester_.ExpectTotalCount(kCaptchaHCaptchaCountHistogramName, 0);
  }

 protected:
  base::test::TaskEnvironment task_environment_;
  TestingPrefServiceSimple pref_service_;
  std::unique_ptr<CaptchaMetrics> metrics_;
  base::HistogramTester histogram_tester_;
};

TEST_F(CaptchaMetricsTest, DoesNotReportOnConstruction) {
  ExpectNoSamples();
}

TEST_F(CaptchaMetricsTest, DoesNotReportUntilInterval) {
  metrics_->RecordCaptcha(CaptchaProvider::kGoogle);
  metrics_->RecordCaptcha(CaptchaProvider::kCloudflare);
  metrics_->RecordCaptcha(CaptchaProvider::kHCaptcha);
  metrics_->RecordCaptcha(CaptchaProvider::kOther);

  ExpectNoSamples();
}

TEST_F(CaptchaMetricsTest, DoesNotRereportOnRestartWithinInterval) {
  metrics_ = std::make_unique<CaptchaMetrics>(&pref_service_);

  ExpectNoSamples();
}

TEST_F(CaptchaMetricsTest, BucketsDailyCounts) {
  auto record_and_report = [this](int count) {
    for (int i = 0; i < count; ++i) {
      metrics_->RecordCaptcha(CaptchaProvider::kOther);
    }
    task_environment_.FastForwardBy(base::Days(1));
  };

  // 1
  record_and_report(1);
  histogram_tester_.ExpectBucketCount(kCaptchaTotalCountHistogramName, 1, 1);
  // Move to the next window.
  task_environment_.FastForwardBy(base::Seconds(1));

  // 2
  record_and_report(2);
  histogram_tester_.ExpectBucketCount(kCaptchaTotalCountHistogramName, 2, 1);
  // Move to the next window.
  task_environment_.FastForwardBy(base::Seconds(1));

  // 3-5
  record_and_report(5);
  histogram_tester_.ExpectBucketCount(kCaptchaTotalCountHistogramName, 3, 1);
  // Move to the next window.
  task_environment_.FastForwardBy(base::Seconds(1));

  // 6-10
  record_and_report(10);
  histogram_tester_.ExpectBucketCount(kCaptchaTotalCountHistogramName, 4, 1);
  // Move to the next window.
  task_environment_.FastForwardBy(base::Seconds(1));

  // 11+
  record_and_report(11);
  histogram_tester_.ExpectBucketCount(kCaptchaTotalCountHistogramName, 5, 1);
}

TEST_F(CaptchaMetricsTest, RecordsProviderCounts) {
  metrics_->RecordCaptcha(CaptchaProvider::kGoogle);
  metrics_->RecordCaptcha(CaptchaProvider::kGoogle);
  metrics_->RecordCaptcha(CaptchaProvider::kCloudflare);
  metrics_->RecordCaptcha(CaptchaProvider::kHCaptcha);
  metrics_->RecordCaptcha(CaptchaProvider::kOther);

  task_environment_.FastForwardBy(base::Days(1));

  // total=5 → 3-5, google=2, cloudflare=1, hcaptcha=1
  histogram_tester_.ExpectBucketCount(kCaptchaTotalCountHistogramName, 3, 1);
  histogram_tester_.ExpectBucketCount(kCaptchaGoogleCountHistogramName, 2, 1);
  histogram_tester_.ExpectBucketCount(kCaptchaCloudflareCountHistogramName, 1,
                                      1);
  histogram_tester_.ExpectBucketCount(kCaptchaHCaptchaCountHistogramName, 1, 1);
}

TEST_F(CaptchaMetricsTest, ExpiresAfterOneDay) {
  for (int i = 0; i < 6; ++i) {
    metrics_->RecordCaptcha(CaptchaProvider::kGoogle);
  }
  task_environment_.FastForwardBy(base::Days(1));
  histogram_tester_.ExpectBucketCount(kCaptchaTotalCountHistogramName, 4, 1);
  histogram_tester_.ExpectBucketCount(kCaptchaGoogleCountHistogramName, 4, 1);

  task_environment_.FastForwardBy(base::Days(1));
  histogram_tester_.ExpectBucketCount(kCaptchaTotalCountHistogramName, 0, 1);
  histogram_tester_.ExpectBucketCount(kCaptchaGoogleCountHistogramName, 0, 1);
}

}  // namespace misc_metrics
