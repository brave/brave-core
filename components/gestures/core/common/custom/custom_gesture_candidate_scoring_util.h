// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_GESTURES_CORE_COMMON_CUSTOM_CUSTOM_GESTURE_CANDIDATE_SCORING_UTIL_H_
#define BRAVE_COMPONENTS_GESTURES_CORE_COMMON_CUSTOM_CUSTOM_GESTURE_CANDIDATE_SCORING_UTIL_H_

#include "brave/components/gestures/core/common/custom/custom_gesture_candidate_info.h"
#include "brave/components/gestures/core/common/custom/custom_gesture_info.h"
#include "brave/components/gestures/core/common/gesture_path_types.h"

namespace gestures {

// Scores every saved custom gesture against `gesture_path`. Empty if
// `gesture_path` has fewer than two points or nothing is saved.
CustomGestureCandidates ScoreCustomGestures(
    const CustomGestures& custom_gestures,
    const GesturePath& gesture_path);

// Picks by `hit_score`, not average distance, so a shape matching a few
// distinctive points can beat one mediocre everywhere.
const CustomGestureCandidateInfo* BestCustomGestureCandidateWithin(
    const CustomGestureCandidates& gesture_candidates,
    float max_distance,
    float min_hit_score);

}  // namespace gestures

#endif  // BRAVE_COMPONENTS_GESTURES_CORE_COMMON_CUSTOM_CUSTOM_GESTURE_CANDIDATE_SCORING_UTIL_H_
