// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/gestures/gesture_profile_prefs.h"

#include "brave/components/gestures/core/browser/mouse_rocker_default_commands.h"
#include "brave/components/gestures/core/browser/mouse_wheel_default_commands.h"
#include "brave/components/gestures/core/browser/prefs_registration.h"
#include "chrome/app/chrome_command_ids.h"

namespace gestures {

void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry) {
  RegisterProfilePrefs(
      registry,
      MouseRockerDefaultCommands{.left = IDC_BACK, .right = IDC_FORWARD},
      MouseWheelDefaultCommands{.up = IDC_SELECT_PREVIOUS_TAB,
                                .down = IDC_SELECT_NEXT_TAB});
}

}  // namespace gestures
