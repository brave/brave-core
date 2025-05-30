// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_P3A_METRIC_CONFIG_UTILS_H_
#define BRAVE_COMPONENTS_P3A_METRIC_CONFIG_UTILS_H_

#include <optional>
#include <string_view>

#include "brave/components/p3a/metric_log_type.h"

namespace p3a {

struct MetricConfig;

// Returns the base configuration for a metric from the built-in metric arrays
// (kCollectedExpressHistograms, kCollectedTypicalHistograms,
// kCollectedSlowHistograms)
const MetricConfig* GetBaseMetricConfig(std::string_view histogram_name);

// Returns the log type for a histogram from the built-in metric arrays
std::optional<MetricLogType> GetBaseLogTypeForHistogram(
    std::string_view histogram_name);

}  // namespace p3a

#endif  // BRAVE_COMPONENTS_P3A_METRIC_CONFIG_UTILS_H_
