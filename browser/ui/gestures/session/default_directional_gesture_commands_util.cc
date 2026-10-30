// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/gestures/session/default_directional_gesture_commands_util.h"

#include "brave/app/brave_command_ids.h"
#include "chrome/app/chrome_command_ids.h"

namespace gestures {

DirectionalGestureDefaultCommandsInfo DefaultDirectionalGestureCommands() {
  return {
      .new_tab = IDC_NEW_TAB,
      .back = IDC_BACK,
      .forward = IDC_FORWARD,
      .close_tab = IDC_CLOSE_TAB,
      .new_window = IDC_NEW_WINDOW,
      .previous_tab = IDC_SELECT_PREVIOUS_TAB,
      .duplicate_tab = IDC_DUPLICATE_TAB,
      .next_tab = IDC_SELECT_NEXT_TAB,
      .reload = IDC_RELOAD,
      .reopen_closed_tab = IDC_RESTORE_TAB,
      .scroll_tab_to_top = IDC_SCROLL_TAB_TO_TOP,
  };
}

}  // namespace gestures
