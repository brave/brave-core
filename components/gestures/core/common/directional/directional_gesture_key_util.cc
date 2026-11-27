// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/directional/directional_gesture_key_util.h"

namespace gestures {

std::string_view ToDirectionalGestureStrokeGlyph(
    DirectionalGestureStrokeType stroke_type) {
  switch (stroke_type) {
    case DirectionalGestureStrokeType::kUp:
      return "↑";
    case DirectionalGestureStrokeType::kDown:
      return "↓";
    case DirectionalGestureStrokeType::kLeft:
      return "←";
    case DirectionalGestureStrokeType::kRight:
      return "→";
    case DirectionalGestureStrokeType::kUpLeft:
      return "↖";
    case DirectionalGestureStrokeType::kUpRight:
      return "↗";
    case DirectionalGestureStrokeType::kDownLeft:
      return "↙";
    case DirectionalGestureStrokeType::kDownRight:
      return "↘";
  }
}

std::string ToDirectionalGestureStrokesKey(
    const DirectionalGestureStrokes& strokes) {
  std::string key;
  for (DirectionalGestureStrokeType stroke_type : strokes) {
    key.append(ToDirectionalGestureStrokeGlyph(stroke_type));
  }
  return key;
}

}  // namespace gestures
