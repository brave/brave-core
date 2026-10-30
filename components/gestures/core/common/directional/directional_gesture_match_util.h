// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_GESTURES_CORE_COMMON_DIRECTIONAL_DIRECTIONAL_GESTURE_MATCH_UTIL_H_
#define BRAVE_COMPONENTS_GESTURES_CORE_COMMON_DIRECTIONAL_DIRECTIONAL_GESTURE_MATCH_UTIL_H_

#include <optional>

#include "brave/components/gestures/core/common/directional/directional_gesture_mapping.h"
#include "brave/components/gestures/core/common/directional/directional_gesture_match_info.h"
#include "brave/components/gestures/core/common/gesture_path_types.h"

namespace gestures {

// `strokes`' best match against `directional_gesture_mapping`, applying the
// match-distance threshold itself. `nullopt` if none qualifies.
std::optional<DirectionalGestureMatchInfo> MaybeMatchDirectionalGesture(
    const DirectionalGestureMapping& directional_gesture_mapping,
    const DirectionalGestureStrokes& strokes,
    const GesturePath& gesture_path);

}  // namespace gestures

#endif  // BRAVE_COMPONENTS_GESTURES_CORE_COMMON_DIRECTIONAL_DIRECTIONAL_GESTURE_MATCH_UTIL_H_
