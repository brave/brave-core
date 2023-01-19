/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/user_interaction/user_activity/user_activity_features.h"

#include "base/metrics/field_trial_params.h"
#include "base/time/time.h"
#include "bat/ads/internal/common/metrics/field_trial_params_util.h"

namespace ads::user_activity::features {

namespace {

constexpr char kFeatureName[] = "UserActivity";

constexpr char kFieldTrialParameterTriggers[] = "triggers";
constexpr char kDefaultTriggers[] =
    "0D0B14110D0B14110D0B14110D0B1411=-1.0;0D0B1411070707=-1.0;07070707=-1.0";
constexpr char kFieldTrialParameterTimeWindow[] = "time_window";
constexpr base::TimeDelta kDefaultTimeWindow = base::Minutes(15);
constexpr char kFieldTrialParameterThreshold[] = "threshold";
constexpr double kDefaultTreshold = 0.0;

constexpr char kFieldTrialParameterIdleTimeThreshold[] = "idle_time_threshold";
constexpr base::TimeDelta kDefaultIdleTimeThreshold = base::Seconds(5);
constexpr char kFieldTrialParameterMaximumIdleTime[] = "maximum_idle_time";
constexpr base::TimeDelta kDefaultMaximumIdleTime = base::Seconds(0);

constexpr char kFieldTrialParameterShouldDetectScreenWasLocked[] =
    "should_detect_was_locked";
constexpr bool kDefaultShouldDetectScreenWasLocked = false;

}  // namespace

BASE_FEATURE(kFeature, kFeatureName, base::FEATURE_ENABLED_BY_DEFAULT);

bool IsEnabled() {
  return base::FeatureList::IsEnabled(kFeature);
}

std::string GetTriggers() {
  return GetFieldTrialParamByFeatureAsString(
      kFeature, kFieldTrialParameterTriggers, kDefaultTriggers);
}

base::TimeDelta GetTimeWindow() {
  return GetFieldTrialParamByFeatureAsTimeDelta(
      kFeature, kFieldTrialParameterTimeWindow, kDefaultTimeWindow);
}

double GetThreshold() {
  return GetFieldTrialParamByFeatureAsDouble(
      kFeature, kFieldTrialParameterThreshold, kDefaultTreshold);
}

base::TimeDelta GetIdleTimeThreshold() {
  return GetFieldTrialParamByFeatureAsTimeDelta(
      kFeature, kFieldTrialParameterIdleTimeThreshold,
      kDefaultIdleTimeThreshold);
}

base::TimeDelta GetMaximumIdleTime() {
  return GetFieldTrialParamByFeatureAsTimeDelta(
      kFeature, kFieldTrialParameterMaximumIdleTime, kDefaultMaximumIdleTime);
}

bool ShouldDetectScreenWasLocked() {
  return GetFieldTrialParamByFeatureAsBool(
      kFeature, kFieldTrialParameterShouldDetectScreenWasLocked,
      kDefaultShouldDetectScreenWasLocked);
}

}  // namespace ads::user_activity::features
