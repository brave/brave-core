// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/gesture_path_matcher_util.h"

#include <functional>
#include <limits>
#include <numeric>
#include <utility>

#include "base/numerics/safe_conversions.h"
#include "brave/components/gestures/core/common/gesture_path_rotation_util.h"

namespace gestures {

float AverageGesturePathDistance(const GesturePath& lhs_gesture_path,
                                 const GesturePath& rhs_gesture_path) {
  const float total_distance =
      std::transform_reduce(lhs_gesture_path.cbegin(), lhs_gesture_path.cend(),
                            rhs_gesture_path.cbegin(), 0.0F, std::plus<>(),
                            [](const gfx::PointF& lhs, const gfx::PointF& rhs) {
                              return (lhs - rhs).Length();
                            });
  return total_distance / base::saturated_cast<float>(lhs_gesture_path.size());
}

GesturePath BestCandidateGesturePathRotation(
    const GesturePath& gesture_path,
    const GesturePath& candidate_gesture_path,
    bool candidate_gesture_path_is_closed) {
  if (!candidate_gesture_path_is_closed) {
    return candidate_gesture_path;
  }

  float best_distance = std::numeric_limits<float>::max();
  GesturePath best_aligned_candidate = candidate_gesture_path;
  ForEachClosedGesturePathRotation(
      candidate_gesture_path, [&](GesturePath rotated_candidate_gesture_path) {
        const float distance = AverageGesturePathDistance(
            gesture_path, rotated_candidate_gesture_path);
        if (distance < best_distance) {
          best_distance = distance;
          best_aligned_candidate = std::move(rotated_candidate_gesture_path);
        }
      });
  return best_aligned_candidate;
}

float BestCandidateGesturePathDistance(
    const GesturePath& gesture_path,
    const GesturePath& candidate_gesture_path,
    bool candidate_gesture_path_is_closed) {
  return AverageGesturePathDistance(
      gesture_path,
      BestCandidateGesturePathRotation(gesture_path, candidate_gesture_path,
                                       candidate_gesture_path_is_closed));
}

}  // namespace gestures
