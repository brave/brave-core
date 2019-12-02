/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_PERF_PREDICTOR_BROWSER_PERF_PREDICTOR_TAB_HELPER_H_
#define BRAVE_COMPONENTS_BRAVE_PERF_PREDICTOR_BROWSER_PERF_PREDICTOR_TAB_HELPER_H_

#include <string>

#include "base/macros.h"
#include "components/page_load_metrics/common/page_load_metrics.mojom.h"
#include "components/prefs/pref_registry_simple.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"
#include "content/public/common/resource_load_info.mojom.h"
#include "url/gurl.h"

#include "brave/components/brave_perf_predictor/browser/bandwidth_savings_predictor.h"
#include "brave/components/brave_perf_predictor/browser/perf_predictor_tab_helper.h"

namespace brave_perf_predictor {

class PerfPredictorTabHelper : public content::WebContentsObserver,
    public content::WebContentsUserData<PerfPredictorTabHelper> {
 public:
  explicit PerfPredictorTabHelper(content::WebContents*);
  ~PerfPredictorTabHelper() override;
  void OnBlockedSubresource(const std::string& subresource);
  void OnPageLoadTimingUpdated(
    const page_load_metrics::mojom::PageLoadTiming& timing);
  static void RegisterProfilePrefs(PrefRegistrySimple* registry);

 protected:
  void RecordSaving();

  // content::WebContentsObserver overrides.
  void DidStartNavigation(
      content::NavigationHandle* navigation_handle) override;
  void ReadyToCommitNavigation(
      content::NavigationHandle* navigation_handle) override;
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;
  void ResourceLoadComplete(
      content::RenderFrameHost* render_frame_host,
      const content::GlobalRequestID& request_id,
      const content::mojom::ResourceLoadInfo& resource_load_info) override;
  void DidAttachInterstitialPage() override;
  void WebContentsDestroyed() override;

 private:
  friend class content::WebContentsUserData<PerfPredictorTabHelper>;
  int64_t navigation_id_;
  GURL main_frame_url_;
  BandwidthSavingsPredictor* bandwidth_predictor_;

  WEB_CONTENTS_USER_DATA_KEY_DECL();
  DISALLOW_COPY_AND_ASSIGN(PerfPredictorTabHelper);
};

}  // namespace brave_perf_predictor

#endif  // BRAVE_COMPONENTS_BRAVE_PERF_PREDICTOR_BROWSER_PERF_PREDICTOR_TAB_HELPER_H_
