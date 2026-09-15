/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/misc_metrics/captcha_metrics.h"

#include <optional>
#include <string_view>

#include "base/check.h"
#include "base/containers/fixed_flat_map.h"
#include "base/notreached.h"
#include "base/time/time.h"
#include "base/values.h"
#include "brave/browser/brave_browser_process.h"
#include "brave/browser/misc_metrics/process_misc_metrics.h"
#include "brave/components/misc_metrics/pref_names.h"
#include "brave/components/p3a_utils/bucket.h"
#include "chrome/browser/page_load_metrics/observers/captcha_provider_manager.h"
#include "chrome/browser/profiles/profile.h"
#include "components/page_load_metrics/browser/page_load_metrics_observer.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/navigation_handle.h"
#include "url/gurl.h"

namespace misc_metrics {

namespace {
constexpr base::TimeDelta kReportInterval = base::Days(1);
// 0, 1, 2, 3-5, 6-10, 11+
constexpr int kCaptchaCountBuckets[] = {0, 1, 2, 5, 10};

// The corresponding prefs in the dictionary kMiscMetricsCaptchaDictionaryPref
// "brave.misc_metrics.captcha_dict".
inline constexpr char kCaptchaTotalCountPref[] =
    "brave.misc_metrics.captcha_count";
inline constexpr char kCaptchaTotalCountUserActivatedPref[] =
    "brave.misc_metrics.captcha_count_user_activated";

inline constexpr char kCaptchaGoogleCountPref[] =
    "brave.misc_metrics.captcha_google_count";
inline constexpr char kCaptchaGoogleCountUserActivatedPref[] =
    "brave.misc_metrics.captcha_google_count_user_activated";

inline constexpr char kCaptchaCloudflareCountPref[] =
    "brave.misc_metrics.captcha_cloudflare_count";
inline constexpr char kCaptchaCloudflareCountUserActivatedPref[] =
    "brave.misc_metrics.captcha_cloudflare_count_user_activated";

inline constexpr char kCaptchaHCaptchaCountPref[] =
    "brave.misc_metrics.captcha_hcaptcha_count";
inline constexpr char kCaptchaHCaptchaCountUserActivatedPref[] =
    "brave.misc_metrics.captcha_hcaptcha_count_user_activated";

constexpr auto kHistogramToPrefMap =
    base::MakeFixedFlatMap<std::string_view, std::string_view>({
        {kCaptchaTotalCountHistogramName, kCaptchaTotalCountPref},
        {kCaptchaTotalCountUserActivatedHistogramName,
         kCaptchaTotalCountUserActivatedPref},
        {kCaptchaGoogleCountHistogramName, kCaptchaGoogleCountPref},
        {kCaptchaGoogleCountUserActivatedHistogramName,
         kCaptchaGoogleCountUserActivatedPref},
        {kCaptchaCloudflareCountHistogramName, kCaptchaCloudflareCountPref},
        {kCaptchaCloudflareCountUserActivatedHistogramName,
         kCaptchaCloudflareCountUserActivatedPref},
        {kCaptchaHCaptchaCountHistogramName, kCaptchaHCaptchaCountPref},
        {kCaptchaHCaptchaCountUserActivatedHistogramName,
         kCaptchaHCaptchaCountUserActivatedPref},
    });
}  // namespace

class BraveCaptchaPageLoadMetricsObserver
    : public page_load_metrics::PageLoadMetricsObserver {
 public:
  explicit BraveCaptchaPageLoadMetricsObserver(CaptchaMetrics* captcha_metrics)
      : captcha_metrics_(captcha_metrics) {}

 private:
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

  // This is called each time the user interacts with the captcha frame.
  void FrameReceivedUserActivation(
      content::RenderFrameHost* render_frame_host) override {
    // This helps to avoid re-recording the metrics on other user activation
    // input like mouse events. We don't explicitly check the click event as
    // it's not straightforward to isolate that. However, simply checking on a
    // general user activation should be good enough to detect captcha checks
    // which required user to interact with the frame.
    if (recorded_user_activation_) {
      return;
    }

    const GURL url = render_frame_host->GetLastCommittedURL();
    std::optional<page_load_metrics::CaptchaProvider> captcha_provider =
        page_load_metrics::CaptchaProviderManager::GetInstance()
            ->GetCaptchaProviderForUrl(url);
    if (captcha_provider.has_value()) {
      recorded_user_activation_ = true;
      captcha_metrics_->MaybeRecordCaptchaForUrl(url,
                                                 /*is_user_activated= */ true);
    }
  }

  bool recorded_user_activation_ = false;
  raw_ptr<CaptchaMetrics> captcha_metrics_;
};

CaptchaMetrics::CaptchaMetrics(PrefService* local_state)
    : local_state_(local_state) {
  ReportToP3AIfPossible();
}

CaptchaMetrics::~CaptchaMetrics() = default;

// static
void CaptchaMetrics::RegisterPrefs(PrefRegistrySimple* registry) {
  registry->RegisterDictionaryPref(kMiscMetricsCaptchaDictionaryPref, {});
  registry->RegisterTimePref(kMiscMetricsCaptchaLastRecordTime, {});
}

// static
std::unique_ptr<page_load_metrics::PageLoadMetricsObserverInterface>
CaptchaMetrics::CreatePageLoadMetricsObserver(Profile* profile) {
  if (!profile || !profile->IsRegularProfile()) {
    return nullptr;
  }

  if (!g_brave_browser_process ||
      !g_brave_browser_process->process_misc_metrics() ||
      !g_brave_browser_process->process_misc_metrics()->captcha_metrics()) {
    return nullptr;
  }

  EnsureDefaultCaptchaProviders();
  return std::make_unique<BraveCaptchaPageLoadMetricsObserver>(
      g_brave_browser_process->process_misc_metrics()->captcha_metrics());
}

// static
void CaptchaMetrics::EnsureDefaultCaptchaProviders() {
  auto* manager = page_load_metrics::CaptchaProviderManager::GetInstance();
  if (!manager->empty()) {
    return;
  }
  // List taken from Chromium's Captcha Providers component. Brave blocks that
  // CRX, so load the same URL patterns locally.
  //
  // Note, we differ from the upstream by not intercepting the bframe for
  // Google's recaptcha. This is done to avoid double counting in
  // OnDidFinishSubFrameNavigation. Recaptcha embeds two widgets - 1) not a
  // robot checkbox (which gets loaded via /anchor) and the 3x3 image tiles
  // (loaded via /bframe). Therefore, hooking only on the "/anchor" is enough
  // for the current use-case.
  manager->SetCaptchaProviders({
      "*google.com/recaptcha/api2/anchor",
      "*google.com/recaptcha/enterprise/anchor",
      "*recaptcha.net/recaptcha/api2/anchor",
      "*recaptcha.net/recaptcha/enterprise/anchor",
      "*hcaptcha.com/captcha/*",
      "*challenges.cloudflare.com/*",
  });
}

void CaptchaMetrics::MaybeRecordCaptchaForUrl(const GURL& url,
                                              const bool is_user_activated) {
  // ScopedDictPrefUpdate needs to be run on UI thread.
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  std::optional<page_load_metrics::CaptchaProvider> captcha_provider =
      page_load_metrics::CaptchaProviderManager::GetInstance()
          ->GetCaptchaProviderForUrl(url);
  // Note that for Cloudflare, the Captcha providers only matches if a frame
  // document was navigated to a URL matching "*challenges.cloudflare.com/*"
  // which is the complete turnstile check.
  //
  //  However, Cloudflare also provides a lightweight technique for security
  //  checks via their javascript detections solution which are scripts embedded
  //  directly in the same origin and is located in
  //  "<origin>/cdn-cgi/challenge-platform/...". To observe that, we need to
  //  hook into WebContentsObserver and observe the resource load events.
  //
  // See
  // https://developers.cloudflare.com/cloudflare-challenges/challenge-types/javascript-detections/
  // for more details.
  if (!captcha_provider.has_value()) {
    return;
  }

  ScopedDictPrefUpdate update(local_state_, kMiscMetricsCaptchaDictionaryPref);
  auto increment = [&update, &is_user_activated](
                       const char* pref, const char* user_activated_pref) {
    update->Set(pref, update->FindInt(pref).value_or(0) + 1);
    if (is_user_activated) {
      update->Set(user_activated_pref,
                  update->FindInt(user_activated_pref).value_or(0) + 1);
    }
  };

  increment(kCaptchaTotalCountPref, kCaptchaTotalCountUserActivatedPref);

  switch (*captcha_provider) {
    case page_load_metrics::CaptchaProvider::kReCaptcha:
      increment(kCaptchaGoogleCountPref, kCaptchaGoogleCountUserActivatedPref);
      return;
    case page_load_metrics::CaptchaProvider::kCloudflareTurnstile:
      increment(kCaptchaCloudflareCountPref,
                kCaptchaCloudflareCountUserActivatedPref);
      return;
    case page_load_metrics::CaptchaProvider::kHCaptcha:
      increment(kCaptchaHCaptchaCountPref,
                kCaptchaHCaptchaCountUserActivatedPref);
      return;
    case page_load_metrics::CaptchaProvider::kUnknown:
      return;
  }

  NOTREACHED();
}

void CaptchaMetrics::ReportToP3AIfPossible() {
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
                        &CaptchaMetrics::ReportToP3AIfPossible);
    return;
  }

  // In the first ever recorded run, last_recorded_time is null and so are the
  // various captcha storages. So, we can skip emitting as it doesn't reflect
  // no captchas were seen.
  const base::DictValue& counts =
      local_state_->GetDict(kMiscMetricsCaptchaDictionaryPref);

  if (!last_recorded_time.is_null()) {
    for (const auto& [histogram, pref] : kHistogramToPrefMap) {
      // Record only if the metric actually has a non zero value.
      const int value = counts.FindInt(pref).value_or(0);
      if (value > 0) {
        p3a_utils::RecordToHistogramBucket(histogram.data(),
                                           kCaptchaCountBuckets, value);
      }
    }

    // Re-initialize the dict.
    local_state_->ClearPref(kMiscMetricsCaptchaDictionaryPref);
  }

  // Update the last recorded time to now, and start the timer.
  local_state_->SetTime(kMiscMetricsCaptchaLastRecordTime, now);
  report_timer_.Start(FROM_HERE, now + kReportInterval, this,
                      &CaptchaMetrics::ReportToP3AIfPossible);
}

}  // namespace misc_metrics

std::unique_ptr<page_load_metrics::PageLoadMetricsObserverInterface>
BraveCreateCaptchaPageLoadMetricsObserver(Profile* profile) {
  return misc_metrics::CaptchaMetrics::CreatePageLoadMetricsObserver(profile);
}
