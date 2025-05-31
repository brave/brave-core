/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_P3A_MANAGED_PREF_METRIC_H_
#define BRAVE_COMPONENTS_P3A_MANAGED_PREF_METRIC_H_

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "base/functional/callback.h"
#include "base/json/json_value_converter.h"
#include "brave/components/p3a/managed/remote_metric.h"
#include "components/prefs/pref_change_registrar.h"

class PrefService;

namespace p3a {

struct PrefMetricDefinition {
  PrefMetricDefinition();
  ~PrefMetricDefinition();

  PrefMetricDefinition(const PrefMetricDefinition&) = delete;
  PrefMetricDefinition& operator=(const PrefMetricDefinition&) = delete;
  PrefMetricDefinition(PrefMetricDefinition&& other);

  static void RegisterJSONConverter(
      base::JSONValueConverter<PrefMetricDefinition>* converter);

  bool Validate() const;

  // Name of the preference to monitor
  std::string pref_name;

  // Map of preference values to metric values (buckets)
  base::Value::Dict value_map;

  // If true, monitor profile preferences. If false, use local state.
  bool use_profile_prefs = false;
};

// This remote metric class is used to report the value of a preference.
// A preference value to metric value mapping is used to generate the metric
// value.
class PrefMetric : public RemoteMetric {
 public:
  PrefMetric(PrefService* local_state,
             PrefMetricDefinition definition,
             base::RepeatingCallback<void(size_t)> update_callback);
  ~PrefMetric() override;

  PrefMetric(const PrefMetric&) = delete;
  PrefMetric& operator=(const PrefMetric&) = delete;

  // RemoteMetric implementation
  void HandleHistogramChange(std::string_view histogram_name,
                             size_t sample) override;
  std::vector<std::string_view> GetSourceHistogramNames() const override;
  std::optional<std::string_view> GetStorageKey() const override;
  void OnLastUsedProfilePrefsChanged(PrefService* profile_prefs) override;

 private:
  void UpdateMetric();

  raw_ptr<PrefService> current_prefs_ = nullptr;
  PrefMetricDefinition definition_;
  base::RepeatingCallback<void(size_t)> update_callback_;
  PrefChangeRegistrar pref_change_registrar_;
};

}  // namespace p3a

#endif  // BRAVE_COMPONENTS_P3A_MANAGED_PREF_METRIC_H_
