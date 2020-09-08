/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_perf_predictor/browser/perf_predictor_page_metrics_observer.h"

#include <memory>
#include <utility>

#include "brave/components/brave_perf_predictor/browser/perf_predictor_tab_helper.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"

namespace brave_perf_predictor {

#if defined(OS_ANDROID)
PerfPredictorPageMetricsObserver::PerfPredictorPageMetricsObserver(
    std::unique_ptr<
        brave_perf_predictor::PerfPredictorTabHelperDelegateAndroid>
            delegate)
    : tab_helper_delegate_(std::move(delegate)) {}
#else
PerfPredictorPageMetricsObserver::PerfPredictorPageMetricsObserver() = default;
#endif
PerfPredictorPageMetricsObserver::~PerfPredictorPageMetricsObserver() = default;

page_load_metrics::PageLoadMetricsObserver::ObservePolicy
PerfPredictorPageMetricsObserver::OnCommit(
    content::NavigationHandle* navigation_handle,
    ukm::SourceId source_id) {
  // Skip if off the record
  if (navigation_handle->GetWebContents()
          ->GetBrowserContext()
          ->IsOffTheRecord())
    return STOP_OBSERVING;

  navigation_id_ = navigation_handle->GetNavigationId();
  // We'll be forwarding all performance metrics to the observer
  observer_ = PerfPredictorTabHelper::FromWebContents(
      navigation_handle->GetWebContents());
  if (!observer_) {
    VLOG(2) << navigation_id_ << " could not get PerfPredictorTabHelper";
    return STOP_OBSERVING;
  }
#if defined(OS_ANDROID)
  observer_->set_perf_predictor_tab_helper_deletate(tab_helper_delegate_.get());
#endif
  return CONTINUE_OBSERVING;
}

page_load_metrics::PageLoadMetricsObserver::ObservePolicy
PerfPredictorPageMetricsObserver::ShouldObserveMimeType(
    const std::string& mime_type) const {
  // Observe all MIME types. We still only use actual data usage, so strange
  // cases (e.g., data:// URLs) will still record the right amount of data
  // usage.
  return CONTINUE_OBSERVING;
}

void PerfPredictorPageMetricsObserver::OnFirstContentfulPaintInPage(
    const page_load_metrics::mojom::PageLoadTiming& timing) {
  if (observer_) {
    observer_->OnPageLoadTimingUpdated(timing);
  } else {
    VLOG(2) << "PerfPredictorTabHelper not ready for timing updates";
  }
}

void PerfPredictorPageMetricsObserver::
    OnFirstMeaningfulPaintInMainFrameDocument(
        const page_load_metrics::mojom::PageLoadTiming& timing) {
  if (observer_) {
    observer_->OnPageLoadTimingUpdated(timing);
  } else {
    VLOG(2) << "PerfPredictorTabHelper not ready for timing updates";
  }
}

void PerfPredictorPageMetricsObserver::OnLoadEventStart(
    const page_load_metrics::mojom::PageLoadTiming& timing) {
  if (observer_) {
    observer_->OnPageLoadTimingUpdated(timing);
  } else {
    VLOG(2) << "PerfPredictorTabHelper not ready for timing updates";
  }
}

}  // namespace brave_perf_predictor
