/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/serp_metrics/serp_metrics_all_profiles_aggregator_mock.h"

#include "chrome/browser/profiles/profile_attributes_storage.h"
#include "components/prefs/pref_service.h"

namespace serp_metrics {

SerpMetricsAllProfilesAggregatorMock::SerpMetricsAllProfilesAggregatorMock(
    PrefService* local_state,
    ProfileAttributesStorage& profile_attributes_storage)
    : SerpMetricsAllProfilesAggregator(local_state,
                                       profile_attributes_storage) {}

SerpMetricsAllProfilesAggregatorMock::~SerpMetricsAllProfilesAggregatorMock() =
    default;

}  // namespace serp_metrics
