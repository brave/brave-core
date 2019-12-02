/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/page_load_metrics/browser/page_load_metrics_embedder_base.h"

#include "base/timer/timer.h"

#include "components/page_load_metrics/browser/observers/core_page_load_metrics_observer.h"
#include "components/page_load_metrics/browser/observers/use_counter_page_load_metrics_observer.h"
#include "components/page_load_metrics/browser/page_load_tracker.h"

#include "brave/components/brave_perf_predictor/browser/buildflags/buildflags.h"

#if BUILDFLAG(ENABLE_BRAVE_PERF_PREDICTOR)
#include "brave/components/brave_perf_predictor/browser/perf_predictor_page_metrics_observer.h"
using brave_perf_predictor::PerfPredictorPageMetricsObserver;
#endif

namespace page_load_metrics {

PageLoadMetricsEmbedderBase::PageLoadMetricsEmbedderBase(
    content::WebContents* web_contents)
    : web_contents_(web_contents) {}

PageLoadMetricsEmbedderBase::~PageLoadMetricsEmbedderBase() = default;

void PageLoadMetricsEmbedderBase::RegisterObservers(PageLoadTracker* tracker) {
  // Register observers used by all embedders
  if (!IsPrerendering()) {
    tracker->AddObserver(std::make_unique<CorePageLoadMetricsObserver>());
    tracker->AddObserver(std::make_unique<UseCounterPageLoadMetricsObserver>());
  }
  // Register Brave-specific observers
#if BUILDFLAG(ENABLE_BRAVE_PERF_PREDICTOR)
  tracker->AddObserver(std::make_unique<PerfPredictorPageMetricsObserver>());
#endif
  // Allow the embedder to register any embedder-specific observers
  RegisterEmbedderObservers(tracker);
}

std::unique_ptr<base::OneShotTimer> PageLoadMetricsEmbedderBase::CreateTimer() {
  return std::make_unique<base::OneShotTimer>();
}

}  // namespace page_load_metrics
