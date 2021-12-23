/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_perf_predictor/browser/perf_predictor_tab_helper.h"

#include "brave/components/brave_perf_predictor/browser/named_third_party_registry_factory.h"
#include "brave/components/brave_perf_predictor/common/pref_names.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_user_data.h"

#if defined(OS_ANDROID)
#include "brave/browser/android/brave_shields_content_settings.h"
#endif

namespace brave_perf_predictor {

PerfPredictorTabHelper::PerfPredictorTabHelper(
    content::WebContents* web_contents)
    : WebContentsObserver(web_contents),
      content::WebContentsUserData<PerfPredictorTabHelper>(*web_contents),
      bandwidth_predictor_(std::make_unique<BandwidthSavingsPredictor>(
          NamedThirdPartyRegistryFactory::GetForBrowserContext(
              web_contents->GetBrowserContext()))) {
  if (web_contents->GetBrowserContext()->IsOffTheRecord())
    return;

  bandwidth_tracker_ = std::make_unique<P3ABandwidthSavingsTracker>(
      user_prefs::UserPrefs::Get(web_contents->GetBrowserContext()));
}

PerfPredictorTabHelper::~PerfPredictorTabHelper() = default;

void PerfPredictorTabHelper::OnPageLoadTimingUpdated(
    const page_load_metrics::mojom::PageLoadTiming& timing) {
  bandwidth_predictor_->OnPageLoadTimingUpdated(timing);
}

// static
void PerfPredictorTabHelper::RegisterProfilePrefs(
    PrefRegistrySimple* registry) {
  registry->RegisterUint64Pref(prefs::kBandwidthSavedBytes, 0);
}

// static
void PerfPredictorTabHelper::DispatchBlockedEvent(
    const std::string& subresource,
    int frame_tree_node_id) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  content::WebContents* web_contents =
      content::WebContents::FromFrameTreeNodeId(frame_tree_node_id);
  if (!web_contents)
    return;

  PerfPredictorTabHelper* blocking_observer =
      brave_perf_predictor::PerfPredictorTabHelper::FromWebContents(
          web_contents);
  if (blocking_observer) {
    blocking_observer->OnBlockedSubresource(subresource);
  }
}

void PerfPredictorTabHelper::RecordSavings() {
  const uint64_t savings =
      static_cast<uint64_t>(bandwidth_predictor_->PredictSavingsBytes());
  bandwidth_predictor_->Reset();
  VLOG(3) << "Saving computed bw saving = " << savings;
  if (savings > 0) {
    // BrowserContenxt can be null in tests
    auto* browser_context = GetWebContents().GetBrowserContext();
    if (!browser_context)
      return;

    PrefService* prefs = user_prefs::UserPrefs::Get(browser_context);
    if (prefs)
      prefs->SetUint64(prefs::kBandwidthSavedBytes,
                       prefs->GetUint64(prefs::kBandwidthSavedBytes) + savings);

    if (bandwidth_tracker_)
      bandwidth_tracker_->RecordSavings(savings);
#if defined(OS_ANDROID)
    chrome::android::BraveShieldsContentSettings::DispatchSavedBandwidth(
        savings);
#endif
  }
}

void PerfPredictorTabHelper::OnBlockedSubresource(
    const std::string& subresource) {
  bandwidth_predictor_->OnSubresourceBlocked(subresource);
}

void PerfPredictorTabHelper::DidStartNavigation(
    content::NavigationHandle* handle) {
  if (!handle || !handle->IsInMainFrame() || handle->IsDownload())
    return;
  // Gather prediction of the _previous_ navigation
  if (navigation_id_ != handle->GetNavigationId() && navigation_id_ > 0)
    RecordSavings();
}

void PerfPredictorTabHelper::DidFinishNavigation(
    content::NavigationHandle* handle) {
  if (!handle || !handle->IsInMainFrame() || !handle->HasCommitted() ||
      handle->IsDownload())
    return;
  // Reset predictor state when we're committed to this navigation
  bandwidth_predictor_->Reset();
  // Record current navigation ID to know if we're in the same navigation later
  navigation_id_ = handle->GetNavigationId();
}

void PerfPredictorTabHelper::ResourceLoadComplete(
    content::RenderFrameHost* render_frame_host,
    const content::GlobalRequestID& request_id,
    const blink::mojom::ResourceLoadInfo& resource_load_info) {
  if (render_frame_host)
    bandwidth_predictor_->OnResourceLoadComplete(GetWebContents().GetURL(),
                                                 resource_load_info);
}

void PerfPredictorTabHelper::WebContentsDestroyed() {
  // Run a prediction when Web Contents get destroyed (e.g. tab/window closed)
  RecordSavings();
  VLOG(3) << "Web contents destroyed, savings recorded";
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(PerfPredictorTabHelper);

}  // namespace brave_perf_predictor
