// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_GESTURES_CORE_COMMON_CUSTOM_CUSTOM_GESTURE_HIT_SCORE_UTIL_H_
#define BRAVE_COMPONENTS_GESTURES_CORE_COMMON_CUSTOM_CUSTOM_GESTURE_HIT_SCORE_UTIL_H_

#include "brave/components/gestures/core/common/gesture_path_types.h"

namespace gestures {

// Highest `HitScore` that `candidate_gesture_path` reaches against
// `gesture_path` over any rotation/winding. Both must already be normalized.
float BestHitScore(const GesturePath& gesture_path,
                   const GesturePath& candidate_gesture_path,
                   bool candidate_gesture_path_is_closed,
                   float per_point_threshold);

// Average per-point score, 1.0 exact to 0.0 by `per_point_threshold`. Both
// paths must already be normalized, since the threshold assumes that scale.
float HitScore(const GesturePath& gesture_path,
               const GesturePath& candidate_gesture_path,
               float per_point_threshold);

}  // namespace gestures

#endif  // BRAVE_COMPONENTS_GESTURES_CORE_COMMON_CUSTOM_CUSTOM_GESTURE_HIT_SCORE_UTIL_H_
