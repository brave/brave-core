// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/custom/test/custom_gesture_test_util.h"

#include <utility>

namespace gestures::test {

CustomGestureInfo MakeCustomGesture(const std::string& id,
                                    GesturePath path,
                                    GestureActions actions) {
  CustomGestureInfo custom_gesture;
  custom_gesture.id = id;
  custom_gesture.path = std::move(path);
  custom_gesture.actions = std::move(actions);
  return custom_gesture;
}

}  // namespace gestures::test
