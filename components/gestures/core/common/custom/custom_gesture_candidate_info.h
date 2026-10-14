// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_GESTURES_CORE_COMMON_CUSTOM_CUSTOM_GESTURE_CANDIDATE_INFO_H_
#define BRAVE_COMPONENTS_GESTURES_CORE_COMMON_CUSTOM_CUSTOM_GESTURE_CANDIDATE_INFO_H_

#include <limits>

#include "base/memory/raw_ptr.h"
#include "brave/components/gestures/core/common/custom/custom_gesture_info.h"

namespace gestures {

// One candidate's result from scoring against a traced path, used to pick
// the winner.
struct CustomGestureCandidateInfo {
  raw_ptr<const CustomGestureInfo> custom_gesture = nullptr;
  float hit_score = -1;
  float distance = std::numeric_limits<float>::max();
  bool is_closed = false;
};

}  // namespace gestures

#endif  // BRAVE_COMPONENTS_GESTURES_CORE_COMMON_CUSTOM_CUSTOM_GESTURE_CANDIDATE_INFO_H_
