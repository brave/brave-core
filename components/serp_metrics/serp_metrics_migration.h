/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SERP_METRICS_SERP_METRICS_MIGRATION_H_
#define BRAVE_COMPONENTS_SERP_METRICS_SERP_METRICS_MIGRATION_H_

namespace base {
class Time;
}  // namespace base

class PrefService;

namespace serp_metrics {

// Called during migration to derive the stale-period boundary from
// `kLastCheckYMD` when `kLastReportedAt` has not yet been written. The
// day boundary approximation may cause under counting on the first reporting
// cycle after migration.
// Returns `base::Time::Now()` on parse failure to avoid double reporting.
// This suppresses accumulated metrics for corrupt pref data; returning `{}`
// would instead overreport all prior history.
base::Time GetMigrationStaleBoundaryFromLastCheckYMD(
    const PrefService& local_state);

}  // namespace serp_metrics

#endif  // BRAVE_COMPONENTS_SERP_METRICS_SERP_METRICS_MIGRATION_H_
