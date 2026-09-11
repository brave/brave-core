/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_MISC_METRICS_CAPTCHA_METRICS_H_
#define BRAVE_BROWSER_MISC_METRICS_CAPTCHA_METRICS_H_

#include <memory>

#include "base/memory/raw_ptr.h"
#include "base/timer/wall_clock_timer.h"

class GURL;
class PrefRegistrySimple;
class PrefService;
class Profile;

namespace page_load_metrics {
class PageLoadMetricsObserverInterface;
}  // namespace page_load_metrics

namespace misc_metrics {

inline constexpr char kCaptchaTotalCountHistogramName[] =
    "Brave.CaptchaCount.Total";
inline constexpr char kCaptchaGoogleCountHistogramName[] =
    "Brave.CaptchaCount.Google";
inline constexpr char kCaptchaCloudflareCountHistogramName[] =
    "Brave.CaptchaCount.Cloudflare";
inline constexpr char kCaptchaHCaptchaCountHistogramName[] =
    "Brave.CaptchaCount.hCaptcha";

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

  // Returns a page-load observer when captcha metrics collection is enabled
  // and |profile| is a regular profile. Returns nullptr otherwise.
  static std::unique_ptr<page_load_metrics::PageLoadMetricsObserverInterface>
  CreatePageLoadMetricsObserver(Profile* profile);

 private:
  friend class BraveCaptchaPageLoadMetricsObserver;
  friend class CaptchaMetricsBrowserTest;
  friend class CaptchaMetricsTest;

  // Seeds CaptchaProviderManager with Chromium's URL patterns when empty.
  static void EnsureDefaultCaptchaProviders();

  // Records a captcha if |url| matches a known provider. Does not emit P3A.
  void MaybeRecordCaptchaForUrl(const GURL& url);

  // Emits the last 24h counts to P3A and schedules the next report.
  // Reports a histogram for a corresponding captcha provider iff the count was
  // non zero.
  void ReportToP3AIfPossible();

  // The timer to help schedule the next reporting.
  base::WallClockTimer report_timer_;
  raw_ptr<PrefService> local_state_;
};

}  // namespace misc_metrics

#endif  // BRAVE_BROWSER_MISC_METRICS_CAPTCHA_METRICS_H_
