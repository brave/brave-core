// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/p3a/metric_config_utils.h"

#include "brave/components/p3a/metric_names.h"

namespace p3a {

const MetricConfig* GetBaseMetricConfig(std::string_view histogram_name) {
  const std::optional<MetricConfig>* metric_config = nullptr;

  auto it = p3a::kCollectedTypicalHistograms.find(histogram_name);
  if (it != p3a::kCollectedTypicalHistograms.end()) {
    metric_config = &it->second;
  } else if (it = p3a::kCollectedSlowHistograms.find(histogram_name);
             it != p3a::kCollectedSlowHistograms.end()) {
    metric_config = &it->second;
  } else if (it = p3a::kCollectedExpressHistograms.find(histogram_name);
             it != p3a::kCollectedExpressHistograms.end()) {
    metric_config = &it->second;
  }
  if (metric_config && metric_config->has_value()) {
    return &metric_config->value();
  }
  return nullptr;
}

std::optional<MetricLogType> GetBaseLogTypeForHistogram(
    std::string_view histogram_name) {
  if (p3a::kCollectedExpressHistograms.contains(histogram_name)) {
    return MetricLogType::kExpress;
  } else if (p3a::kCollectedSlowHistograms.contains(histogram_name)) {
    return MetricLogType::kSlow;
  } else if (p3a::kCollectedTypicalHistograms.contains(histogram_name)) {
    return MetricLogType::kTypical;
  }
  return std::nullopt;
}

}  // namespace p3a
