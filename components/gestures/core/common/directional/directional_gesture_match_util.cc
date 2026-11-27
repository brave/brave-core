// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/directional/directional_gesture_match_util.h"

#include "brave/components/gestures/core/common/directional/directional_gesture_canonical_path_util.h"
#include "brave/components/gestures/core/common/features.h"
#include "brave/components/gestures/core/common/gesture_path_matcher_util.h"
#include "brave/components/gestures/core/common/gesture_path_normalize_util.h"
#include "brave/components/gestures/core/common/gesture_path_resample_util.h"

namespace gestures {

namespace {

// How far the actual drawn `gesture_path` is from `strokes`' own idealized
// shape, both resampled/normalized the same way for a fair comparison.
float DirectionalMatchDistance(const DirectionalGestureStrokes& strokes,
                               const GesturePath& gesture_path) {
  const size_t custom_gesture_resample_point_count =
      features::kCustomGestureResamplePointCount.Get();
  const GesturePath resampled_gesture_path = ResampleGesturePathToPointCount(
      gesture_path, custom_gesture_resample_point_count);
  const GesturePath normalized_gesture_path =
      NormalizeGesturePathScaleAndTranslation(resampled_gesture_path);
  const GesturePath resampled_canonical_gesture_path =
      ResampleGesturePathToPointCount(
          DirectionalGestureStrokesToCanonicalPath(strokes),
          custom_gesture_resample_point_count);
  const GesturePath normalized_canonical_gesture_path =
      NormalizeGesturePathScaleAndTranslation(resampled_canonical_gesture_path);
  const GesturePath best_candidate_gesture_path_rotation =
      BestCandidateGesturePathRotation(
          normalized_gesture_path, normalized_canonical_gesture_path,
          /*candidate_gesture_path_is_closed=*/false);
  return AverageGesturePathDistance(normalized_gesture_path,
                                    best_candidate_gesture_path_rotation);
}

}  // namespace

std::optional<DirectionalGestureMatchInfo> MaybeMatchDirectionalGesture(
    const DirectionalGestureMapping& directional_gesture_mapping,
    const DirectionalGestureStrokes& strokes,
    const GesturePath& gesture_path) {
  const GestureActions& actions =
      directional_gesture_mapping.ActionsForStrokes(strokes);
  if (actions.empty()) {
    return std::nullopt;
  }
  const float distance = DirectionalMatchDistance(strokes, gesture_path);
  if (distance > features::kCustomGestureMaxMatchDistance.Get()) {
    return std::nullopt;
  }
  return DirectionalGestureMatchInfo{.actions = actions, .distance = distance};
}

}  // namespace gestures
