// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/gestures/gesture_command_browser_mapping.h"

#include "base/strings/utf_string_conversions.h"
#include "brave/app/command_utils.h"
#include "brave/components/gestures/core/common/actions/gesture_command_constants.h"
#include "components/grit/brave_components_strings.h"
#include "ui/base/l10n/l10n_util.h"

namespace gestures {

std::optional<int> MaybeToGestureCommandId(int command) {
  switch (command) {
    case kUnassigned:
    case kNavigateToUrl:
    case kScrollToTop:
    case kSwitchToTab:
      // Each of these is handled directly by `GestureActionDispatcher`
      // instead of going through `chrome::ExecuteCommand`.
      return std::nullopt;
    default:
      break;
  }
  // Any other value is a raw `IDC_*` command id; see
  // `gesture_command_constants.h`.
  return command;
}

std::u16string ToDisplayName(int command) {
  switch (command) {
    case kUnassigned:
      return l10n_util::GetStringUTF16(IDS_BRAVE_GESTURES_COMMAND_UNASSIGNED);
    case kNavigateToUrl:
      return l10n_util::GetStringUTF16(
          IDS_BRAVE_GESTURES_COMMAND_NAVIGATE_TO_URL);
    case kScrollToTop:
      return l10n_util::GetStringUTF16(
          IDS_BRAVE_GESTURES_COMMAND_SCROLL_TO_TOP);
    case kSwitchToTab:
      return l10n_util::GetStringUTF16(
          IDS_BRAVE_GESTURES_COMMAND_SWITCH_TO_TAB);
    default:
      break;
  }
  // Any other value is a raw `IDC_*` command id; see
  // `gesture_command_constants.h`.
  return base::UTF8ToUTF16(commands::GetCommandName(command));
}

}  // namespace gestures
