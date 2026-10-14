// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_GESTURES_CORE_COMMON_TEST_GESTURE_PATH_TEST_UTIL_H_
#define BRAVE_COMPONENTS_GESTURES_CORE_COMMON_TEST_GESTURE_PATH_TEST_UTIL_H_

#include <cstddef>

#include "brave/components/gestures/core/common/gesture_path_types.h"

namespace gestures::test {

// Triangle (closed shape).
GesturePath MakeTriangleGesturePath();

// Zigzag (open shape).
GesturePath MakeZigzagGesturePath();

// Letter V (open shape).
GesturePath MakeLetterVGesturePath();

// Conjunction (open shape).
GesturePath MakeConjunctionGesturePath();

// Circle (closed shape).
GesturePath MakeCircleGesturePath();

// Spiral (closed shape).
GesturePath MakeSpiralGesturePath();

// Handwriting (open shape).
GesturePath MakeHandwritingGesturePath();

// Hook (open shape).
GesturePath MakeHookGesturePath();

// Rocket (closed shape).
GesturePath MakeRocketGesturePath();

// Diamond (closed shape).
GesturePath MakeDiamondGesturePath();

// Oval (closed shape).
GesturePath MakeOvalGesturePath();

// Left-open frame (open shape).
GesturePath MakeLeftOpenFrameGesturePath();

// Right-open frame (open shape).
GesturePath MakeRightOpenFrameGesturePath();

// Square arch (open shape).
GesturePath MakeSquareArchGesturePath();

// Intersection (open shape).
GesturePath MakeIntersectionGesturePath();

// Letter C (open shape).
GesturePath MakeLetterCGesturePath();

// Letter U (open shape).
GesturePath MakeLetterUGesturePath();

// Reversed C (open shape).
GesturePath MakeReversedCGesturePath();

// Angled corner (open shape).
GesturePath MakeAngledCornerGesturePath();

// Squiggle (open shape).
GesturePath MakeSquiggleGesturePath();

// Double peak (open shape).
GesturePath MakeDoublePeakGesturePath();

// Coil (open shape).
GesturePath MakeCoilGesturePath();

}  // namespace gestures::test

#endif  // BRAVE_COMPONENTS_GESTURES_CORE_COMMON_TEST_GESTURE_PATH_TEST_UTIL_H_
