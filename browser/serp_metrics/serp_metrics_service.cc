/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/serp_metrics/serp_metrics_service.h"

#include "base/feature_list.h"
#include "base/files/file_path.h"
#include "brave/browser/serp_metrics/serp_metrics_time_period_store_factory.h"
#include "brave/components/serp_metrics/serp_metrics.h"
#include "brave/components/serp_metrics/serp_metrics_feature.h"
#include "chrome/browser/profiles/profile_attributes_storage.h"
#include "components/prefs/pref_service.h"

namespace serp_metrics {

SerpMetricsService::SerpMetricsService(
    PrefService& local_state,
    base::FilePath profile_path,
    ProfileAttributesStorage& profile_attributes_storage) {
  if (base::FeatureList::IsEnabled(serp_metrics::kSerpMetricsFeature)) {
    serp_metrics_ = std::make_unique<SerpMetrics>(
        &local_state, SerpMetricsTimePeriodStoreFactory(
                          profile_path, profile_attributes_storage));
  }
}

SerpMetricsService::~SerpMetricsService() = default;

serp_metrics::SerpMetrics* SerpMetricsService::Get() {
  return serp_metrics_.get();
}

}  // namespace serp_metrics
