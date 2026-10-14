// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_GESTURES_CORE_COMMON_CUSTOM_CUSTOM_GESTURE_INFO_H_
#define BRAVE_COMPONENTS_GESTURES_CORE_COMMON_CUSTOM_CUSTOM_GESTURE_INFO_H_

#include <string>
#include <vector>

#include "brave/components/gestures/core/common/actions/gesture_action_info.h"
#include "brave/components/gestures/core/common/gesture_path_types.h"

namespace gestures {

// A user-recorded custom gesture, a freeform traced path matched by shape
// rather than by the fixed 8-direction strokes `DirectionalGestureMapping`
// handles.
struct CustomGestureInfo {
  CustomGestureInfo();

  CustomGestureInfo(const CustomGestureInfo&);
  CustomGestureInfo& operator=(const CustomGestureInfo&);

  CustomGestureInfo(CustomGestureInfo&&) noexcept;
  CustomGestureInfo& operator=(CustomGestureInfo&&) noexcept;

  ~CustomGestureInfo();

  // Stable identifier for this drawing, used to find and remove it among
  // others when the user edits or deletes it from the settings UI.
  std::string id;

  // The raw traced points as recorded, compared against a newly-drawn path
  // after both are resampled and normalized.
  GesturePath path;

  // The actions run when this gesture matches, in order. Never empty once
  // loaded; a gesture with no action configured isn't persisted.
  GestureActions actions;
};

// All of a profile's saved custom gestures.
using CustomGestures = std::vector<CustomGestureInfo>;

}  // namespace gestures

#endif  // BRAVE_COMPONENTS_GESTURES_CORE_COMMON_CUSTOM_CUSTOM_GESTURE_INFO_H_
