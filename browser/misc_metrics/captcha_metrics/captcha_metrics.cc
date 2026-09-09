/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/misc_metrics/captcha_metrics/captcha_metrics.h"

#include <optional>

#include "base/check.h"
#include "base/time/time.h"
#include "base/values.h"
#include "brave/browser/brave_browser_process.h"
#include "brave/browser/misc_metrics/process_misc_metrics.h"
#include "brave/components/misc_metrics/pref_names.h"
#include "brave/components/p3a_utils/bucket.h"
#include "chrome/browser/page_load_metrics/observers/captcha_provider_manager.h"
#include "components/page_load_metrics/browser/page_load_metrics_observer.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/navigation_handle.h"
#include "url/gurl.h"

namespace misc_metrics {

namespace {

// Keys to the dictionary pref.
inline constexpr char kMiscMetricsCaptchaCount[] =
    "brave.misc_metrics.captcha_count";
inline constexpr char kMiscMetricsCaptchaGoogleCount[] =
    "brave.misc_metrics.captcha_google_count";
inline constexpr char kMiscMetricsCaptchaCloudflareCount[] =
    "brave.misc_metrics.captcha_cloudflare_count";
inline constexpr char kMiscMetricsCaptchaHCaptchaCount[] =
    "brave.misc_metrics.captcha_hcaptcha_count";

constexpr base::TimeDelta kReportInterval = base::Days(1);
// 0, 1, 2, 3-5, 6-10, 11+
constexpr int kCaptchaCountBuckets[] = {0, 1, 2, 5, 10};

}  // namespace

class BraveCaptchaPageLoadMetricsObserver
    : public page_load_metrics::PageLoadMetricsObserver {
 public:
  explicit BraveCaptchaPageLoadMetricsObserver(CaptchaMetrics* captcha_metrics)
      : captcha_metrics_(captcha_metrics) {}

  const char* GetObserverName() const override {
    static const char kName[] = "BraveCaptchaPageLoadMetricsObserver";
    return kName;
  }

  ObservePolicy OnPrerenderStart(content::NavigationHandle*,
                                 const GURL&) override {
    // Brave disables prerender. If this runs, captcha metrics need a real
    // prerender policy instead of ignoring the page.
    DCHECK(false) << "OnPrerenderStart called; prerender is disabled in Brave.";
    return STOP_OBSERVING;
  }

  ObservePolicy OnFencedFramesStart(content::NavigationHandle*,
                                    const GURL&) override {
    // Brave disables fenced frames. If this runs, captcha metrics need a real
    // fenced-frame policy instead of ignoring the page.
    DCHECK(false)
        << "OnFencedFramesStart called; fenced frames are disabled in Brave.";
    return STOP_OBSERVING;
  }

  // Full-page captchas loaded in the top level frame.
  ObservePolicy OnCommit(
      content::NavigationHandle* navigation_handle) override {
    captcha_metrics_->MaybeRecordCaptchaForUrl(navigation_handle->GetURL());
    return CONTINUE_OBSERVING;
  }

  // Captcha's loaded in a subframe.
  void OnDidFinishSubFrameNavigation(
      content::NavigationHandle* navigation_handle) override {
    if (!navigation_handle->HasCommitted()) {
      return;
    }

    // Check if the mainframe already has a captcha provider, if so then skip
    // recording for embedded iframes. This is the situation where we are on
    // a fullpage captcha which may embed iframes with the same origin. We would
    // not want to double count here.
    if (page_load_metrics::CaptchaProviderManager::GetInstance()
            ->GetCaptchaProviderForUrl(GetDelegate().GetUrl())
            .has_value()) {
      return;
    }

    captcha_metrics_->MaybeRecordCaptchaForUrl(navigation_handle->GetURL());
  }

 private:
  raw_ptr<CaptchaMetrics> captcha_metrics_;
};

CaptchaMetrics::CaptchaMetrics(PrefService* local_state)
    : local_state_(local_state) {
  ReportCounts();
}

CaptchaMetrics::~CaptchaMetrics() = default;

void CaptchaMetrics::RegisterPrefs(PrefRegistrySimple* registry) {
  registry->RegisterDictionaryPref(kMiscMetricsCaptchaDictionaryPref, {});
  registry->RegisterTimePref(kMiscMetricsCaptchaLastRecordTime, {});
}

std::unique_ptr<page_load_metrics::PageLoadMetricsObserverInterface>
CaptchaMetrics::CreatePageLoadMetricsObserver() {
  if (!g_brave_browser_process ||
      !g_brave_browser_process->process_misc_metrics() ||
      !g_brave_browser_process->process_misc_metrics()->captcha_metrics()) {
    return nullptr;
  }

  EnsureDefaultCaptchaProviders();
  return std::make_unique<BraveCaptchaPageLoadMetricsObserver>(
      g_brave_browser_process->process_misc_metrics()->captcha_metrics());
}

void CaptchaMetrics::EnsureDefaultCaptchaProviders() {
  auto* manager = page_load_metrics::CaptchaProviderManager::GetInstance();
  if (!manager->empty()) {
    return;
  }
  // List taken from Chromium's Captcha Providers component. Brave blocks that
  // CRX, so load the same URL patterns locally.
  manager->SetCaptchaProviders({
      "*google.com/recaptcha/api2/anchor",
      "*google.com/recaptcha/api2/bframe",
      "*google.com/recaptcha/enterprise/anchor",
      "*google.com/recaptcha/enterprise/bframe",
      "*recaptcha.net/recaptcha/api2/anchor",
      "*recaptcha.net/recaptcha/api2/bframe",
      "*recaptcha.net/recaptcha/enterprise/anchor",
      "*recaptcha.net/recaptcha/enterprise/bframe",
      "*hcaptcha.com/captcha/*",
      "*challenges.cloudflare.com/*",
  });
}

void CaptchaMetrics::RecordCaptcha(CaptchaProvider provider) {
  base::DictValue dict =
      local_state_->GetDict(kMiscMetricsCaptchaDictionaryPref).Clone();
  auto increment = [&dict](const char* key) {
    dict.Set(key, dict.FindInt(key).value_or(0) + 1);
  };

  increment(kMiscMetricsCaptchaCount);

  switch (provider) {
    case CaptchaProvider::kGoogle:
      increment(kMiscMetricsCaptchaGoogleCount);
      break;
    case CaptchaProvider::kCloudflare:
      increment(kMiscMetricsCaptchaCloudflareCount);
      break;
    case CaptchaProvider::kHCaptcha:
      increment(kMiscMetricsCaptchaHCaptchaCount);
      break;
    case CaptchaProvider::kOther:
      break;
  }

  local_state_->SetDict(kMiscMetricsCaptchaDictionaryPref, std::move(dict));
}

void CaptchaMetrics::MaybeRecordCaptchaForUrl(const GURL& url) {
  std::optional<page_load_metrics::CaptchaProvider> captcha_provider =
      page_load_metrics::CaptchaProviderManager::GetInstance()
          ->GetCaptchaProviderForUrl(url);
  if (!captcha_provider.has_value()) {
    return;
  }

  CaptchaProvider provider = CaptchaProvider::kOther;
  switch (*captcha_provider) {
    case page_load_metrics::CaptchaProvider::kReCaptcha:
      provider = CaptchaProvider::kGoogle;
      break;
    case page_load_metrics::CaptchaProvider::kCloudflareTurnstile:
      provider = CaptchaProvider::kCloudflare;
      break;
    case page_load_metrics::CaptchaProvider::kHCaptcha:
      provider = CaptchaProvider::kHCaptcha;
      break;
    case page_load_metrics::CaptchaProvider::kUnknown:
      break;
  }
  RecordCaptcha(provider);
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
    const base::DictValue& counts =
        local_state_->GetDict(kMiscMetricsCaptchaDictionaryPref);
    p3a_utils::RecordToHistogramBucket(
        kCaptchaTotalCountHistogramName, kCaptchaCountBuckets,
        counts.FindInt(kMiscMetricsCaptchaCount).value_or(0));
    p3a_utils::RecordToHistogramBucket(
        kCaptchaGoogleCountHistogramName, kCaptchaCountBuckets,
        counts.FindInt(kMiscMetricsCaptchaGoogleCount).value_or(0));
    p3a_utils::RecordToHistogramBucket(
        kCaptchaCloudflareCountHistogramName, kCaptchaCountBuckets,
        counts.FindInt(kMiscMetricsCaptchaCloudflareCount).value_or(0));
    p3a_utils::RecordToHistogramBucket(
        kCaptchaHCaptchaCountHistogramName, kCaptchaCountBuckets,
        counts.FindInt(kMiscMetricsCaptchaHCaptchaCount).value_or(0));
    // Re-initialize the dict.
    local_state_->SetDict(kMiscMetricsCaptchaDictionaryPref, {});
  }

  // Update the last recorded time to now, and start the timer.
  local_state_->SetTime(kMiscMetricsCaptchaLastRecordTime, now);
  report_timer_.Start(FROM_HERE, now + kReportInterval, this,
                      &CaptchaMetrics::ReportCounts);
}

}  // namespace misc_metrics

std::unique_ptr<page_load_metrics::PageLoadMetricsObserverInterface>
BraveCreateCaptchaPageLoadMetricsObserver() {
  return misc_metrics::CaptchaMetrics::CreatePageLoadMetricsObserver();
}
