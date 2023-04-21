/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_attention/user_idle_detection/user_idle_detection_feature.h"

namespace brave_ads {

BASE_FEATURE(kUserIdleDetectionFeature,
             "UserIdleDetection",
             base::FEATURE_ENABLED_BY_DEFAULT);

bool IsIdleDetectionFeatureEnabled() {
  return base::FeatureList::IsEnabled(kUserIdleDetectionFeature);
}

}  // namespace brave_ads
