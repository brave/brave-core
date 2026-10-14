// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/custom/custom_gesture_recognizer.h"

#include <optional>

#include "base/memory/raw_ref.h"
#include "base/numerics/safe_conversions.h"
#include "brave/components/gestures/core/common/custom/custom_gesture_candidate_info.h"
#include "brave/components/gestures/core/common/custom/custom_gesture_candidate_scoring_util.h"
#include "brave/components/gestures/core/common/features.h"

namespace gestures {

CustomGestureRecognizer::CustomGestureRecognizer() = default;

CustomGestureRecognizer::~CustomGestureRecognizer() = default;

void CustomGestureRecognizer::Add(CustomGestureInfo custom_gesture) {
  Remove(custom_gesture.id);
  custom_gestures_.push_back(std::move(custom_gesture));
}

void CustomGestureRecognizer::Remove(std::string_view id) {
  std::erase_if(custom_gestures_,
                [&id](const CustomGestureInfo& custom_gesture) {
                  return custom_gesture.id == id;
                });
}

void CustomGestureRecognizer::Clear() {
  custom_gestures_.clear();
}

std::optional<CustomGestureMatchInfo>
CustomGestureRecognizer::MaybeMatchGesture(
    const GesturePath& gesture_path) const {
  const CustomGestureCandidates gesture_candidates =
      ScoreCustomGestures(custom_gestures_, gesture_path);
  const auto max_distance = base::saturated_cast<float>(
      features::kCustomGestureMaxMatchDistance.Get());
  const auto min_hit_score =
      base::saturated_cast<float>(features::kCustomGestureMinHitScore.Get());
  if (const CustomGestureCandidateInfo* best_gesture_candidate =
          BestCustomGestureCandidateWithin(gesture_candidates, max_distance,
                                           min_hit_score)) {
    return CustomGestureMatchInfo{raw_ref(*best_gesture_candidate->gesture),
                                  best_gesture_candidate->distance};
  }
  return std::nullopt;
}

}  // namespace gestures
