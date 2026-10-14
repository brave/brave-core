// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/custom/custom_gesture_hit_score_util.h"

#include <algorithm>
#include <functional>
#include <numeric>

#include "base/numerics/safe_conversions.h"
#include "brave/components/gestures/core/common/gesture_path_rotation_util.h"

namespace gestures {

float HitScore(const GesturePath& gesture_path,
               const GesturePath& candidate_gesture_path,
               float per_point_threshold) {
  if (gesture_path.empty() || per_point_threshold <= 0) {
    return 0;
  }
  // Falls off linearly between a perfect point and `per_point_threshold`,
  // so a near-perfect point counts for more than a barely-within one.
  const float total_score = std::transform_reduce(
      gesture_path.cbegin(), gesture_path.cend(),
      candidate_gesture_path.cbegin(), 0.0F, std::plus<>(),
      [per_point_threshold](const gfx::PointF& point,
                            const gfx::PointF& candidate_point) {
        const float distance = (point - candidate_point).Length();
        return std::max(0.0F, 1.0F - distance / per_point_threshold);
      });
  return total_score / base::saturated_cast<float>(gesture_path.size());
}

float BestHitScore(const GesturePath& gesture_path,
                   const GesturePath& candidate_gesture_path,
                   bool candidate_gesture_path_is_closed,
                   float per_point_threshold) {
  if (!candidate_gesture_path_is_closed) {
    return HitScore(gesture_path, candidate_gesture_path, per_point_threshold);
  }

  // The best alignment by average distance isn't necessarily the best one
  // by hit score, so this searches independently.
  float best_hit_score = 0.0F;
  ForEachClosedGesturePathRotation(
      candidate_gesture_path,
      [&](const GesturePath& rotated_candidate_gesture_path) {
        best_hit_score =
            std::max(best_hit_score,
                     HitScore(gesture_path, rotated_candidate_gesture_path,
                              per_point_threshold));
      });
  return best_hit_score;
}

}  // namespace gestures
