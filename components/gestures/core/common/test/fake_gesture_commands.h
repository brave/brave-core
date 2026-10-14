// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_GESTURES_CORE_COMMON_TEST_FAKE_GESTURE_COMMANDS_H_
#define BRAVE_COMPONENTS_GESTURES_CORE_COMMON_TEST_FAKE_GESTURE_COMMANDS_H_

namespace gestures::test {

// Stands in for a real browser `IDC_*` command id in shape-matching tests,
// which only ever compare a saved gesture's action against itself.
inline constexpr int kFakeGestureCommand = 33000;
inline constexpr int kAnotherFakeGestureCommand = 33001;
inline constexpr int kYetAnotherFakeGestureCommand = 33002;

}  // namespace gestures::test

#endif  // BRAVE_COMPONENTS_GESTURES_CORE_COMMON_TEST_FAKE_GESTURE_COMMANDS_H_
