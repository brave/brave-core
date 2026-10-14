// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_GESTURES_CORE_COMMON_DIRECTIONAL_DIRECTIONAL_GESTURE_MATCH_INFO_H_
#define BRAVE_COMPONENTS_GESTURES_CORE_COMMON_DIRECTIONAL_DIRECTIONAL_GESTURE_MATCH_INFO_H_

#include <limits>

#include "brave/components/gestures/core/common/actions/gesture_action_info.h"

namespace gestures {

// A match's actions and distance, before the threshold is applied, so
// callers can compare it against another candidate directly.
struct DirectionalGestureMatchInfo final {
  GestureActions actions;
  float distance = std::numeric_limits<float>::max();
};

}  // namespace gestures

#endif  // BRAVE_COMPONENTS_GESTURES_CORE_COMMON_DIRECTIONAL_DIRECTIONAL_GESTURE_MATCH_INFO_H_
