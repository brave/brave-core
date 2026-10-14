// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/custom/custom_gesture_recognizer.h"

#include <limits>

#include "base/no_destructor.h"
#include "brave/components/gestures/core/common/custom/custom_gesture_geometry_util.h"
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

base::optional_ref<const CustomGestureInfo>
CustomGestureRecognizer::MatchCustomGesture(
    const std::vector<gfx::PointF>& path) const {
  if (path.size() < 2 || custom_gestures_.empty()) {
    return std::nullopt;
  }

  const size_t resample_point_count =
      features::kCustomGestureResamplePointCount.Get();
  const std::vector<gfx::PointF> normalized_path =
      ResampleAndNormalizePath(path, resample_point_count);

  float best_distance = std::numeric_limits<float>::max();
  const CustomGestureInfo* best_match = nullptr;
  for (const CustomGestureInfo& custom_gesture : custom_gestures_) {
    if (custom_gesture.stroke_points.size() < 2) {
      continue;
    }
    const std::vector<gfx::PointF> normalized_candidate =
        ResampleAndNormalizePath(custom_gesture.stroke_points,
                                 resample_point_count);
    const float distance =
        DistanceToCandidate(normalized_path, normalized_candidate,
                            IsClosedPath(custom_gesture.stroke_points));
    if (distance < best_distance) {
      best_distance = distance;
      best_match = &custom_gesture;
    }
  }

  if (best_distance > features::kCustomGestureMatchThreshold.Get()) {
    return std::nullopt;
  }
  return *best_match;
}

const std::vector<GestureActionInfo>& CustomGestureRecognizer::ActionsForMatch(
    const std::vector<gfx::PointF>& path) const {
  static const base::NoDestructor<std::vector<GestureActionInfo>> kNoActions;
  base::optional_ref<const CustomGestureInfo> custom_gesture =
      MatchCustomGesture(path);
  if (!custom_gesture) {
    return *kNoActions;
  }
  return custom_gesture->actions;
}

}  // namespace gestures
