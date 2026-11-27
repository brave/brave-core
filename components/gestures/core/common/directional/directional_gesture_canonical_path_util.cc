// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/directional/directional_gesture_canonical_path_util.h"

#include "brave/components/gestures/core/common/directional/directional_gesture_vector_util.h"

namespace gestures {

GesturePath DirectionalGestureStrokesToCanonicalPath(
    const DirectionalGestureStrokes& strokes) {
  GesturePath gesture_path = {gfx::PointF()};
  gfx::PointF current_point;
  for (DirectionalGestureStrokeType stroke_type : strokes) {
    current_point += UnitVectorForDirection(stroke_type);
    gesture_path.push_back(current_point);
  }
  return gesture_path;
}

}  // namespace gestures
