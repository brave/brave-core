// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_GESTURES_CORE_COMMON_CUSTOM_CUSTOM_GESTURE_CANDIDATE_INFO_H_
#define BRAVE_COMPONENTS_GESTURES_CORE_COMMON_CUSTOM_CUSTOM_GESTURE_CANDIDATE_INFO_H_

#include <limits>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "brave/components/gestures/core/common/custom/custom_gesture_info.h"

namespace gestures {

// One candidate's result from scoring against a traced gesture_path, used to
// pick the winner.
struct CustomGestureCandidateInfo final {
  raw_ptr<const CustomGestureInfo> gesture = nullptr;
  float hit_score = -1.0F;
  float distance = std::numeric_limits<float>::max();
  bool is_closed = false;
};

using CustomGestureCandidates = std::vector<CustomGestureCandidateInfo>;

}  // namespace gestures

#endif  // BRAVE_COMPONENTS_GESTURES_CORE_COMMON_CUSTOM_CUSTOM_GESTURE_CANDIDATE_INFO_H_
