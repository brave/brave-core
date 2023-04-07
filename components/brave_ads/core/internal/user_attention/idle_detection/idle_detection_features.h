/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ATTENTION_IDLE_DETECTION_IDLE_DETECTION_FEATURES_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ATTENTION_IDLE_DETECTION_IDLE_DETECTION_FEATURES_H_

#include "base/feature_list.h"

namespace base {
class TimeDelta;
}  // namespace base

namespace brave_ads::idle_detection::features {

BASE_DECLARE_FEATURE(kIdleDetection);

bool IsEnabled();
base::TimeDelta GetIdleTimeThreshold();
base::TimeDelta GetMaximumIdleTime();
bool ShouldDetectScreenWasLocked();

}  // namespace brave_ads::idle_detection::features

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ATTENTION_IDLE_DETECTION_IDLE_DETECTION_FEATURES_H_
