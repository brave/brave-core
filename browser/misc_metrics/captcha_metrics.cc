/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/misc_metrics/captcha_metrics.h"

#include "base/time/time.h"
#include "brave/components/misc_metrics/common/histogram_names.h"
#include "brave/components/misc_metrics/pref_names.h"
#include "brave/components/p3a_utils/bucket.h"
#include "components/prefs/pref_registry_simple.h"

namespace misc_metrics {

namespace {

constexpr base::TimeDelta kReportInterval = base::Days(1);
// 0, 1, 2, 3-5, 6-10, 11+
constexpr int kCaptchaCountBuckets[] = {0, 1, 2, 5, 10};

}  // namespace

CaptchaMetrics::CaptchaMetrics(PrefService* local_state)
    : total_storage_(local_state, kMiscMetricsCaptchaCount),
      google_storage_(local_state, kMiscMetricsCaptchaGoogleCount),
      cloudflare_storage_(local_state, kMiscMetricsCaptchaCloudflareCount),
      hcaptcha_storage_(local_state, kMiscMetricsCaptchaHCaptchaCount) {
  ReportCounts();
}

CaptchaMetrics::~CaptchaMetrics() = default;

void CaptchaMetrics::RegisterPrefs(PrefRegistrySimple* registry) {
  registry->RegisterListPref(kMiscMetricsCaptchaCount);
  registry->RegisterListPref(kMiscMetricsCaptchaGoogleCount);
  registry->RegisterListPref(kMiscMetricsCaptchaCloudflareCount);
  registry->RegisterListPref(kMiscMetricsCaptchaHCaptchaCount);
}

void CaptchaMetrics::RecordCaptcha(CaptchaProvider provider) {
  total_storage_.RecordValueNow(1);
  switch (provider) {
    case CaptchaProvider::kGoogle:
      google_storage_.RecordValueNow(1);
      break;
    case CaptchaProvider::kCloudflare:
      cloudflare_storage_.RecordValueNow(1);
      break;
    case CaptchaProvider::kHCaptcha:
      hcaptcha_storage_.RecordValueNow(1);
      break;
    case CaptchaProvider::kOther:
      break;
  }
  ReportCounts();
}

void CaptchaMetrics::ReportCounts() {
  p3a_utils::RecordToHistogramBucket(
      kCaptchaCountHistogramName, kCaptchaCountBuckets,
      static_cast<int>(total_storage_.GetLast24HourSum()));
  p3a_utils::RecordToHistogramBucket(
      kCaptchaGoogleCountHistogramName, kCaptchaCountBuckets,
      static_cast<int>(google_storage_.GetLast24HourSum()));
  p3a_utils::RecordToHistogramBucket(
      kCaptchaCloudflareCountHistogramName, kCaptchaCountBuckets,
      static_cast<int>(cloudflare_storage_.GetLast24HourSum()));
  p3a_utils::RecordToHistogramBucket(
      kCaptchaHCaptchaCountHistogramName, kCaptchaCountBuckets,
      static_cast<int>(hcaptcha_storage_.GetLast24HourSum()));
  report_timer_.Start(FROM_HERE, base::Time::Now() + kReportInterval, this,
                      &CaptchaMetrics::ReportCounts);
}

}  // namespace misc_metrics
