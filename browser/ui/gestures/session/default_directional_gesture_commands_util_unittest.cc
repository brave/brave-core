// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/gestures/session/default_directional_gesture_commands_util.h"

#include "brave/app/brave_command_ids.h"
#include "chrome/app/chrome_command_ids.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace gestures {

TEST(DefaultDirectionalGestureCommandsUtilTest,
     ReturnsBrowserCommandIdsForDefaultDirectionalGestures) {
  EXPECT_THAT(
      DefaultDirectionalGestureCommands(),
      testing::FieldsAre(IDC_NEW_TAB, IDC_BACK, IDC_FORWARD, IDC_CLOSE_TAB,
                         IDC_NEW_WINDOW, IDC_SELECT_PREVIOUS_TAB,
                         IDC_DUPLICATE_TAB, IDC_SELECT_NEXT_TAB, IDC_RELOAD,
                         IDC_RESTORE_TAB, IDC_SCROLL_TAB_TO_TOP));
}

}  // namespace gestures
