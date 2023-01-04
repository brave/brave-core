/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/resources/behavioral/anti_targeting/anti_targeting_features.h"

#include "base/metrics/field_trial_params.h"

namespace ads::resource::features {

namespace {

constexpr char kFeatureName[] = "AntiTargeting";
constexpr char kFieldTrialParameterResourceVersion[] =
    "anti_targeting_resource_version";
constexpr int kDefaultResourceVersion = 1;

}  // namespace

BASE_FEATURE(kAntiTargeting, kFeatureName, base::FEATURE_ENABLED_BY_DEFAULT);

bool IsAntiTargetingEnabled() {
  return base::FeatureList::IsEnabled(kAntiTargeting);
}

int GetAntiTargetingResourceVersion() {
  return GetFieldTrialParamByFeatureAsInt(kAntiTargeting,
                                          kFieldTrialParameterResourceVersion,
                                          kDefaultResourceVersion);
}

}  // namespace ads::resource::features
