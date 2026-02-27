/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/serp_metrics/serp_metrics_all_profiles_aggregator.h"

#include <numeric>

#include "base/feature_list.h"
#include "brave/browser/serp_metrics/serp_metrics_util.h"
#include "brave/components/serp_metrics/serp_metrics.h"
#include "brave/components/serp_metrics/serp_metrics_feature.h"
#include "chrome/browser/profiles/profile_attributes_storage.h"

namespace serp_metrics {

SerpMetricsAllProfilesAggregator::SerpMetricsAllProfilesAggregator(
    PrefService* local_state,
    ProfileAttributesStorage& profile_attributes_storage) {
  if (!base::FeatureList::IsEnabled(serp_metrics::kSerpMetricsFeature)) {
    return;
  }

  for (ProfileAttributesEntry* entry :
       profile_attributes_storage.GetAllProfilesAttributes()) {
    const base::FilePath& profile_path = entry->GetPath();
    profile_attributes_serp_metrics_.push_back(CreateSerpMetrics(
        local_state, profile_path, profile_attributes_storage));
  }
}

SerpMetricsAllProfilesAggregator::~SerpMetricsAllProfilesAggregator() = default;

size_t SerpMetricsAllProfilesAggregator::GetSearchCountForYesterday(
    SerpMetricType type) const {
  return std::accumulate(
      profile_attributes_serp_metrics_.begin(),
      profile_attributes_serp_metrics_.end(), size_t{0},
      [type](size_t count, const std::unique_ptr<SerpMetrics>& serp_metrics) {
        return count + serp_metrics->GetSearchCountForYesterday(type);
      });
}

size_t SerpMetricsAllProfilesAggregator::GetSearchCountForStalePeriod() const {
  return std::accumulate(
      profile_attributes_serp_metrics_.begin(),
      profile_attributes_serp_metrics_.end(), size_t{0},
      [](size_t count, const std::unique_ptr<SerpMetrics>& serp_metrics) {
        return count + serp_metrics->GetSearchCountForStalePeriod();
      });
}

}  // namespace serp_metrics
