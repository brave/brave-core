/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_MISC_METRICS_CAPTCHA_METRICS_H_
#define BRAVE_BROWSER_MISC_METRICS_CAPTCHA_METRICS_H_

#include <memory>

#include "base/memory/raw_ptr.h"
#include "base/timer/wall_clock_timer.h"
#include "brave/components/time_period_storage/daily_storage.h"

class GURL;
class PrefRegistrySimple;
class PrefService;

namespace page_load_metrics {
class PageLoadMetricsObserverInterface;
}  // namespace page_load_metrics

namespace misc_metrics {

enum class CaptchaProvider {
  kOther = 0,
  kGoogle = 1,
  kCloudflare = 2,
  kHCaptcha = 3
};

// This class provides the back-end implementation to record a captcha metrics
// once the captcha was detected by the BraveCaptchaPageLoadMetricsObserver.
class CaptchaMetrics {
 public:
  // Schedules the first P3A report. Does not emit on a first-ever registration.
  explicit CaptchaMetrics(PrefService* local_state);
  ~CaptchaMetrics();

  CaptchaMetrics(const CaptchaMetrics&) = delete;
  CaptchaMetrics& operator=(const CaptchaMetrics&) = delete;

  // Registers the captcha related prefs to store the histogram count.
  static void RegisterPrefs(PrefRegistrySimple* registry);

  // Returns a page-load observer when captcha metrics collection is enabled.
  static std::unique_ptr<page_load_metrics::PageLoadMetricsObserverInterface>
  CreatePageLoadMetricsObserver();

 private:
  friend class BraveCaptchaPageLoadMetricsObserver;
  friend class CaptchaMetricsTest;

  // Records a captcha for |provider| in DailyStorage. Does not emit P3A.
  void RecordCaptcha(CaptchaProvider provider);

  // Records a captcha if |url| matches a known provider. Does not emit P3A.
  void MaybeRecordCaptchaForUrl(const GURL& url);

  // Emits the last 24h counts to P3A and schedules the next report.
  void ReportCounts();

  // Seeds CaptchaProviderManager with Chromium's URL patterns when empty.
  static void EnsureDefaultCaptchaProviders();

  DailyStorage total_storage_;
  DailyStorage google_storage_;
  DailyStorage cloudflare_storage_;
  DailyStorage hcaptcha_storage_;

  base::WallClockTimer report_timer_;
  raw_ptr<PrefService> local_state_;
};

}  // namespace misc_metrics

#endif  // BRAVE_BROWSER_MISC_METRICS_CAPTCHA_METRICS_H_
