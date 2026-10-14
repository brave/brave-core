// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_GESTURES_CORE_COMMON_CUSTOM_CUSTOM_GESTURE_MATCH_INFO_H_
#define BRAVE_COMPONENTS_GESTURES_CORE_COMMON_CUSTOM_CUSTOM_GESTURE_MATCH_INFO_H_

#include <limits>

#include "base/types/optional_ref.h"
#include "brave/components/gestures/core/common/custom/custom_gesture_info.h"

namespace gestures {

// The best-matching custom gesture for a path and its distance, before the
// match threshold is applied. Exposed so callers that need to compare it
// against another candidate's distance on equal footing (e.g. a directional
// stroke's idealized template) can decide the winner themselves, rather than
// only getting a yes/no answer the way `ActionsForMatch` does.
struct CustomGestureMatchInfo {
  base::optional_ref<const CustomGestureInfo> custom_gesture;
  float distance = std::numeric_limits<float>::max();
};

}  // namespace gestures

#endif  // BRAVE_COMPONENTS_GESTURES_CORE_COMMON_CUSTOM_CUSTOM_GESTURE_MATCH_INFO_H_
