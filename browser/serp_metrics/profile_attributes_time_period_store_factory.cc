/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/serp_metrics/profile_attributes_time_period_store_factory.h"

#include <memory>
#include <string_view>

#include "brave/browser/serp_metrics/profile_attributes_time_period_store.h"

namespace serp_metrics {

ProfileAttributesTimePeriodStoreFactory::
    ProfileAttributesTimePeriodStoreFactory(
        const base::FilePath& profile_path,
        ProfileAttributesStorage& profile_attributes_storage)
    : profile_path_(profile_path),
      profile_attributes_storage_(profile_attributes_storage) {}

std::unique_ptr<SerpMetricsTimePeriodStore>
ProfileAttributesTimePeriodStoreFactory::Build(
    std::string_view metric_name) const {
  return std::make_unique<ProfileAttributesTimePeriodStore>(
      profile_path_, profile_attributes_storage_.get(), metric_name);
}

}  // namespace serp_metrics
