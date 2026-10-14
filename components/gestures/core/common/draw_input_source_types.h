// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_GESTURES_CORE_COMMON_DRAW_INPUT_SOURCE_TYPES_H_
#define BRAVE_COMPONENTS_GESTURES_CORE_COMMON_DRAW_INPUT_SOURCE_TYPES_H_

namespace gestures {

// Which input mechanism started an in-progress draw.
enum class DrawInputSourceType {
  kRightButtonDrag,
  // Named after `blink::WebInputEvent::kAltKey`, the cross-platform modifier
  // this checks. Labeled Alt/Option depending on platform.
  kAltLeftButtonDrag,
  kTouchDrag
};

}  // namespace gestures

#endif  // BRAVE_COMPONENTS_GESTURES_CORE_COMMON_DRAW_INPUT_SOURCE_TYPES_H_
