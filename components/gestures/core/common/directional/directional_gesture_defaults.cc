// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/directional/directional_gesture_defaults.h"

#include "brave/components/gestures/core/common/actions/gesture_action_util.h"
#include "brave/components/gestures/core/common/actions/gesture_command_constants.h"

namespace gestures {

// Chosen so the most commonly used actions are assigned to the simplest
// strokes.
base::flat_map<std::string, std::vector<GestureActionInfo>, std::less<>>
DefaultMappings(const DirectionalGestureDefaultCommands& commands) {
  return {
      {"↓", {MakeGestureAction(commands.new_tab)}},
      {"←", {MakeGestureAction(commands.back)}},
      {"→", {MakeGestureAction(commands.forward)}},
      {"↑", {MakeGestureAction(kScrollToTop)}},
      {"↓→", {MakeGestureAction(commands.close_tab)}},
      {"↓↑", {MakeGestureAction(commands.new_window)}},
      {"←↑", {MakeGestureAction(commands.previous_tab)}},
      {"→↓", {MakeGestureAction(commands.duplicate_tab)}},
      {"→↑", {MakeGestureAction(commands.next_tab)}},
      {"↑↓", {MakeGestureAction(commands.reload)}},
      {"↑→", {MakeGestureAction(commands.reopen_closed_tab)}},
  };
}

}  // namespace gestures
