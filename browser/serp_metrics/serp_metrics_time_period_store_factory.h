/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_SERP_METRICS_SERP_METRICS_TIME_PERIOD_STORE_FACTORY_H_
#define BRAVE_BROWSER_SERP_METRICS_SERP_METRICS_TIME_PERIOD_STORE_FACTORY_H_

#include <memory>

#include "base/files/file_path.h"
#include "base/memory/raw_ref.h"
#include "brave/components/time_period_storage/time_period_store_factory.h"

class ProfileAttributesStorage;
class TimePeriodStore;

namespace serp_metrics {

// A factory that creates SerpMetricsTimePeriodStore which uses profile
// attributes for storage.
class SerpMetricsTimePeriodStoreFactory : public TimePeriodStoreFactory {
 public:
  SerpMetricsTimePeriodStoreFactory(
      const base::FilePath& profile_path,
      ProfileAttributesStorage& profile_attributes_storage);

  std::unique_ptr<TimePeriodStore> Build(
      const char* metric_name) const override;

 private:
  const base::FilePath profile_path_;
  const raw_ref<ProfileAttributesStorage> profile_attributes_storage_;
};

}  // namespace serp_metrics

#endif  // BRAVE_BROWSER_SERP_METRICS_SERP_METRICS_TIME_PERIOD_STORE_FACTORY_H_
