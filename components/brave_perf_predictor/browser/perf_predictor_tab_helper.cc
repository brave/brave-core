/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_perf_predictor/browser/perf_predictor_tab_helper.h"

#include "brave/components/brave_perf_predictor/browser/third_parties.h"
#include "brave/components/brave_perf_predictor/browser/third_party_extractor.h"
#include "brave/components/brave_perf_predictor/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/web_contents_user_data.h"

namespace brave_perf_predictor {

PerfPredictorTabHelper::PerfPredictorTabHelper(
    content::WebContents* web_contents)
    : WebContentsObserver(web_contents), bandwidth_predictor_() {
  if (web_contents->GetBrowserContext()->IsOffTheRecord())
    return;

#if BUILDFLAG(BRAVE_P3A_ENABLED)
  bandwidth_tracker_ = std::make_unique<P3ABandwidthSavingsTracker>(
      user_prefs::UserPrefs::Get(web_contents->GetBrowserContext()));
#endif
}

PerfPredictorTabHelper::~PerfPredictorTabHelper() = default;

void PerfPredictorTabHelper::DidStartNavigation(
    content::NavigationHandle* handle) {
  if (!handle || !handle->IsInMainFrame() || handle->IsDownload())
    return;
  // Gather prediction of the _previous_ navigation
  if (navigation_id_ != 0)
    RecordSavings();
}

void PerfPredictorTabHelper::ReadyToCommitNavigation(
    content::NavigationHandle* handle) {
  if (!handle || !handle->IsInMainFrame() || handle->IsDownload())
    return;
  // Reset predictor state when we're committed to this navigation
  if (bandwidth_predictor_)
    bandwidth_predictor_->Reset();
}

void PerfPredictorTabHelper::DidFinishNavigation(
    content::NavigationHandle* handle) {
  if (!handle || !handle->IsInMainFrame() || !handle->HasCommitted() ||
      handle->IsDownload())
    return;

  main_frame_url_ = handle->GetURL();
}

void PerfPredictorTabHelper::RecordSavings() {
  if (bandwidth_predictor_ && web_contents()) {
    uint64_t savings = (uint64_t)bandwidth_predictor_->predict();
    VLOG(3) << "Saving computed bw saving = " << savings;
    if (savings > 0) {
      // BrowserContenxt can be null in tests
      auto* browser_context = web_contents()->GetBrowserContext();
      if (!browser_context)
        return;
      PrefService* prefs = user_prefs::UserPrefs::Get(browser_context);

      if (prefs)
        prefs->SetUint64(
            prefs::kBandwidthSavedBytes,
            prefs->GetUint64(prefs::kBandwidthSavedBytes) + savings);

#if BUILDFLAG(BRAVE_P3A_ENABLED)
      if (bandwidth_tracker_)
        bandwidth_tracker_->RecordSavings(savings);
#endif
    }
  }
}

void PerfPredictorTabHelper::ResourceLoadComplete(
    content::RenderFrameHost* render_frame_host,
    const content::GlobalRequestID& request_id,
    const content::mojom::ResourceLoadInfo& resource_load_info) {
  if (render_frame_host && bandwidth_predictor_)
    bandwidth_predictor_->OnResourceLoadComplete(main_frame_url_,
                                                 resource_load_info);
}

void PerfPredictorTabHelper::OnBlockedSubresource(
    const std::string& subresource) {
  if (bandwidth_predictor_)
    bandwidth_predictor_->OnSubresourceBlocked(subresource);
}

void PerfPredictorTabHelper::DidAttachInterstitialPage() {
  // web contents unloaded
  if (bandwidth_predictor_)
    // Predict to clear the state, but don't save the result
    bandwidth_predictor_->Reset();
}

void PerfPredictorTabHelper::WebContentsDestroyed() {
  // Run a prediction when Web Contents get destroyed (e.g. tab/window closed)
  RecordSavings();
}

void PerfPredictorTabHelper::OnPageLoadTimingUpdated(
    const page_load_metrics::mojom::PageLoadTiming& timing) {
  if (bandwidth_predictor_)
    bandwidth_predictor_->OnPageLoadTimingUpdated(timing);
}

// static
void PerfPredictorTabHelper::RegisterProfilePrefs(
    PrefRegistrySimple* registry) {
  registry->RegisterUint64Pref(prefs::kBandwidthSavedBytes, 0);
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(PerfPredictorTabHelper)

}  // namespace brave_perf_predictor
