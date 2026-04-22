/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_SERP_METRICS_PROFILE_ATTRIBUTES_TIME_PERIOD_STORE_H_
#define BRAVE_BROWSER_SERP_METRICS_PROFILE_ATTRIBUTES_TIME_PERIOD_STORE_H_

#include <string>
#include <string_view>

#include "base/files/file_path.h"
#include "base/memory/raw_ref.h"
#include "brave/components/serp_metrics/time_period_storage/serp_metrics_time_period_store.h"

class ProfileAttributesStorage;

namespace base {
class ListValue;
}  // namespace base

namespace serp_metrics {

// Implementation of `SerpMetricsTimePeriodStore` that uses profile attributes
// for storage and has the following data structure:
// "serp_metrics": {
//   "metric_name_1": [time_period_values_1],
//   "metric_name_2": [time_period_values_2],
//   ...
// }
class ProfileAttributesTimePeriodStore final
    : public SerpMetricsTimePeriodStore {
 public:
  ProfileAttributesTimePeriodStore(
      const base::FilePath& profile_path,
      ProfileAttributesStorage& profile_attributes_storage,
      std::string_view metric_name);

  ProfileAttributesTimePeriodStore(const ProfileAttributesTimePeriodStore&) =
      delete;
  ProfileAttributesTimePeriodStore& operator=(
      const ProfileAttributesTimePeriodStore&) = delete;

  ~ProfileAttributesTimePeriodStore() override;

  // SerpMetricsTimePeriodStore:
  const base::ListValue* Get() override;
  void Set(base::ListValue list) override;
  void Clear() override;

 private:
  const base::FilePath profile_path_;
  const raw_ref<ProfileAttributesStorage> profile_attributes_storage_;
  const std::string metric_name_;
};

}  // namespace serp_metrics

#endif  // BRAVE_BROWSER_SERP_METRICS_PROFILE_ATTRIBUTES_TIME_PERIOD_STORE_H_
