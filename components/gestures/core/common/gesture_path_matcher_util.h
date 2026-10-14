// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_GESTURES_CORE_COMMON_GESTURE_PATH_MATCHER_UTIL_H_
#define BRAVE_COMPONENTS_GESTURES_CORE_COMMON_GESTURE_PATH_MATCHER_UTIL_H_

#include "brave/components/gestures/core/common/gesture_path_types.h"

namespace gestures {

// Mean per-point distance between `lhs_gesture_path` and `rhs_gesture_path`,
// which must already have the same point count.
float AverageGesturePathDistance(const GesturePath& lhs_gesture_path,
                                 const GesturePath& rhs_gesture_path);

// `candidate_gesture_path`'s rotation/winding that best aligns with
// `gesture_path`, or itself unchanged if not closed.
GesturePath BestCandidateGesturePathRotation(
    const GesturePath& gesture_path,
    const GesturePath& candidate_gesture_path,
    bool candidate_gesture_path_is_closed);

// `AverageGesturePathDistance` between `gesture_path` and whichever rotation
// `BestCandidateGesturePathRotation` picks.
float BestCandidateGesturePathDistance(
    const GesturePath& gesture_path,
    const GesturePath& candidate_gesture_path,
    bool candidate_gesture_path_is_closed);

}  // namespace gestures

#endif  // BRAVE_COMPONENTS_GESTURES_CORE_COMMON_GESTURE_PATH_MATCHER_UTIL_H_
