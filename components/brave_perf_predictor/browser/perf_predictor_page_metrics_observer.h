/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_PERF_PREDICTOR_BROWSER_PERF_PREDICTOR_PAGE_METRICS_OBSERVER_H_
#define BRAVE_COMPONENTS_BRAVE_PERF_PREDICTOR_BROWSER_PERF_PREDICTOR_PAGE_METRICS_OBSERVER_H_

#include <cstdint>
#include <memory>
#include <string>

#include "components/page_load_metrics/browser/page_load_metrics_observer.h"
#include "services/metrics/public/cpp/ukm_source_id.h"

#if defined(OS_ANDROID)
#include "brave/components/brave_perf_predictor/browser/perf_predictor_tab_helper.h"
#endif

namespace content {
class NavigationHandle;
}  // namespace content

namespace page_load_metrics {
namespace mojom {
class PageLoadTiming;
}  // namespace mojom
}  // namespace page_load_metrics

namespace brave_perf_predictor {

class PerfPredictorTabHelper;

// Observer responsible for recording per site performance metrics.
class PerfPredictorPageMetricsObserver
    : public page_load_metrics::PageLoadMetricsObserver {
 public:
#if defined(OS_ANDROID)
  explicit PerfPredictorPageMetricsObserver(
      std::unique_ptr<
          brave_perf_predictor::PerfPredictorTabHelperDelegateAndroid>
          delegate);
#else
  PerfPredictorPageMetricsObserver();
#endif
  ~PerfPredictorPageMetricsObserver() override;

  PerfPredictorPageMetricsObserver(const PerfPredictorPageMetricsObserver&) =
      delete;
  PerfPredictorPageMetricsObserver& operator=(
      const PerfPredictorPageMetricsObserver&) = delete;

  void OnFirstContentfulPaintInPage(
      const page_load_metrics::mojom::PageLoadTiming& timing) override;
  void OnFirstMeaningfulPaintInMainFrameDocument(
      const page_load_metrics::mojom::PageLoadTiming& timing) override;
  void OnLoadEventStart(
      const page_load_metrics::mojom::PageLoadTiming& timing) override;

 private:
  ObservePolicy OnCommit(content::NavigationHandle* navigation_handle,
                         ukm::SourceId source_id) override;

  ObservePolicy ShouldObserveMimeType(
      const std::string& mime_type) const override;

  int64_t navigation_id_ = 0;

  // The browser context this navigation is operating in.
  PerfPredictorTabHelper* observer_ = nullptr;

#if defined(OS_ANDROID)
  std::unique_ptr<brave_perf_predictor::PerfPredictorTabHelperDelegateAndroid>
      tab_helper_delegate_;
#endif
};

}  // namespace brave_perf_predictor

#endif  // BRAVE_COMPONENTS_BRAVE_PERF_PREDICTOR_BROWSER_PERF_PREDICTOR_PAGE_METRICS_OBSERVER_H_
