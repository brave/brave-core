/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/user_interaction/user_activity/user_activity_features.h"

#include "base/metrics/field_trial_params.h"
#include "base/time/time.h"
#include "bat/ads/internal/base/metrics/field_trial_params_util.h"

namespace ads::user_activity::features {

namespace {

constexpr char kFeatureName[] = "UserActivity";

constexpr char kFieldTrialParameterTriggers[] = "triggers";
constexpr char kDefaultTriggers[] = "01=.5;02=.5;08=1;09=1;0D=1;0E=1";
constexpr char kFieldTrialParameterTimeWindow[] = "time_window";
constexpr base::TimeDelta kDefaultTimeWindow = base::Hours(1);
constexpr char kFieldTrialParameterThreshold[] = "threshold";
constexpr double kDefaultTreshold = 2.0;

constexpr char kFieldTrialParameterIdleTimeThreshold[] = "idle_time_threshold";
constexpr base::TimeDelta kDefaultIdleTimeThreshold = base::Seconds(15);
constexpr char kFieldTrialParameterMaximumIdleTime[] = "maximum_idle_time";
constexpr base::TimeDelta kDefaultMaximumIdleTime = base::Seconds(0);

constexpr char kFieldTrialParameterShouldDetectScreenWasLocked[] =
    "should_detect_was_locked";
constexpr bool kDefaultShouldDetectScreenWasLocked = false;

}  // namespace

const base::Feature kFeature{kFeatureName, base::FEATURE_ENABLED_BY_DEFAULT};

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
