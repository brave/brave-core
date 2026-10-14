// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/custom/custom_gesture_candidate_scoring_util.h"

#include "base/numerics/safe_conversions.h"
#include "brave/components/gestures/core/common/features.h"
#include "brave/components/gestures/core/common/gesture_geometry_util.h"

namespace gestures {

std::vector<CustomGestureCandidateInfo> CustomGestureCandidates(
    const CustomGestures& custom_gestures,
    const GesturePath& gesture_path) {
  std::vector<CustomGestureCandidateInfo> gesture_candidates;
  if (gesture_path.size() < 2 || custom_gestures.empty()) {
    return gesture_candidates;
  }

  const size_t resample_point_count =
      features::kCustomGestureResamplePointCount.Get();
  const auto inlier_point_threshold = base::saturated_cast<float>(
      features::kCustomGestureInlierPointThreshold.Get());
  const GesturePath normalized_path =
      ResampleAndNormalizePath(gesture_path, resample_point_count);

  for (const CustomGestureInfo& custom_gesture : custom_gestures) {
    if (custom_gesture.path.size() < 2) {
      continue;
    }
    const GesturePath normalized_candidate_path =
        ResampleAndNormalizePath(custom_gesture.path, resample_point_count);
    const bool is_closed = IsClosedPath(custom_gesture.path);
    const float hit_score =
        BestHitScore(normalized_path, normalized_candidate_path, is_closed,
                     inlier_point_threshold);
    const float distance = DistanceToCandidate(
        normalized_path, normalized_candidate_path, is_closed);
    gesture_candidates.push_back(
        {&custom_gesture, hit_score, distance, is_closed});
  }
  return gesture_candidates;
}

const CustomGestureCandidateInfo* BestCustomGestureCandidateWithin(
    const std::vector<CustomGestureCandidateInfo>& gesture_candidates,
    float max_distance) {
  const CustomGestureCandidateInfo* best_gesture_candidate = nullptr;
  for (const CustomGestureCandidateInfo& gesture_candidate :
       gesture_candidates) {
    if (gesture_candidate.distance > max_distance) {
      continue;
    }
    if (!best_gesture_candidate ||
        gesture_candidate.hit_score > best_gesture_candidate->hit_score ||
        (gesture_candidate.hit_score == best_gesture_candidate->hit_score &&
         gesture_candidate.distance < best_gesture_candidate->distance)) {
      best_gesture_candidate = &gesture_candidate;
    }
  }
  return best_gesture_candidate;
}

}  // namespace gestures
