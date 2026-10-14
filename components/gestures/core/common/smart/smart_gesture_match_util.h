// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_GESTURES_CORE_COMMON_SMART_SMART_GESTURE_MATCH_UTIL_H_
#define BRAVE_COMPONENTS_GESTURES_CORE_COMMON_SMART_SMART_GESTURE_MATCH_UTIL_H_

#include <optional>

#include "base/types/optional_ref.h"
#include "brave/components/gestures/core/common/actions/gesture_action_info.h"
#include "brave/components/gestures/core/common/gesture_path_types.h"
#include "brave/components/gestures/core/common/smart/smart_gesture_content_info.h"
#include "brave/components/gestures/core/common/smart/smart_gesture_mapping.h"

namespace gestures {

// A smart gesture (the draw started on a link, image, or selection) takes
// priority over the directional/custom cascade. `nullopt` if none qualifies.
std::optional<GestureActions> MaybeMatchSmartGesture(
    const SmartGestureMapping& smart_gesture_mapping,
    base::optional_ref<const SmartGestureContentInfo> smart_gesture_content,
    const GesturePath& gesture_path);

}  // namespace gestures

#endif  // BRAVE_COMPONENTS_GESTURES_CORE_COMMON_SMART_SMART_GESTURE_MATCH_UTIL_H_
