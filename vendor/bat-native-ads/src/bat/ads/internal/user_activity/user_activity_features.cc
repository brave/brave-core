/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/user_activity/user_activity_features.h"

#include "base/metrics/field_trial_params.h"
#include "base/time/time.h"
#include "bat/ads/internal/features/features_util.h"

namespace ads {
namespace features {
namespace user_activity {

namespace {

const char kFeatureName[] = "UserActivity";

const char kFieldTrialParameterTriggers[] = "triggers";
const char kDefaultTriggers[] = "01=.5;02=.5;08=1;09=1;0D=1;0E=1";
const char kFieldTrialParameterTimeWindow[] = "time_window";
const base::TimeDelta kDefaultTimeWindow = base::Hours(1);
const char kFieldTrialParameterThreshold[] = "threshold";
const double kDefaultTreshold = 2.0;

const char kFieldTrialParameterIdleTimeThreshold[] = "idle_time_threshold";
const base::TimeDelta kDefaultIdleTimeThreshold = base::Seconds(15);
const char kFieldTrialParameterMaximumIdleTime[] = "maximum_idle_time";
const base::TimeDelta kDefaultMaximumIdleTime = base::Seconds(0);

const char kFieldTrialParameterShouldDetectWasLocked[] =
    "should_detect_was_locked";
const bool kDefaultShouldDetectWasLocked = false;

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

bool ShouldDetectWasLocked() {
  return GetFieldTrialParamByFeatureAsBool(
      kFeature, kFieldTrialParameterShouldDetectWasLocked,
      kDefaultShouldDetectWasLocked);
}

}  // namespace user_activity
}  // namespace features
}  // namespace ads
