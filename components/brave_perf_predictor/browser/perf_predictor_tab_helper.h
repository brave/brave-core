/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_PERF_PREDICTOR_BROWSER_PERF_PREDICTOR_TAB_HELPER_H_
#define BRAVE_COMPONENTS_BRAVE_PERF_PREDICTOR_BROWSER_PERF_PREDICTOR_TAB_HELPER_H_

#include <memory>
#include <string>

#include "base/memory/weak_ptr.h"
#include "brave/components/brave_perf_predictor/browser/bandwidth_savings_predictor.h"
#include "brave/components/brave_perf_predictor/browser/p3a_bandwidth_savings_tracker.h"
#include "content/public/browser/web_contents_observer.h"
#include "ui/base/unowned_user_data/scoped_unowned_user_data.h"

class PrefRegistrySimple;

namespace content {
class NavigationHandle;
class RenderFrameHost;
class WebContents;
}  // namespace content

namespace blink {
namespace mojom {
class ResourceLoadInfo;
}  // namespace mojom
}  // namespace blink

namespace tabs {
class TabInterface;
}  // namespace tabs

namespace page_load_metrics {
namespace mojom {
class PageLoadTiming;
}  // namespace mojom
}  // namespace page_load_metrics

namespace brave_perf_predictor {

// The main entry point for performance prediction. Collects events from
// WebContentsObserver, received `PageLoadTiming` reports and adblocker resource
// blocked events to compute estimated shields' savings seen by the user. Owned
// by the tab's features, and recreated when the tab's contents is replaced.
class PerfPredictorTabHelper : public content::WebContentsObserver {
 public:
  DECLARE_USER_DATA(PerfPredictorTabHelper);

  static PerfPredictorTabHelper* From(tabs::TabInterface* tab);

  PerfPredictorTabHelper(tabs::TabInterface& tab,
                         content::WebContents* web_contents);
  // Runs a prediction for the page shown, as the tab closes or its contents is
  // replaced.
  ~PerfPredictorTabHelper() override;
  PerfPredictorTabHelper(const PerfPredictorTabHelper&) = delete;
  PerfPredictorTabHelper& operator=(const PerfPredictorTabHelper&) = delete;

  // Called from `PerfPredictorPageMetricsObserver`, associated through
  // `WebContents`
  void OnPageLoadTimingUpdated(
      const page_load_metrics::mojom::PageLoadTiming& timing);
  static void RegisterProfilePrefs(PrefRegistrySimple* registry);
  // Called from Brave Shields
  static void DispatchBlockedEvent(
      const std::string& subresource,
      content::GlobalRenderFrameHostToken render_frame_token);

  base::WeakPtr<PerfPredictorTabHelper> GetWeakPtr();

 private:
  void RecordSavings();
  void OnBlockedSubresource(const std::string& subresource);

  // content::WebContentsObserver overrides.

  // The same tab helper can be reused for multiple navigation instances,
  // need to "close" previous navigation's set of features and run prediction as
  // soon as a new navigation starts.
  void DidStartNavigation(
      content::NavigationHandle* navigation_handle) override;

  // When navigation is finished and actual page load starts we start collecting
  // a new feature set for prediction
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;

  // We collect stats about individual resources loaded, to track their sizes
  // and counts by type
  void ResourceLoadComplete(
      content::RenderFrameHost* render_frame_host,
      const content::GlobalRequestID& request_id,
      const GURL& original_url,
      const blink::mojom::ResourceLoadInfo& resource_load_info) override;

  int64_t navigation_id_ = -1;
  std::unique_ptr<BandwidthSavingsPredictor> bandwidth_predictor_;
  std::unique_ptr<P3ABandwidthSavingsTracker> bandwidth_tracker_;

  ui::ScopedUnownedUserData<PerfPredictorTabHelper> scoped_unowned_user_data_;

  base::WeakPtrFactory<PerfPredictorTabHelper> weak_factory_{this};
};

}  // namespace brave_perf_predictor

#endif  // BRAVE_COMPONENTS_BRAVE_PERF_PREDICTOR_BROWSER_PERF_PREDICTOR_TAB_HELPER_H_
