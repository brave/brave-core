/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SERP_METRICS_TIME_PERIOD_STORAGE_SERP_METRICS_PREF_TIME_PERIOD_STORE_FACTORY_H_
#define BRAVE_COMPONENTS_SERP_METRICS_TIME_PERIOD_STORAGE_SERP_METRICS_PREF_TIME_PERIOD_STORE_FACTORY_H_

#include <memory>
#include <string>
#include <string_view>

#include "base/memory/raw_ptr.h"
#include "brave/components/serp_metrics/time_period_storage/serp_metrics_time_period_store_factory.h"

class PrefService;

namespace serp_metrics {

class SerpMetricsTimePeriodStore;

// A factory that creates `SerpMetricsPrefTimePeriodStore` which uses
// `PrefService` for storage.
class SerpMetricsPrefTimePeriodStoreFactory final
    : public SerpMetricsTimePeriodStoreFactory {
 public:
  SerpMetricsPrefTimePeriodStoreFactory(PrefService* prefs,
                                        std::string_view pref_name);

  SerpMetricsPrefTimePeriodStoreFactory(
      const SerpMetricsPrefTimePeriodStoreFactory&) = delete;
  SerpMetricsPrefTimePeriodStoreFactory& operator=(
      const SerpMetricsPrefTimePeriodStoreFactory&) = delete;

  ~SerpMetricsPrefTimePeriodStoreFactory() override = default;

  std::unique_ptr<SerpMetricsTimePeriodStore> Build(
      std::string_view pref_key) const override;

 private:
  const raw_ptr<PrefService> prefs_;
  const std::string pref_name_;
};

}  // namespace serp_metrics

#endif  // BRAVE_COMPONENTS_SERP_METRICS_TIME_PERIOD_STORAGE_SERP_METRICS_PREF_TIME_PERIOD_STORE_FACTORY_H_
