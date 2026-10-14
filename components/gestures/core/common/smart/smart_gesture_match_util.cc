// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/smart/smart_gesture_match_util.h"

#include "brave/components/gestures/core/common/features.h"
#include "brave/components/gestures/core/common/gesture_path_distance_util.h"
#include "brave/components/gestures/core/common/smart/smart_gesture_direction_util.h"

namespace gestures {

std::optional<GestureActions> MaybeMatchSmartGesture(
    const SmartGestureMapping& smart_gesture_mapping,
    base::optional_ref<const SmartGestureContentInfo> smart_gesture_content,
    const GesturePath& gesture_path) {
  if (!smart_gesture_content) {
    return std::nullopt;
  }
  // Below the minimum, a barely-moved cursor can't commit to a direction;
  // beyond it, the shape reads as directional/custom, not a smart swipe.
  const double gesture_path_length = GesturePathLength(gesture_path);
  if (gesture_path_length < features::kSmartGestureMinDistancePixels.Get() ||
      gesture_path_length > features::kSmartGestureMaxDistancePixels.Get()) {
    return std::nullopt;
  }
  const SmartGestureDirectionType smart_direction_type =
      ClassifySmartGestureDirection(gesture_path);
  GestureActions actions = smart_gesture_mapping.ActionsFor(
      smart_gesture_content->type, smart_direction_type);
  if (actions.empty()) {
    return std::nullopt;
  }
  return actions;
}

}  // namespace gestures
