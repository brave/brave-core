/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/check.h"
#include "base/feature_list.h"
#include "brave/components/misc_metrics/features.h"
#include "components/page_load_metrics/browser/page_load_metrics_observer_interface.h"
#define InitializePageLoadMetricsForWebContents \
  InitializePageLoadMetricsForWebContents_Chromium
#include <chrome/browser/page_load_metrics/page_load_metrics_initialize.cc>
#undef InitializePageLoadMetricsForWebContents

#include <memory>
#include <optional>

#include "brave/browser/brave_browser_process.h"
#include "brave/browser/misc_metrics/captcha_metrics.h"
#include "brave/browser/misc_metrics/process_misc_metrics.h"
#include "brave/components/brave_perf_predictor/browser/perf_predictor_page_metrics_observer.h"
#include "chrome/browser/page_load_metrics/observers/captcha_provider_manager.h"
#include "components/page_load_metrics/browser/page_load_metrics_observer.h"
#include "content/public/browser/navigation_handle.h"
#include "url/gurl.h"

namespace {

void MaybeRecordCaptchaForUrl(const GURL& url) {
  std::optional<page_load_metrics::CaptchaProvider> captcha_provider =
      page_load_metrics::CaptchaProviderManager::GetInstance()
          ->GetCaptchaProviderForUrl(url);
  if (!captcha_provider.has_value()) {
    return;
  }

  if (!g_brave_browser_process ||
      !g_brave_browser_process->process_misc_metrics() ||
      !g_brave_browser_process->process_misc_metrics()->captcha_metrics()) {
    return;
  }

  misc_metrics::CaptchaProvider provider =
      misc_metrics::CaptchaProvider::kOther;
  switch (*captcha_provider) {
    case page_load_metrics::CaptchaProvider::kReCaptcha:
      provider = misc_metrics::CaptchaProvider::kGoogle;
      break;
    case page_load_metrics::CaptchaProvider::kCloudflareTurnstile:
      provider = misc_metrics::CaptchaProvider::kCloudflare;
      break;
    case page_load_metrics::CaptchaProvider::kHCaptcha:
      provider = misc_metrics::CaptchaProvider::kHCaptcha;
      break;
    case page_load_metrics::CaptchaProvider::kUnknown:
      break;
  }
  g_brave_browser_process->process_misc_metrics()
      ->captcha_metrics()
      ->RecordCaptcha(provider);
}

// This class detects the potential captcha events on top-level/sub frame loads
// and emits a histogram for that.
class BraveCaptchaPageLoadMetricsObserver
    : public page_load_metrics::PageLoadMetricsObserver {
 public:
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
    MaybeRecordCaptchaForUrl(navigation_handle->GetURL());
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

    MaybeRecordCaptchaForUrl(navigation_handle->GetURL());
  }
};

class BravePageLoadMetricsEmbedder : public PageLoadMetricsEmbedder {
 public:
  explicit BravePageLoadMetricsEmbedder(content::WebContents* web_contents);
  BravePageLoadMetricsEmbedder(const BravePageLoadMetricsEmbedder&) = delete;
  BravePageLoadMetricsEmbedder& operator=(const BravePageLoadMetricsEmbedder&) =
      delete;
  ~BravePageLoadMetricsEmbedder() override;

 protected:
  // page_load_metrics::PageLoadMetricsEmbedderBase:
  void RegisterObservers(page_load_metrics::PageLoadTracker* tracker,
                         content::NavigationHandle* navigation_handle) override;
};

BravePageLoadMetricsEmbedder::BravePageLoadMetricsEmbedder(
    content::WebContents* web_contents)
    : PageLoadMetricsEmbedder(web_contents) {}

BravePageLoadMetricsEmbedder::~BravePageLoadMetricsEmbedder() = default;

void BravePageLoadMetricsEmbedder::RegisterObservers(
    page_load_metrics::PageLoadTracker* tracker,
    content::NavigationHandle* navigation_handle) {
  PageLoadMetricsEmbedder::RegisterObservers(tracker, navigation_handle);

  tracker->AddObserver(
      std::make_unique<
          brave_perf_predictor::PerfPredictorPageMetricsObserver>());

  if (base::FeatureList::IsEnabled(
          misc_metrics::features::kCaptchaMetricsCollection)) {
    tracker->AddObserver(
        std::make_unique<BraveCaptchaPageLoadMetricsObserver>());
  }
}

}  // namespace

void InitializePageLoadMetricsForWebContents(
    content::WebContents* web_contents) {
  // TODO(bug https://github.com/brave/brave-browser/issues/7784)
  // change
  // android_webview/browser/page_load_metrics/page_load_metrics_initialize.cc
  // as well to register Page Load Metrics Observers
  page_load_metrics::MetricsWebContentsObserver::CreateForWebContents(
      web_contents,
      std::make_unique<BravePageLoadMetricsEmbedder>(web_contents));
}
