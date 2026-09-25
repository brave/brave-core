/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/misc_metrics/captcha_metrics.h"

#include <memory>

#include "base/test/metrics/histogram_tester.h"
#include "base/time/time.h"
#include "base/values.h"
#include "brave/components/misc_metrics/pref_names.h"
#include "components/prefs/testing_pref_service.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace misc_metrics {

namespace {

GURL GoogleCaptchaUrl() {
  return GURL("https://www.google.com/recaptcha/api2/anchor");
}

GURL CloudflareCaptchaUrl() {
  return GURL(
      "https://challenges.cloudflare.com/cdn-cgi/challenge-platform/turnstile");
}

GURL HCaptchaUrl() {
  return GURL("https://www.hcaptcha.com/captcha/index.html");
}

GURL NonCaptchaUrl() {
  return GURL("https://example.com/simple.html");
}

}  // namespace

class CaptchaMetricsTest : public testing::Test {
 public:
  CaptchaMetricsTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}

  void SetUp() override {
    CaptchaMetrics::RegisterPrefs(pref_service_.registry());
    metrics_ = std::make_unique<CaptchaMetrics>(&pref_service_);
    metrics_->EnsureDefaultCaptchaProviders();

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

  void MaybeRecordCaptchaForUrl(const GURL& url,
                                bool is_user_activated = false) {
    metrics_->MaybeRecordCaptchaForUrl(url, is_user_activated);
  }

 protected:
  content::BrowserTaskEnvironment task_environment_;
  TestingPrefServiceSimple pref_service_;
  std::unique_ptr<CaptchaMetrics> metrics_;
  base::HistogramTester histogram_tester_;
};

TEST_F(CaptchaMetricsTest, DoesNotReportOnConstruction) {
  ExpectNoSamples();
}

TEST_F(CaptchaMetricsTest, DoesNotReportUntilInterval) {
  MaybeRecordCaptchaForUrl(GoogleCaptchaUrl());
  MaybeRecordCaptchaForUrl(CloudflareCaptchaUrl());
  MaybeRecordCaptchaForUrl(HCaptchaUrl());

  ExpectNoSamples();
}

TEST_F(CaptchaMetricsTest, DoesNotRecordNonCaptchaUrl) {
  MaybeRecordCaptchaForUrl(NonCaptchaUrl());
  task_environment_.FastForwardBy(base::Days(1));

  ExpectNoSamples();
}

TEST_F(CaptchaMetricsTest, DoesNotRereportOnRestartWithinInterval) {
  metrics_ = std::make_unique<CaptchaMetrics>(&pref_service_);

  ExpectNoSamples();
}

TEST_F(CaptchaMetricsTest, BucketsDailyCounts) {
  auto record_and_report = [this](int count) {
    for (int i = 0; i < count; ++i) {
      MaybeRecordCaptchaForUrl(GoogleCaptchaUrl());
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
  MaybeRecordCaptchaForUrl(GoogleCaptchaUrl());
  MaybeRecordCaptchaForUrl(GoogleCaptchaUrl());
  MaybeRecordCaptchaForUrl(CloudflareCaptchaUrl());
  MaybeRecordCaptchaForUrl(HCaptchaUrl());

  task_environment_.FastForwardBy(base::Days(1));

  // total=4 → 3-5, google=2, cloudflare=1, hcaptcha=1
  histogram_tester_.ExpectBucketCount(kCaptchaTotalCountHistogramName, 3, 1);
  histogram_tester_.ExpectBucketCount(kCaptchaGoogleCountHistogramName, 2, 1);
  histogram_tester_.ExpectBucketCount(kCaptchaCloudflareCountHistogramName, 1,
                                      1);
  histogram_tester_.ExpectBucketCount(kCaptchaHCaptchaCountHistogramName, 1, 1);
}

TEST_F(CaptchaMetricsTest, RecordsUserActivatedCounts) {
  // Two google captchas shown; only one is interacted with by the user.
  MaybeRecordCaptchaForUrl(GoogleCaptchaUrl());
  MaybeRecordCaptchaForUrl(GoogleCaptchaUrl(), /*is_user_activated=*/true);

  task_environment_.FastForwardBy(base::Days(1));

  histogram_tester_.ExpectBucketCount(kCaptchaTotalCountHistogramName, 1, 1);
  histogram_tester_.ExpectBucketCount(kCaptchaGoogleCountHistogramName, 1, 1);

  // User-activated counts include only the interacted-with load.
  histogram_tester_.ExpectBucketCount(
      kCaptchaTotalCountUserActivatedHistogramName, 1, 1);
  histogram_tester_.ExpectBucketCount(
      kCaptchaGoogleCountUserActivatedHistogramName, 1, 1);

  // Providers that were never interacted with emit nothing on the
  // user-activated series.
  histogram_tester_.ExpectTotalCount(
      kCaptchaCloudflareCountUserActivatedHistogramName, 0);
  histogram_tester_.ExpectTotalCount(
      kCaptchaHCaptchaCountUserActivatedHistogramName, 0);
}

TEST_F(CaptchaMetricsTest, StoresCountsInNestedProviderDict) {
  // One activated + one non-activated Google captcha, plus one Cloudflare.
  MaybeRecordCaptchaForUrl(GoogleCaptchaUrl());
  MaybeRecordCaptchaForUrl(GoogleCaptchaUrl(), /*is_user_activated=*/true);
  MaybeRecordCaptchaForUrl(CloudflareCaptchaUrl());

  // Counts are stored nested per-provider, keyed by provider with inner
  // "total" / "user_activated" fields, rather than as flat dotted keys.
  const base::DictValue& counts =
      pref_service_.GetDict(kMiscMetricsCaptchaDictionaryPref);

  // "all": total counts every non-activated load (google + cloudflare = 2);
  // user_activated counts the single interacted-with load.
  const base::DictValue* all = counts.FindDict("all");
  ASSERT_TRUE(all);
  EXPECT_EQ(all->FindInt("total"), 2);
  EXPECT_EQ(all->FindInt("user_activated"), 1);

  const base::DictValue* google = counts.FindDict("google");
  ASSERT_TRUE(google);
  EXPECT_EQ(google->FindInt("total"), 1);
  EXPECT_EQ(google->FindInt("user_activated"), 1);

  // Cloudflare was never interacted with, so it has no user_activated field.
  const base::DictValue* cloudflare = counts.FindDict("cloudflare");
  ASSERT_TRUE(cloudflare);
  EXPECT_EQ(cloudflare->FindInt("total"), 1);
  EXPECT_FALSE(cloudflare->FindInt("user_activated").has_value());

  // Providers with no captchas seen have no entry at all.
  EXPECT_FALSE(counts.FindDict("hcaptcha"));
}

TEST_F(CaptchaMetricsTest, ExpiresAfterOneDay) {
  for (int i = 0; i < 6; ++i) {
    MaybeRecordCaptchaForUrl(GoogleCaptchaUrl());
  }
  task_environment_.FastForwardBy(base::Days(1));
  histogram_tester_.ExpectBucketCount(kCaptchaTotalCountHistogramName, 4, 1);
  histogram_tester_.ExpectBucketCount(kCaptchaGoogleCountHistogramName, 4, 1);

  // An empty window does not emit; the previous day's samples are unchanged.
  task_environment_.FastForwardBy(base::Days(1));
  histogram_tester_.ExpectTotalCount(kCaptchaTotalCountHistogramName, 1);
  histogram_tester_.ExpectTotalCount(kCaptchaGoogleCountHistogramName, 1);
}

}  // namespace misc_metrics
