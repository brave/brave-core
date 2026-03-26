/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/serp_metrics/serp_metrics_service_ios.h"

#include "base/feature_list.h"
#include "brave/components/serp_metrics/serp_metrics.h"
#include "brave/components/serp_metrics/serp_metrics_feature.h"
#include "brave/components/time_period_storage/pref_time_period_store_factory.h"
#include "brave/ios/browser/serp_metrics/serp_metrics_prefs.h"
#include "components/prefs/pref_service.h"

namespace serp_metrics {

SerpMetricsServiceIOS::SerpMetricsServiceIOS(PrefService& local_state,
                                             PrefService& profile_prefs) {
  if (base::FeatureList::IsEnabled(kSerpMetricsFeature)) {
    serp_metrics_ = std::make_unique<SerpMetrics>(
        &local_state, PrefTimePeriodStoreFactory(
                          &profile_prefs, kSerpMetricsTimePeriodStorage));
  }
}

SerpMetricsServiceIOS::~SerpMetricsServiceIOS() = default;

void SerpMetricsServiceIOS::Shutdown() {
  serp_metrics_.reset();
}

SerpMetrics* SerpMetricsServiceIOS::Get() {
  return serp_metrics_.get();
}

}  // namespace serp_metrics
