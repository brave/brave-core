/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SERP_METRICS_TIME_PERIOD_STORAGE_SERP_METRICS_TIME_PERIOD_STORE_FACTORY_H_
#define BRAVE_COMPONENTS_SERP_METRICS_TIME_PERIOD_STORAGE_SERP_METRICS_TIME_PERIOD_STORE_FACTORY_H_

#include <memory>
#include <string_view>

namespace serp_metrics {

class SerpMetricsTimePeriodStore;

// Factory which abstracts the creation of `SerpMetricsTimePeriodStore`
// instances.
class SerpMetricsTimePeriodStoreFactory {
 public:
  virtual ~SerpMetricsTimePeriodStoreFactory() = default;

  virtual std::unique_ptr<SerpMetricsTimePeriodStore> Build(
      std::string_view pref_key) const = 0;
};

}  // namespace serp_metrics

#endif  // BRAVE_COMPONENTS_SERP_METRICS_TIME_PERIOD_STORAGE_SERP_METRICS_TIME_PERIOD_STORE_FACTORY_H_
