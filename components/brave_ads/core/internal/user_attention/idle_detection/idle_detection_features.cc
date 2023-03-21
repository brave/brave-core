/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_attention/idle_detection/idle_detection_features.h"

#include "base/metrics/field_trial_params.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/common/metrics/field_trial_params_util.h"

namespace brave_ads::idle_detection::features {

namespace {

constexpr char kIdleTimeThresholdFieldTrialParamName[] = "idle_time_threshold";
constexpr base::TimeDelta kIdleTimeThresholdDefaultValue = base::Seconds(5);

constexpr char kMaximumIdleTimeFieldTrialParamName[] = "maximum_idle_time";
constexpr base::TimeDelta kMaximumIdleTimeDefaultValue = base::Seconds(0);

constexpr char kShouldDetectScreenWasLockedFieldTrialParamName[] =
    "should_detect_was_locked";
constexpr bool kShouldDetectScreenWasLockedDefaultValue = false;

}  // namespace

BASE_FEATURE(kIdleDetection, "IdleDetection", base::FEATURE_ENABLED_BY_DEFAULT);

bool IsEnabled() {
  return base::FeatureList::IsEnabled(kIdleDetection);
}

base::TimeDelta GetIdleTimeThreshold() {
  return GetFieldTrialParamByFeatureAsTimeDelta(
      kIdleDetection, kIdleTimeThresholdFieldTrialParamName,
      kIdleTimeThresholdDefaultValue);
}

base::TimeDelta GetMaximumIdleTime() {
  return GetFieldTrialParamByFeatureAsTimeDelta(
      kIdleDetection, kMaximumIdleTimeFieldTrialParamName,
      kMaximumIdleTimeDefaultValue);
}

bool ShouldDetectScreenWasLocked() {
  return GetFieldTrialParamByFeatureAsBool(
      kIdleDetection, kShouldDetectScreenWasLockedFieldTrialParamName,
      kShouldDetectScreenWasLockedDefaultValue);
}

}  // namespace brave_ads::idle_detection::features
