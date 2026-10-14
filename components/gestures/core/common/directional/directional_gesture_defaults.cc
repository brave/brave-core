// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/directional/directional_gesture_defaults.h"

#include "brave/components/gestures/core/common/actions/gesture_action_util.h"

namespace gestures {

// Chosen so the most commonly used actions are assigned to the simplest
// strokes.
base::flat_map<std::string, std::vector<GestureActionInfo>, std::less<>>
DefaultMappings() {
  return {
      {"↓", MakeGestureActions(GestureCommandType::kNewTab)},
      {"←", MakeGestureActions(GestureCommandType::kBack)},
      {"→", MakeGestureActions(GestureCommandType::kForward)},
      {"↑", MakeGestureActions(GestureCommandType::kScrollToTop)},
      {"↓→", MakeGestureActions(GestureCommandType::kCloseTab)},
      {"↓↑", MakeGestureActions(GestureCommandType::kNewWindow)},
      {"←↑", MakeGestureActions(GestureCommandType::kPreviousTab)},
      {"→↓", MakeGestureActions(GestureCommandType::kDuplicateTab)},
      {"→↑", MakeGestureActions(GestureCommandType::kNextTab)},
      {"↑↓", MakeGestureActions(GestureCommandType::kReload)},
      {"↑→", MakeGestureActions(GestureCommandType::kReopenClosedTab)},
  };
}

}  // namespace gestures
