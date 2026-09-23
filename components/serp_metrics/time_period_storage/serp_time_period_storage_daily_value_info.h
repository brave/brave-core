/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SERP_METRICS_TIME_PERIOD_STORAGE_SERP_TIME_PERIOD_STORAGE_DAILY_VALUE_INFO_H_
#define BRAVE_COMPONENTS_SERP_METRICS_TIME_PERIOD_STORAGE_SERP_TIME_PERIOD_STORAGE_DAILY_VALUE_INFO_H_

#include <cstdint>

#include "base/time/time.h"

namespace serp_metrics {

// A count observed on a particular calendar day.
struct DailyValueInfo {
  base::Time time;
  uint64_t value = 0ULL;
};

}  // namespace serp_metrics

#endif  // BRAVE_COMPONENTS_SERP_METRICS_TIME_PERIOD_STORAGE_SERP_TIME_PERIOD_STORAGE_DAILY_VALUE_INFO_H_
