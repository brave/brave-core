/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/serp_metrics/time_period_storage/serp_metrics_pref_time_period_store_factory.h"

#include <memory>
#include <string_view>

#include "brave/components/serp_metrics/time_period_storage/serp_metrics_pref_time_period_store.h"

namespace serp_metrics {

SerpMetricsPrefTimePeriodStoreFactory::SerpMetricsPrefTimePeriodStoreFactory(
    PrefService* prefs,
    std::string_view pref_name)
    : prefs_(prefs), pref_name_(pref_name) {}

std::unique_ptr<SerpMetricsTimePeriodStore>
SerpMetricsPrefTimePeriodStoreFactory::Build(std::string_view pref_key) const {
  return std::make_unique<SerpMetricsPrefTimePeriodStore>(prefs_.get(),
                                                          pref_name_, pref_key);
}

}  // namespace serp_metrics
