/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/serp_metrics/serp_metrics_migration.h"

#include <string>

#include "base/time/time.h"
#include "brave/components/constants/pref_names.h"
#include "components/prefs/pref_service.h"

namespace serp_metrics {

base::Time GetMigrationStaleBoundaryFromLastCheckYMD(
    const PrefService& local_state) {
  const std::string& last_check_ymd = local_state.GetString(kLastCheckYMD);
  if (last_check_ymd.empty()) {
    // If never checked, assume the full time period.
    return {};
  }

  // `kLastCheckYMD` was written as a local calendar date, so for UTC+N clients
  // who ping before UTC midnight the stale boundary may be one UTC day late.
  // This is a one-time migration artefact; once `kLastReportedAt` is written
  // after the first ping under the new scheme this path is never taken again.
  base::Time last_checked_at;
  const bool success =
      base::Time::FromUTCString(last_check_ymd.c_str(), &last_checked_at);
  if (!success) {
    // If we can't parse the last check date, return the current time to avoid
    // double reporting of previous searches.
    return base::Time::Now();
  }

  // Searches recorded on the day of `kLastCheckYMD` have not yet been
  // reported, so the stale period begins at UTC midnight of that day.
  return last_checked_at.UTCMidnight();
}

}  // namespace serp_metrics
