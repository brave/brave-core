// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_GESTURES_CORE_COMMON_CUSTOM_TEST_CUSTOM_GESTURE_TEST_UTIL_H_
#define BRAVE_COMPONENTS_GESTURES_CORE_COMMON_CUSTOM_TEST_CUSTOM_GESTURE_TEST_UTIL_H_

#include <string>
#include <vector>

#include "brave/components/gestures/core/common/actions/gesture_action_info.h"
#include "brave/components/gestures/core/common/custom/custom_gesture_info.h"
#include "ui/gfx/geometry/point_f.h"

namespace gestures::test {

// Builds a `CustomGestureInfo` with the given actions.
CustomGestureInfo MakeCustomGesture(const std::string& id,
                                    std::vector<gfx::PointF> stroke_points,
                                    std::vector<GestureActionInfo> actions);

// An open, two-segment "V" shape: down then up.
std::vector<gfx::PointF> MakeVStrokePoints();

// A closed triangle traced starting at `start_vertex` and going all the way
// back around to it, so the same shape can be retraced starting from a
// different vertex while still closing the loop.
std::vector<gfx::PointF> MakeTriangleStrokePoints(size_t start_vertex);

}  // namespace gestures::test

#endif  // BRAVE_COMPONENTS_GESTURES_CORE_COMMON_CUSTOM_TEST_CUSTOM_GESTURE_TEST_UTIL_H_
