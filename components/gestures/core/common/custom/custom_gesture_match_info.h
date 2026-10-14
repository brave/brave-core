// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_GESTURES_CORE_COMMON_CUSTOM_CUSTOM_GESTURE_MATCH_INFO_H_
#define BRAVE_COMPONENTS_GESTURES_CORE_COMMON_CUSTOM_CUSTOM_GESTURE_MATCH_INFO_H_

#include "base/memory/raw_ref.h"
#include "brave/components/gestures/core/common/custom/custom_gesture_info.h"

namespace gestures {

// The best-matching custom gesture and its distance, so callers can compare
// it against another candidate directly.
struct CustomGestureMatchInfo final {
  const raw_ref<const CustomGestureInfo> gesture;
  float path_distance;
};

}  // namespace gestures

#endif  // BRAVE_COMPONENTS_GESTURES_CORE_COMMON_CUSTOM_CUSTOM_GESTURE_MATCH_INFO_H_
