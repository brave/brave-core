/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/p3a/metric_log_type.h"

#include <optional>

#include "base/notreached.h"

namespace p3a {

namespace {
constexpr char kSlowStr[] = "slow";
constexpr char kTypicalStr[] = "typical";
constexpr char kExpressStr[] = "express";
}  // namespace

const char* MetricLogTypeToString(MetricLogType log_type) {
  switch (log_type) {
    case MetricLogType::kSlow:
      return kSlowStr;
    case MetricLogType::kTypical:
      return kTypicalStr;
    case MetricLogType::kExpress:
      return kExpressStr;
  }
  NOTREACHED();
}

std::optional<MetricLogType> StringToMetricLogType(
    const std::string& log_type_str) {
  if (log_type_str == kSlowStr) {
    return MetricLogType::kSlow;
  } else if (log_type_str == kTypicalStr) {
    return MetricLogType::kTypical;
  } else if (log_type_str == kExpressStr) {
    return MetricLogType::kExpress;
  }
  return std::nullopt;
}

}  // namespace p3a
