/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SERP_METRICS_TIME_PERIOD_STORAGE_SERP_METRICS_PREF_TIME_PERIOD_STORE_H_
#define BRAVE_COMPONENTS_SERP_METRICS_TIME_PERIOD_STORAGE_SERP_METRICS_PREF_TIME_PERIOD_STORE_H_

#include "base/memory/raw_ptr.h"
#include "brave/components/serp_metrics/time_period_storage/serp_metrics_time_period_store.h"

class PrefService;

namespace base {
class ListValue;
}  // namespace base

namespace serp_metrics {

// Implementation of `SerpMetricsTimePeriodStore` that uses `PrefService` for
// storage. Supports both direct list prefs and dictionary-based list prefs.
class SerpMetricsPrefTimePeriodStore : public SerpMetricsTimePeriodStore {
 public:
  // Constructor for direct list pref store.
  SerpMetricsPrefTimePeriodStore(PrefService* prefs, const char* pref_name);

  // Constructor for dictionary-based list pref store.
  SerpMetricsPrefTimePeriodStore(PrefService* prefs,
                                 const char* pref_name,
                                 const char* dict_key);

  ~SerpMetricsPrefTimePeriodStore() override;

  SerpMetricsPrefTimePeriodStore(const SerpMetricsPrefTimePeriodStore&) =
      delete;
  SerpMetricsPrefTimePeriodStore& operator=(
      const SerpMetricsPrefTimePeriodStore&) = delete;

  // SerpMetricsTimePeriodStore:
  const base::ListValue* Get() override;
  void Set(base::ListValue list) override;
  void Clear() override;

 private:
  const raw_ptr<PrefService> prefs_;
  const char* pref_name_ = nullptr;
  const char* dict_key_ = nullptr;
};

}  // namespace serp_metrics

#endif  // BRAVE_COMPONENTS_SERP_METRICS_TIME_PERIOD_STORAGE_SERP_METRICS_PREF_TIME_PERIOD_STORE_H_
