/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/features/anti_targeting/anti_targeting_features.h"

#include "base/metrics/field_trial_params.h"

namespace ads {
namespace features {

namespace {
const char kFeatureName[] = "AntiTargeting";
const char kFieldTrialParameterResourceVersion[] =
    "anti_targeting_resource_version";
const int kDefaultResourceVersion = 1;
}  // namespace

const base::Feature kAntiTargeting{kFeatureName,
                                   base::FEATURE_ENABLED_BY_DEFAULT};

bool IsAntiTargetingEnabled() {
  return base::FeatureList::IsEnabled(kAntiTargeting);
}

int GetAntiTargetingResourceVersion() {
  return GetFieldTrialParamByFeatureAsInt(kAntiTargeting,
                                          kFieldTrialParameterResourceVersion,
                                          kDefaultResourceVersion);
}

}  // namespace features
}  // namespace ads
