// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/custom/test/custom_gesture_test_util.h"

#include <optional>
#include <utility>

#include "brave/components/gestures/core/common/custom/custom_gesture_match_info.h"

namespace gestures::test {

CustomGestureInfo MakeCustomGesture(const std::string& id,
                                    GesturePath gesture_path,
                                    GestureActions actions) {
  CustomGestureInfo custom_gesture;
  custom_gesture.id = id;
  custom_gesture.gesture_path = std::move(gesture_path);
  custom_gesture.actions = std::move(actions);
  return custom_gesture;
}

GestureActions CustomGestureActionsForPath(
    const CustomGestureRecognizer& custom_gesture_recognizer,
    const GesturePath& gesture_path) {
  std::optional<CustomGestureMatchInfo> gesture_match =
      custom_gesture_recognizer.MaybeMatchGesture(gesture_path);
  if (!gesture_match) {
    return {};
  }
  return gesture_match->gesture->actions;
}

}  // namespace gestures::test
