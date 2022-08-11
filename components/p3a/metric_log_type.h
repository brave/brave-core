/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_P3A_METRIC_LOG_TYPE_H_
#define BRAVE_COMPONENTS_P3A_METRIC_LOG_TYPE_H_

namespace brave {

constexpr char kCreativeMetricPrefix[] = "creativeInstanceId.";

enum class MetricLogType {
  // Typical metrics are currently sent once per week.
  kTypical,
  // Express metrics are currently sent once per day.
  kExpress
};

constexpr MetricLogType kAllMetricLogTypes[] = {MetricLogType::kTypical,
                                                MetricLogType::kExpress};

const char* MetricLogTypeToString(MetricLogType log_type);

}  // namespace brave

#endif  // BRAVE_COMPONENTS_P3A_METRIC_LOG_TYPE_H_
