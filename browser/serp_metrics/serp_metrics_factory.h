/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_SERP_METRICS_SERP_METRICS_FACTORY_H_
#define BRAVE_BROWSER_SERP_METRICS_SERP_METRICS_FACTORY_H_

#include <memory>

class PrefService;
class ProfileAttributesStorage;
class TimePeriodStoreFactory;

namespace base {
class FilePath;
}  // namespace base

namespace serp_metrics {

class SerpMetrics;

std::unique_ptr<SerpMetrics> CreateSerpMetrics(
    PrefService* local_state,
    const TimePeriodStoreFactory& time_period_store_factory);

// Convenience overload that creates a `SerpMetricsTimePeriodStoreFactory`
// internally and delegates to
// `CreateSerpMetrics(PrefService*, const TimePeriodStoreFactory&)`.
std::unique_ptr<SerpMetrics> CreateSerpMetrics(
    PrefService* local_state,
    const base::FilePath& profile_path,
    ProfileAttributesStorage& profile_attributes_storage);

}  // namespace serp_metrics

#endif  // BRAVE_BROWSER_SERP_METRICS_SERP_METRICS_FACTORY_H_
