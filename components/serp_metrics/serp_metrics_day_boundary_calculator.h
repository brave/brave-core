/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SERP_METRICS_SERP_METRICS_DAY_BOUNDARY_CALCULATOR_H_
#define BRAVE_COMPONENTS_SERP_METRICS_SERP_METRICS_DAY_BOUNDARY_CALCULATOR_H_

#include <optional>
#include <string>

#include "base/memory/raw_ptr.h"
#include "base/time/time.h"

class PrefService;

namespace serp_metrics {

// Decouples day-boundary time calculations from `SerpMetrics`, dispatching to
// UTC or local time depending on the `report_in_utc` flag passed at
// construction.
class SerpMetricsDayBoundaryCalculator final {
 public:
  SerpMetricsDayBoundaryCalculator(PrefService* local_state,
                                   bool report_in_utc);

  SerpMetricsDayBoundaryCalculator(const SerpMetricsDayBoundaryCalculator&) =
      delete;
  SerpMetricsDayBoundaryCalculator& operator=(
      const SerpMetricsDayBoundaryCalculator&) = delete;

  ~SerpMetricsDayBoundaryCalculator();

  // Returns the start of yesterday (midnight at the beginning of the previous
  // calendar day). Subtracting 12 hours ensures we cross into the previous day
  // even during daylight saving time transitions when using local time, so the
  // final normalization to midnight always resolves to the correct day.
  base::Time GetStartOfYesterday(base::Time now) const;

  // Returns the end of yesterday defined as the final millisecond before
  // today's midnight. Subtracting one millisecond ensures the yesterday time
  // range is inclusive of all events on that day without spilling into today.
  base::Time GetEndOfYesterday(base::Time now) const;

  // Returns the end of the stale period defined as the final millisecond before
  // the start of yesterday. This establishes a clear boundary between stale
  // metrics and yesterday's metrics without overlap.
  base::Time GetEndOfStalePeriod(base::Time now) const;

  // Returns the start of the stale period based on the last day a usage ping
  // was sent. Searches recorded on `kLastCheckYMD` have not yet been reported,
  // so the stale period begins at midnight of that day. If the last check date
  // is unavailable or invalid, an empty time is returned to indicate that the
  // full retention period should be considered stale.
  base::Time GetStartOfStalePeriod() const;

 private:
  base::Time Midnight(base::Time time) const;

  std::optional<base::Time> TimeFromString(
      const std::string& time_string) const;

  raw_ptr<PrefService> local_state_;  // Not owned.
  const bool report_in_utc_;
};

}  // namespace serp_metrics

#endif  // BRAVE_COMPONENTS_SERP_METRICS_SERP_METRICS_DAY_BOUNDARY_CALCULATOR_H_
