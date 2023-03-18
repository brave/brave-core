/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_interaction/user_activity/user_activity_features.h"

#include "base/metrics/field_trial_params.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/common/metrics/field_trial_params_util.h"

namespace brave_ads::user_activity::features {

namespace {

constexpr char kTriggersFieldTrialParamName[] = "triggers";
constexpr char kTriggersDefaultValue[] =
    "0D0B14110D0B14110D0B14110D0B1411=-1.0;0D0B1411070707=-1.0;07070707=-1.0";

constexpr char kTimeWindowFieldTrialParamName[] = "time_window";
constexpr base::TimeDelta kTimeWindowDefaultValue = base::Minutes(15);

constexpr char kThresholdFieldTrialParamName[] = "threshold";
constexpr double kTresholdDefaultValue = 0.0;

}  // namespace

BASE_FEATURE(kFeature, "UserActivity", base::FEATURE_ENABLED_BY_DEFAULT);

bool IsEnabled() {
  return base::FeatureList::IsEnabled(kFeature);
}

std::string GetTriggers() {
  return GetFieldTrialParamByFeatureAsString(
      kFeature, kTriggersFieldTrialParamName, kTriggersDefaultValue);
}

base::TimeDelta GetTimeWindow() {
  return GetFieldTrialParamByFeatureAsTimeDelta(
      kFeature, kTimeWindowFieldTrialParamName, kTimeWindowDefaultValue);
}

double GetThreshold() {
  return GetFieldTrialParamByFeatureAsDouble(
      kFeature, kThresholdFieldTrialParamName, kTresholdDefaultValue);
}

}  // namespace brave_ads::user_activity::features
