/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ATTENTION_IDLE_DETECTION_IDLE_DETECTION_FEATURES_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ATTENTION_IDLE_DETECTION_IDLE_DETECTION_FEATURES_H_

#include "base/feature_list.h"
#include "base/metrics/field_trial_params.h"

namespace base {
class TimeDelta;
}  // namespace base

namespace brave_ads {

BASE_DECLARE_FEATURE(kIdleDetectionFeature);

bool IsIdleDetectionEnabled();

constexpr base::FeatureParam<base::TimeDelta> kIdleTimeThreshold{
    &kIdleDetectionFeature, "idle_time_threshold", base::Seconds(5)};

constexpr base::FeatureParam<base::TimeDelta> kMaximumIdleTime{
    &kIdleDetectionFeature, "maximum_idle_time", base::Seconds(0)};

constexpr base::FeatureParam<bool> kShouldDetectScreenWasLocked{
    &kIdleDetectionFeature, "should_detect_screen_was_locked", false};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ATTENTION_IDLE_DETECTION_IDLE_DETECTION_FEATURES_H_
