// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_GESTURES_CORE_COMMON_DIRECTIONAL_TEST_DIRECTIONAL_GESTURE_DEFAULT_COMMANDS_TEST_CONSTANTS_H_
#define BRAVE_COMPONENTS_GESTURES_CORE_COMMON_DIRECTIONAL_TEST_DIRECTIONAL_GESTURE_DEFAULT_COMMANDS_TEST_CONSTANTS_H_

#include "brave/components/gestures/core/common/directional/directional_gesture_default_commands_info.h"

namespace gestures::test {

// Fake `IDC_*` ids; only matters that they're >= 30000.
inline constexpr DirectionalGestureDefaultCommandsInfo kDefaultCommands = {
    .new_tab = 1000001,
    .back = 1000002,
    .forward = 1000003,
    .close_tab = 1000004,
    .new_window = 1000005,
    .previous_tab = 1000006,
    .duplicate_tab = 1000007,
    .next_tab = 1000008,
    .reload = 1000009,
    .reopen_closed_tab = 1000010,
    .scroll_tab_to_top = 1000011,
};

inline constexpr int kFakeNewTab = kDefaultCommands.new_tab;
inline constexpr int kFakeBack = kDefaultCommands.back;
inline constexpr int kFakeForward = kDefaultCommands.forward;
inline constexpr int kFakeCloseTab = kDefaultCommands.close_tab;
inline constexpr int kFakeNewWindow = kDefaultCommands.new_window;
inline constexpr int kFakePreviousTab = kDefaultCommands.previous_tab;
inline constexpr int kFakeDuplicateTab = kDefaultCommands.duplicate_tab;
inline constexpr int kFakeNextTab = kDefaultCommands.next_tab;
inline constexpr int kFakeReload = kDefaultCommands.reload;
inline constexpr int kFakeReopenClosedTab = kDefaultCommands.reopen_closed_tab;
inline constexpr int kFakeScrollTabToTop = kDefaultCommands.scroll_tab_to_top;

}  // namespace gestures::test

#endif  // BRAVE_COMPONENTS_GESTURES_CORE_COMMON_DIRECTIONAL_TEST_DIRECTIONAL_GESTURE_DEFAULT_COMMANDS_TEST_CONSTANTS_H_
