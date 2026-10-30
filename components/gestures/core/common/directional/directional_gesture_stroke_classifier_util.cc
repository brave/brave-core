// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/directional/directional_gesture_stroke_classifier_util.h"

#include <optional>

#include "brave/components/gestures/core/common/directional/directional_gesture_stroke_classifier.h"

namespace gestures {

DirectionalGestureStrokes ClassifyDirectionalGestureStrokes(
    const GesturePath& gesture_path,
    double min_stroke_pixels,
    double min_stroke_straightness,
    size_t max_strokes) {
  DirectionalGestureStrokes strokes;
  if (gesture_path.empty()) {
    return strokes;
  }

  DirectionalGestureStrokeClassifier stroke_classifier(
      gesture_path.front(), min_stroke_pixels, min_stroke_straightness);
  for (size_t i = 1; i < gesture_path.size() && strokes.size() < max_strokes;
       ++i) {
    if (std::optional<DirectionalGestureStrokeType> stroke_type =
            stroke_classifier.AddPoint(gesture_path.at(i), strokes)
                .committed_stroke_type) {
      strokes.push_back(*stroke_type);
    }
  }
  return strokes;
}

}  // namespace gestures
