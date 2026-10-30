// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_GESTURES_CORE_COMMON_SMART_SMART_GESTURE_CONTENT_TYPE_H_
#define BRAVE_COMPONENTS_GESTURES_CORE_COMMON_SMART_SMART_GESTURE_CONTENT_TYPE_H_

namespace gestures {

// Which content was found at a smart gesture's start point; mirrors
// `mojom::SmartGestureContent`'s union for use outside Mojo call sites.
enum class SmartGestureContentType {
  kUndefined,
  kLink,
  kImage,
  kSelectedText,
};

}  // namespace gestures

#endif  // BRAVE_COMPONENTS_GESTURES_CORE_COMMON_SMART_SMART_GESTURE_CONTENT_TYPE_H_
