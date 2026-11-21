// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_GESTURES_CORE_COMMON_GESTURE_MATCH_INFO_H_
#define BRAVE_COMPONENTS_GESTURES_CORE_COMMON_GESTURE_MATCH_INFO_H_

#include "brave/components/gestures/core/common/actions/gesture_action_info.h"
#include "brave/components/gestures/core/common/gesture_match_types.h"

namespace gestures {

// The outcome of resolving a drawn `gesture_path` against every gesture type.
struct GestureMatchInfo final {
  GestureMatchType match_type = GestureMatchType::kUnmatched;
  GestureActions actions;
};

}  // namespace gestures

#endif  // BRAVE_COMPONENTS_GESTURES_CORE_COMMON_GESTURE_MATCH_INFO_H_
