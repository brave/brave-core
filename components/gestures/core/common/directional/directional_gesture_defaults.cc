// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/directional/directional_gesture_defaults.h"

#include "brave/components/gestures/core/common/actions/gesture_action_util.h"

namespace gestures {

// Chosen so the most commonly used actions are assigned to the simplest
// strokes.
DirectionalGestureMappings DefaultMappings(
    const DirectionalGestureDefaultCommandsInfo& default_commands) {
  return {
      {"↓", {MakeGestureAction(default_commands.new_tab)}},
      {"←", {MakeGestureAction(default_commands.back)}},
      {"→", {MakeGestureAction(default_commands.forward)}},
      {"↑", {MakeGestureAction(default_commands.scroll_tab_to_top)}},
      {"↓→", {MakeGestureAction(default_commands.close_tab)}},
      {"↓↑", {MakeGestureAction(default_commands.new_window)}},
      {"←↑", {MakeGestureAction(default_commands.previous_tab)}},
      {"→↓", {MakeGestureAction(default_commands.duplicate_tab)}},
      {"→↑", {MakeGestureAction(default_commands.next_tab)}},
      {"↑↓", {MakeGestureAction(default_commands.reload)}},
      {"↑→", {MakeGestureAction(default_commands.reopen_closed_tab)}},
  };
}

}  // namespace gestures
