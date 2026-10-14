// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_GESTURES_CORE_BROWSER_PREF_NAMES_H_
#define BRAVE_COMPONENTS_GESTURES_CORE_BROWSER_PREF_NAMES_H_

#include <string_view>

namespace gestures::prefs {

// Whether the gestures feature is enabled.
inline constexpr std::string_view kEnabled = "brave.gestures.enabled";

// Whether to show the cursor trail while a gesture is being drawn.
inline constexpr std::string_view kShowTrail = "brave.gestures.show_trail";

// Whether to show the currently-matched command's name next to the cursor
// while a gesture is being drawn.
inline constexpr std::string_view kShowMatchedGesture =
    "brave.gestures.show_matched_gesture";

// Minimum stroke distance in pixels, user-adjustable from 15 (still catches a
// deliberate short stroke) to 60 (large enough that incidental mouse movement
// during a right-button drag doesn't misfire a gesture).
inline constexpr std::string_view kSensitivityInPixels =
    "brave.gestures.sensitivity_in_pixels";

// Maps a gesture key, such as `←` for a single left stroke or `↓→` for a down
// stroke followed by a right stroke, to the actions run when it matches, in
// order. Syncs across devices.
inline constexpr std::string_view kDirectionalGestures =
    "brave.gestures.directional_gestures";

// Custom gestures the user has drawn, each with its traced path and the
// actions run when it matches, in order. Syncs across devices.
inline constexpr std::string_view kCustomGestures =
    "brave.gestures.custom_gestures";

// Whether rocker gestures (holding one mouse button and clicking the other)
// are enabled.
inline constexpr std::string_view kMouseRockerGesturesEnabled =
    "brave.gestures.mouse_rocker_gestures_enabled";

// Chained `kDirectionalGestures`-style list of actions run, in order, when
// the right button is held and the left button is clicked. Syncs across
// devices.
inline constexpr std::string_view kMouseRockerLeftAction =
    "brave.gestures.mouse_rocker_left_action";

// Chained `kDirectionalGestures`-style list of actions run, in order, when
// the left button is held and the right button is clicked. Syncs across
// devices.
inline constexpr std::string_view kMouseRockerRightAction =
    "brave.gestures.mouse_rocker_right_action";

// Whether wheel gestures (holding the modifier button and scrolling the
// wheel) are enabled.
inline constexpr std::string_view kMouseWheelGesturesEnabled =
    "brave.gestures.mouse_wheel_gestures_enabled";

// Which mouse button, held while scrolling, turns the wheel movement into a
// gesture. A `MouseWheelModifierButtonType` value.
inline constexpr std::string_view kMouseWheelModifierButton =
    "brave.gestures.mouse_wheel_modifier_button";

// Minimum accumulated scroll distance, in the same units as
// `WebMouseWheelEvent::delta_y`, before a wheel gesture fires, user-adjustable
// from 0 (fires on any scroll, matching behavior before this threshold
// existed) to 60 (requires a deliberate scroll). Lets a user with a precision
// trackpad or high-resolution wheel opt into requiring a more deliberate
// scroll, without changing the out-of-the-box behavior for anyone else.
inline constexpr std::string_view kMouseWheelSensitivityInPixels =
    "brave.gestures.mouse_wheel_sensitivity_in_pixels";

// Chained `kDirectionalGestures`-style list of actions run, in order, when
// the wheel scrolls up while the modifier button is held. Syncs across
// devices.
inline constexpr std::string_view kMouseWheelUpAction =
    "brave.gestures.mouse_wheel_up_action";

// Chained `kDirectionalGestures`-style list of actions run, in order, when
// the wheel scrolls down while the modifier button is held. Syncs across
// devices.
inline constexpr std::string_view kMouseWheelDownAction =
    "brave.gestures.mouse_wheel_down_action";

// Sites where gestures are disallowed. Syncs across devices.
inline constexpr std::string_view kDisallowedSites =
    "brave.gestures.disallowed_sites";

}  // namespace gestures::prefs

#endif  // BRAVE_COMPONENTS_GESTURES_CORE_BROWSER_PREF_NAMES_H_
