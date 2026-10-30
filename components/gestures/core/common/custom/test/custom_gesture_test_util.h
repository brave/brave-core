// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_GESTURES_CORE_COMMON_CUSTOM_TEST_CUSTOM_GESTURE_TEST_UTIL_H_
#define BRAVE_COMPONENTS_GESTURES_CORE_COMMON_CUSTOM_TEST_CUSTOM_GESTURE_TEST_UTIL_H_

#include <string>

#include "brave/components/gestures/core/common/actions/gesture_action_info.h"
#include "brave/components/gestures/core/common/custom/custom_gesture_info.h"
#include "brave/components/gestures/core/common/custom/custom_gesture_recognizer.h"

namespace gestures::test {

CustomGestureInfo MakeCustomGesture(const std::string& id,
                                    GesturePath gesture_path,
                                    GestureActions actions);

// The actions `custom_gesture_recognizer` would run for `gesture_path`, or
// empty if none match.
GestureActions CustomGestureActionsForPath(
    const CustomGestureRecognizer& custom_gesture_recognizer,
    const GesturePath& gesture_path);

}  // namespace gestures::test

#endif  // BRAVE_COMPONENTS_GESTURES_CORE_COMMON_CUSTOM_TEST_CUSTOM_GESTURE_TEST_UTIL_H_
