// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/custom/custom_gesture_recognizer.h"

#include <limits>
#include <vector>

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

CustomGestureMatchInfo CustomGestureRecognizer::MatchCustomGesture(
    const GesturePath& gesture_path) const {
  const std::vector<CustomGestureCandidateInfo> gesture_candidates =
      CustomGestureCandidates(custom_gestures_, gesture_path);
  const CustomGestureCandidateInfo* best_gesture_candidate =
      BestCustomGestureCandidateWithin(gesture_candidates,
                                       std::numeric_limits<float>::max());
  if (!best_gesture_candidate) {
    return {};
  }
  return {*best_gesture_candidate->custom_gesture,
          best_gesture_candidate->distance};
}

CustomGestureMatchInfo CustomGestureRecognizer::BestCustomGestureMatch(
    const GesturePath& gesture_path) const {
  const std::vector<CustomGestureCandidateInfo> gesture_candidates =
      CustomGestureCandidates(custom_gestures_, gesture_path);
  const CustomGestureCandidateInfo* best_gesture_candidate =
      BestCustomGestureCandidateWithin(
          gesture_candidates,
          base::saturated_cast<float>(
              features::kCustomGestureMaxMatchDistance.Get()));
  if (!best_gesture_candidate) {
    return {};
  }
  return {*best_gesture_candidate->custom_gesture,
          best_gesture_candidate->distance};
}

const GestureActions& CustomGestureRecognizer::ActionsForMatch(
    const GesturePath& gesture_path) const {
  static const base::NoDestructor<GestureActions> kNoGestureActions;
  const CustomGestureMatchInfo gesture_match =
      BestCustomGestureMatch(gesture_path);
  if (!gesture_match.custom_gesture) {
    return *kNoGestureActions;
  }
  return gesture_match.custom_gesture->actions;
}

}  // namespace gestures
