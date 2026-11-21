// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_GESTURES_CORE_COMMON_GESTURE_MATCH_UTIL_H_
#define BRAVE_COMPONENTS_GESTURES_CORE_COMMON_GESTURE_MATCH_UTIL_H_

#include "base/types/optional_ref.h"
#include "brave/components/gestures/core/common/custom/custom_gesture_recognizer.h"
#include "brave/components/gestures/core/common/directional/directional_gesture_mapping.h"
#include "brave/components/gestures/core/common/gesture_match_info.h"
#include "brave/components/gestures/core/common/gesture_path_types.h"
#include "brave/components/gestures/core/common/smart/smart_gesture_content_info.h"
#include "brave/components/gestures/core/common/smart/smart_gesture_mapping.h"

namespace gestures {

// A smart gesture wins outright if it qualifies; otherwise whichever of
// directional/custom is closer and within its own threshold, if either.
GestureMatchInfo MatchGesture(
    const SmartGestureMapping& smart_gesture_mapping,
    base::optional_ref<const SmartGestureContentInfo> smart_gesture_content,
    const DirectionalGestureMapping& directional_gesture_mapping,
    const CustomGestureRecognizer& custom_gesture_recognizer,
    const DirectionalGestureStrokes& strokes,
    const GesturePath& gesture_path);

}  // namespace gestures

#endif  // BRAVE_COMPONENTS_GESTURES_CORE_COMMON_GESTURE_MATCH_UTIL_H_
