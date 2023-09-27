/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_P3A_METRIC_LOG_TYPE_H_
#define BRAVE_COMPONENTS_P3A_METRIC_LOG_TYPE_H_

#include <string>

#include "third_party/abseil-cpp/absl/types/optional.h"

namespace p3a {

constexpr char kCreativeMetricPrefix[] = "creativeInstanceId.";

enum class MetricLogType {
  // Slow metrics are currently sent once per month.
  kSlow,
  // Typical metrics are currently sent once per week.
  kTypical,
  // Express metrics are currently sent once per day.
  kExpress
};

constexpr MetricLogType kAllMetricLogTypes[] = {
    MetricLogType::kSlow, MetricLogType::kTypical, MetricLogType::kExpress};

const char* MetricLogTypeToString(MetricLogType log_type);
absl::optional<MetricLogType> StringToMetricLogType(
    const std::string& log_type_str);

}  // namespace p3a

#endif  // BRAVE_COMPONENTS_P3A_METRIC_LOG_TYPE_H_
