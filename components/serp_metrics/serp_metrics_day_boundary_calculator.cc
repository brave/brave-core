/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/serp_metrics/serp_metrics_day_boundary_calculator.h"

#include <optional>
#include <string>

#include "base/check.h"
#include "base/time/time.h"
#include "brave/components/constants/pref_names.h"
#include "components/prefs/pref_service.h"

namespace serp_metrics {

SerpMetricsDayBoundaryCalculator::SerpMetricsDayBoundaryCalculator(
    PrefService* local_state,
    bool report_in_utc)
    : local_state_(local_state), report_in_utc_(report_in_utc) {
  CHECK(local_state_);
}

SerpMetricsDayBoundaryCalculator::~SerpMetricsDayBoundaryCalculator() = default;

base::Time SerpMetricsDayBoundaryCalculator::Midnight(base::Time time) const {
  return report_in_utc_ ? time.UTCMidnight() : time.LocalMidnight();
}

base::Time SerpMetricsDayBoundaryCalculator::GetStartOfYesterday(
    base::Time now) const {
  return Midnight(Midnight(now) - base::Hours(12));
}

base::Time SerpMetricsDayBoundaryCalculator::GetEndOfYesterday(
    base::Time now) const {
  return Midnight(now) - base::Milliseconds(1);
}

base::Time SerpMetricsDayBoundaryCalculator::GetEndOfStalePeriod(
    base::Time now) const {
  return GetStartOfYesterday(now) - base::Milliseconds(1);
}

std::optional<base::Time> SerpMetricsDayBoundaryCalculator::TimeFromString(
    const std::string& time_string) const {
  base::Time time;
  const bool success =
      report_in_utc_ ? base::Time::FromUTCString(time_string.c_str(), &time)
                     : base::Time::FromString(time_string.c_str(), &time);
  return success ? std::make_optional(time) : std::nullopt;
}

base::Time SerpMetricsDayBoundaryCalculator::GetStartOfStalePeriod() const {
  // `kLastCheckYMD` exists to track when the last daily usage ping was sent,
  // so we can compute how far back metrics should be considered stale.
  const std::string& last_check_ymd = local_state_->GetString(kLastCheckYMD);
  if (last_check_ymd.empty()) {
    // If never checked, assume the full time period.
    return {};
  }

  const std::optional<base::Time> last_checked_at =
      TimeFromString(last_check_ymd);
  if (!last_checked_at) {
    // If we can't parse the last check date, assume the full time period.
    return {};
  }

  // Searches recorded on `kLastCheckYMD` have not yet been reported, so the
  // stale period begins at midnight of that day.
  return Midnight(*last_checked_at);
}

}  // namespace serp_metrics
