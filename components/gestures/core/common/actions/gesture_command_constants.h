// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_GESTURES_CORE_COMMON_ACTIONS_GESTURE_COMMAND_CONSTANTS_H_
#define BRAVE_COMPONENTS_GESTURES_CORE_COMMON_ACTIONS_GESTURE_COMMAND_CONSTANTS_H_

namespace gestures {

inline constexpr int kUnassigned = 0;
inline constexpr int kNavigateToUrl = 1;
inline constexpr int kSwitchToTab = 2;
inline constexpr int kSearchSelectedText = 3;
inline constexpr int kOpenLinkIn = 4;
inline constexpr int kCopyLinkAddress = 5;
inline constexpr int kCopySelectedText = 6;
inline constexpr int kOpenImageIn = 7;
inline constexpr int kSaveImage = 8;
inline constexpr int kCopyImage = 9;

// The highest id above; bump this whenever a new one is added.
inline constexpr int kMaxLocallyHandledGestureCommand = kCopyImage;

// Smallest `IDC_*` command id anywhere; kept in sync by inspection, not by
// including the `IDC_*` headers this component can't depend on.
inline constexpr int kMinRealCommandId = 30000;

}  // namespace gestures

#endif  // BRAVE_COMPONENTS_GESTURES_CORE_COMMON_ACTIONS_GESTURE_COMMAND_CONSTANTS_H_
