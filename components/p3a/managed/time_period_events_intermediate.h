/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_P3A_MANAGED_TIME_PERIOD_EVENTS_INTERMEDIATE_H_
#define BRAVE_COMPONENTS_P3A_MANAGED_TIME_PERIOD_EVENTS_INTERMEDIATE_H_

#include <memory>
#include <string>
#include <vector>

#include "base/containers/flat_set.h"
#include "base/json/json_value_converter.h"
#include "base/memory/raw_ptr.h"
#include "base/values.h"
#include "brave/components/p3a/managed/remote_metric_intermediate.h"

class TimePeriodStorage;

namespace p3a {

class TimePeriodEventsIntermediateDefinition {
 public:
  TimePeriodEventsIntermediateDefinition();
  ~TimePeriodEventsIntermediateDefinition();

  TimePeriodEventsIntermediateDefinition(
      const TimePeriodEventsIntermediateDefinition&) = delete;
  TimePeriodEventsIntermediateDefinition& operator=(
      const TimePeriodEventsIntermediateDefinition&) = delete;
  TimePeriodEventsIntermediateDefinition(
      TimePeriodEventsIntermediateDefinition&&);

  static void RegisterJSONConverter(
      base::JSONValueConverter<TimePeriodEventsIntermediateDefinition>*
          converter);

  std::string storage_key;
  int period_days = 0;
  bool replace_today = false;
  bool report_highest = false;
  bool add_histogram_value = false;
  base::Value::List sources;
};

// Intermediate that counts or sums events from source intermediates over
// configurable time periods.
class TimePeriodEventsIntermediate : public RemoteMetricIntermediate {
 public:
  TimePeriodEventsIntermediate(
      TimePeriodEventsIntermediateDefinition definition,
      Delegate* delegate);
  ~TimePeriodEventsIntermediate() override;

  TimePeriodEventsIntermediate(const TimePeriodEventsIntermediate&) = delete;
  TimePeriodEventsIntermediate& operator=(const TimePeriodEventsIntermediate&) =
      delete;

  bool Init() override;
  base::Value Process() override;
  base::flat_set<std::string_view> GetStorageKeys() const override;
  void OnLastUsedProfilePrefsChanged(PrefService* profile_prefs) override;

 private:
  TimePeriodEventsIntermediateDefinition definition_;
  raw_ptr<TimePeriodStorage> storage_ = nullptr;
  std::vector<std::unique_ptr<RemoteMetricIntermediate>> source_intermediates_;
};

}  // namespace p3a

#endif  // BRAVE_COMPONENTS_P3A_MANAGED_TIME_PERIOD_EVENTS_INTERMEDIATE_H_
