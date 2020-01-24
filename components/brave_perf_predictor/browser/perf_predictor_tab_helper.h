/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_PERF_PREDICTOR_BROWSER_PERF_PREDICTOR_TAB_HELPER_H_
#define BRAVE_COMPONENTS_BRAVE_PERF_PREDICTOR_BROWSER_PERF_PREDICTOR_TAB_HELPER_H_

#include <memory>
#include <string>

#include "brave/components/brave_perf_predictor/browser/bandwidth_savings_predictor.h"
#include "brave/components/p3a/buildflags.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"
#include "url/gurl.h"

#if BUILDFLAG(BRAVE_P3A_ENABLED)
#include "brave/components/brave_perf_predictor/browser/p3a_bandwidth_savings_tracker.h"
#endif

class PrefRegistrySimple;

namespace content {
class NavigationHandle;
class RenderFrameHost;
class WebContents;

namespace mojom {
class ResourceLoadInfo;
}  // namespace mojom
}  // namespace content

namespace page_load_metrics {
namespace mojom {
class PageLoadTiming;
}  // namespace mojom
}  // namespace page_load_metrics

namespace brave_perf_predictor {

class PerfPredictorTabHelper
    : public content::WebContentsObserver,
      public content::WebContentsUserData<PerfPredictorTabHelper> {
 public:
  explicit PerfPredictorTabHelper(content::WebContents*);
  ~PerfPredictorTabHelper() override;
  // disallow copying
  PerfPredictorTabHelper(const PerfPredictorTabHelper&) = delete;
  PerfPredictorTabHelper& operator=(const PerfPredictorTabHelper&) = delete;

  void OnBlockedSubresource(const std::string& subresource);
  void OnPageLoadTimingUpdated(
      const page_load_metrics::mojom::PageLoadTiming& timing);
  static void RegisterProfilePrefs(PrefRegistrySimple* registry);
  static void DispatchBlockedEvent(
    const std::string& subresource,
    int render_process_id,
    int render_frame_id,
    int frame_tree_node_id);

 private:
  friend class content::WebContentsUserData<PerfPredictorTabHelper>;
  void RecordSavings();

  // content::WebContentsObserver overrides.
  void DidStartNavigation(
      content::NavigationHandle* navigation_handle) override;
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;
  void ResourceLoadComplete(
      content::RenderFrameHost* render_frame_host,
      const content::GlobalRequestID& request_id,
      const content::mojom::ResourceLoadInfo& resource_load_info) override;
  void DidAttachInterstitialPage() override;
  void WebContentsDestroyed() override;

  int64_t navigation_id_ = -1;
  std::unique_ptr<BandwidthSavingsPredictor> bandwidth_predictor_;
#if BUILDFLAG(BRAVE_P3A_ENABLED)
  std::unique_ptr<P3ABandwidthSavingsTracker> bandwidth_tracker_;
#endif

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

}  // namespace brave_perf_predictor

#endif  // BRAVE_COMPONENTS_BRAVE_PERF_PREDICTOR_BROWSER_PERF_PREDICTOR_TAB_HELPER_H_
