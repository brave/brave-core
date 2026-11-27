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

// Minimum stroke distance in pixels, user-adjustable so incidental mouse
// movement during a right-button drag doesn't misfire a gesture.
inline constexpr std::string_view kDirectionalGestureSensitivityInPixels =
    "brave.gestures.directional_gesture_sensitivity_in_pixels";

// Maps a gesture key (e.g. `←` or `↓→`) to the actions run, in order.
// Syncs across devices.
inline constexpr std::string_view kDirectionalGestures =
    "brave.gestures.directional_gestures";

// Custom gestures the user has drawn, each with its traced gesture_path and the
// actions run when it matches, in order. Syncs across devices.
inline constexpr std::string_view kCustomGestures =
    "brave.gestures.custom_gestures";

// Maps a smart-gesture key (see `ToSmartGestureKey`, e.g. `"link:right"`) to
// the actions run when it matches, in order. Syncs across devices.
inline constexpr std::string_view kSmartGestures =
    "brave.gestures.smart_gestures";

// Whether rocker gestures (holding one mouse button and clicking the other)
// are enabled.
inline constexpr std::string_view kMouseRockerGesturesEnabled =
    "brave.gestures.mouse_rocker_gestures_enabled";

// Chained `kDirectionalGestures`-style actions run when the right button is
// held and the left button is clicked. Syncs across devices.
inline constexpr std::string_view kMouseRockerLeftAction =
    "brave.gestures.mouse_rocker_left_action";

// Chained `kDirectionalGestures`-style actions run when the left button is
// held and the right button is clicked. Syncs across devices.
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

// Minimum accumulated scroll distance (in `WebMouseWheelEvent::delta_y`
// units) before a wheel gesture fires; user-adjustable, 0 by default.
inline constexpr std::string_view kMouseWheelSensitivityInPixels =
    "brave.gestures.mouse_wheel_sensitivity_in_pixels";

// Chained `kDirectionalGestures`-style actions run when the wheel scrolls up
// while the modifier button is held. Syncs across devices.
inline constexpr std::string_view kMouseWheelUpAction =
    "brave.gestures.mouse_wheel_up_action";

// Chained `kDirectionalGestures`-style actions run when the wheel scrolls
// down while the modifier button is held. Syncs across devices.
inline constexpr std::string_view kMouseWheelDownAction =
    "brave.gestures.mouse_wheel_down_action";

// Sites where gestures are disallowed. Syncs across devices.
inline constexpr std::string_view kDisallowedSites =
    "brave.gestures.disallowed_sites";

}  // namespace gestures::prefs

#endif  // BRAVE_COMPONENTS_GESTURES_CORE_BROWSER_PREF_NAMES_H_
