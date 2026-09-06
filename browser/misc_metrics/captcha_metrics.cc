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
#include "components/prefs/pref_service.h"

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
      hcaptcha_storage_(local_state, kMiscMetricsCaptchaHCaptchaCount),
      local_state_(local_state) {
  ReportCounts();
}

CaptchaMetrics::~CaptchaMetrics() = default;

void CaptchaMetrics::RegisterPrefs(PrefRegistrySimple* registry) {
  registry->RegisterListPref(kMiscMetricsCaptchaCount);
  registry->RegisterListPref(kMiscMetricsCaptchaGoogleCount);
  registry->RegisterListPref(kMiscMetricsCaptchaCloudflareCount);
  registry->RegisterListPref(kMiscMetricsCaptchaHCaptchaCount);
  registry->RegisterTimePref(kMiscMetricsCaptchaLastRecordTime, {});
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
}

void CaptchaMetrics::ReportCounts() {
  base::Time now = base::Time::Now();
  base::Time last_recorded_time =
      local_state_->GetTime(kMiscMetricsCaptchaLastRecordTime);

  // We have already collected the metrics at the |last_recorded_time| but
  // if hit this condition, it means the browser process was destroyed
  // in between. So, we need to re-schedule the timer again to capture the
  // reports.
  if (!last_recorded_time.is_null() &&
      now - last_recorded_time < kReportInterval) {
    report_timer_.Start(FROM_HERE, last_recorded_time + kReportInterval, this,
                        &CaptchaMetrics::ReportCounts);
    return;
  }

  // In the first ever recorded run, last_recorded_time is null and so are the
  // various captcha storages. So, we can skip emitting as it doesn't reflect no
  // captchas were seen.
  if (!last_recorded_time.is_null()) {
    p3a_utils::RecordToHistogramBucket(
        kCaptchaTotalCountHistogramName, kCaptchaCountBuckets,
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
  }

  // Update the last recorded time to now, and start the timer.
  local_state_->SetTime(kMiscMetricsCaptchaLastRecordTime, now);
  report_timer_.Start(FROM_HERE, now + kReportInterval, this,
                      &CaptchaMetrics::ReportCounts);
}

}  // namespace misc_metrics
