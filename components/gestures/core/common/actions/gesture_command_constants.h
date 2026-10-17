// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_GESTURES_CORE_COMMON_ACTIONS_GESTURE_COMMAND_CONSTANTS_H_
#define BRAVE_COMPONENTS_GESTURES_CORE_COMMON_ACTIONS_GESTURE_COMMAND_CONSTANTS_H_

namespace gestures {

// Which command a gesture triggers (see `GestureActionInfo::command`). A
// value outside these 4 is a raw browser `IDC_*` command id, resolved and
// dispatched by the `browser/` layer. This component must not depend on
// `IDC_*` headers, so it treats those ids as opaque.
//
// These 4 are reserved sentinels, not real browser commands, kept at fixed
// small values since real `IDC_*` ids never fall below 30000.
// `kNavigateToUrl` and `kSwitchToTab` take their own params (see
// `GestureActionInfo`); all 3 non-`kUnassigned` sentinels are dispatched
// directly by `GestureActionDispatcher` instead of through
// `chrome::ExecuteCommand`.
//
// Persisted directly in prefs and mirrored in the `GestureCommand`
// TypeScript enum in `gesture_command.ts`. Never renumber or reorder these.
inline constexpr int kUnassigned = 0;
inline constexpr int kNavigateToUrl = 1;
inline constexpr int kScrollToTop = 2;
inline constexpr int kSwitchToTab = 3;

}  // namespace gestures

#endif  // BRAVE_COMPONENTS_GESTURES_CORE_COMMON_ACTIONS_GESTURE_COMMAND_CONSTANTS_H_
