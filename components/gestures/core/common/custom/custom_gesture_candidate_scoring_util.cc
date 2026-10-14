// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/custom/custom_gesture_candidate_scoring_util.h"

#include "base/numerics/safe_conversions.h"
#include "brave/components/gestures/core/common/custom/custom_gesture_hit_score_util.h"
#include "brave/components/gestures/core/common/features.h"
#include "brave/components/gestures/core/common/gesture_path_distance_util.h"
#include "brave/components/gestures/core/common/gesture_path_matcher_util.h"
#include "brave/components/gestures/core/common/gesture_path_normalize_util.h"
#include "brave/components/gestures/core/common/gesture_path_resample_util.h"

namespace gestures {

CustomGestureCandidates ScoreCustomGestures(
    const CustomGestures& custom_gestures,
    const GesturePath& gesture_path) {
  CustomGestureCandidates gesture_candidates;
  if (gesture_path.size() < 2 || custom_gestures.empty()) {
    return gesture_candidates;
  }

  const size_t resample_point_count =
      features::kCustomGestureResamplePointCount.Get();
  const auto inlier_point_threshold = base::saturated_cast<float>(
      features::kCustomGestureInlierPointThreshold.Get());
  const GesturePath resampled_gesture_path =
      ResampleGesturePathToPointCount(gesture_path, resample_point_count);
  const GesturePath normalized_gesture_path =
      NormalizeGesturePathScaleAndTranslation(resampled_gesture_path);

  for (const CustomGestureInfo& custom_gesture : custom_gestures) {
    if (custom_gesture.gesture_path.size() < 2) {
      continue;
    }
    const GesturePath resampled_candidate_gesture_path =
        ResampleGesturePathToPointCount(custom_gesture.gesture_path,
                                        resample_point_count);
    const GesturePath normalized_candidate_gesture_path =
        NormalizeGesturePathScaleAndTranslation(
            resampled_candidate_gesture_path);
    const bool is_closed_gesture_path =
        IsClosedGesturePath(custom_gesture.gesture_path);
    const float best_hit_score =
        BestHitScore(normalized_gesture_path, normalized_candidate_gesture_path,
                     is_closed_gesture_path, inlier_point_threshold);
    const float distance = BestCandidateGesturePathDistance(
        normalized_gesture_path, normalized_candidate_gesture_path,
        is_closed_gesture_path);
    gesture_candidates.push_back(
        {&custom_gesture, best_hit_score, distance, is_closed_gesture_path});
  }
  return gesture_candidates;
}

const CustomGestureCandidateInfo* BestCustomGestureCandidateWithin(
    const CustomGestureCandidates& gesture_candidates,
    float max_distance,
    float min_hit_score) {
  const CustomGestureCandidateInfo* best_gesture_candidate = nullptr;
  for (const CustomGestureCandidateInfo& gesture_candidate :
       gesture_candidates) {
    if (gesture_candidate.distance > max_distance ||
        gesture_candidate.hit_score < min_hit_score) {
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
