/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_MISC_METRICS_CAPTCHA_METRICS_H_
#define BRAVE_BROWSER_MISC_METRICS_CAPTCHA_METRICS_H_

#include <memory>

#include "base/memory/raw_ptr.h"
#include "base/timer/wall_clock_timer.h"
#include "chrome/browser/ui/tabs/contents_observing_tab_feature.h"

class GURL;
class PrefRegistrySimple;
class PrefService;
class Profile;

namespace content {
class RenderFrameHost;
class WebContents;
struct GlobalRequestID;
}  // namespace content

namespace blink::mojom {
class ResourceLoadInfo;
}  // namespace blink::mojom

namespace page_load_metrics {
class PageLoadMetricsObserverInterface;
}  // namespace page_load_metrics

namespace tabs {
class TabInterface;
}  // namespace tabs

namespace misc_metrics {

struct CaptchaProviderMetricDetails;

// Keep the histogram name consistent with metric_names.h
inline constexpr char kCaptchaTotalCountHistogramName[] =
    "Brave.CaptchaCount.Total";
inline constexpr char kCaptchaTotalCountUserActivatedHistogramName[] =
    "Brave.CaptchaCount.Total.UserActivated";

inline constexpr char kCaptchaGoogleCountHistogramName[] =
    "Brave.CaptchaCount.Google";
inline constexpr char kCaptchaGoogleCountUserActivatedHistogramName[] =
    "Brave.CaptchaCount.Google.UserActivated";

inline constexpr char kCaptchaCloudflareCountHistogramName[] =
    "Brave.CaptchaCount.Cloudflare";
inline constexpr char kCaptchaCloudflareCountUserActivatedHistogramName[] =
    "Brave.CaptchaCount.Cloudflare.UserActivated";

inline constexpr char kCaptchaHCaptchaCountHistogramName[] =
    "Brave.CaptchaCount.hCaptcha";
inline constexpr char kCaptchaHCaptchaCountUserActivatedHistogramName[] =
    "Brave.CaptchaCount.hCaptcha.UserActivated";

// This class provides the back-end implementation to record a captcha metrics
// once the captcha was detected by the BraveCaptchaPageLoadMetricsObserver.
class CaptchaMetrics {
 public:
  // Observes Cloudflare javascript-detection script loads. A same-origin
  // resource whose path contains "/cdn-cgi/challenge-platform/" is recorded
  // once per WebContents. Unrelated resources that happen to use that path
  // can be counted as well.
  //
  // Owned by tab features, like CommerceUiTabHelper, so the observer follows
  // the tab's WebContents across discards.
  class CloudflareJsDetectionTabHelper
      : public tabs::ContentsObservingTabFeature {
   public:
    ~CloudflareJsDetectionTabHelper() override;

    CloudflareJsDetectionTabHelper(const CloudflareJsDetectionTabHelper&) =
        delete;
    CloudflareJsDetectionTabHelper& operator=(
        const CloudflareJsDetectionTabHelper&) = delete;

    // Returns nullptr unless captcha metrics are enabled for a regular
    // profile.
    static std::unique_ptr<CloudflareJsDetectionTabHelper> MaybeCreate(
        tabs::TabInterface& tab);

   private:
    CloudflareJsDetectionTabHelper(tabs::TabInterface& tab,
                                   CaptchaMetrics* captcha_metrics);

    // content::WebContentsObserver:
    void ResourceLoadComplete(
        content::RenderFrameHost* render_frame_host,
        const content::GlobalRequestID& request_id,
        const GURL& original_url,
        const blink::mojom::ResourceLoadInfo& resource_load_info) override;

    // tabs::ContentsObservingTabFeature:
    void OnDiscardContents(tabs::TabInterface* tab,
                           content::WebContents* old_contents,
                           content::WebContents* new_contents) override;

    // At most one javascript-detection hit per WebContents. Reset when the
    // tab discards its contents, because this feature outlives that
    // WebContents.
    bool recorded_javascript_detection_ = false;
    // This is needed to trigger calls to record events to local state.
    raw_ptr<CaptchaMetrics> captcha_metrics_;
  };

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

  // Records a captcha if |url| matches a known provider, or a Cloudflare
  // javascript-detection script. Does not emit P3A.
  // |is_user_activated| is a signal fired by
  // PageLoadMetricsObserver.FrameReceivedUserActivation which is true when
  // the user interacted with the frame like click, mouse events etc and false
  // otherwise.
  void MaybeRecordCaptchaForUrl(const GURL& url, const bool is_user_activated);

  // Reports histograms for all supported captcha providers iff their
  // corresponding captcha count recorded in the last 24h was non zero.
  // This also schedules the next report.
  void MaybeReport();

  // Reports a P3A histogram for a corresponding captcha |provider_details| iff
  // its captcha count recorded in the last 24h was non zero.
  void MaybeReportProvider(
      const CaptchaProviderMetricDetails& provider_details);

  // The timer to help schedule the next reporting.
  base::WallClockTimer report_timer_;
  raw_ptr<PrefService> local_state_;
};

}  // namespace misc_metrics

#endif  // BRAVE_BROWSER_MISC_METRICS_CAPTCHA_METRICS_H_
