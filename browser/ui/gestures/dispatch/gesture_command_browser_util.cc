// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/gestures/dispatch/gesture_command_browser_util.h"

#include "base/containers/fixed_flat_map.h"
#include "base/containers/map_util.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/app/command_utils.h"
#include "brave/browser/ui/gestures/dispatch/open_settings_page_web_ui_path_util.h"
#include "brave/components/gestures/core/common/actions/gesture_command_constants.h"
#include "brave/components/gestures/core/common/actions/int_param_info.h"
#include "brave/components/gestures/core/common/actions/open_settings_page_types.h"
#include "brave/components/gestures/core/common/actions/smart_gesture_open_in_command_param_types.h"
#include "brave/components/gestures/core/common/actions/smart_gesture_selected_text_search_command_param_types.h"
#include "brave/components/gestures/core/common/actions/text_param_info.h"
#include "chrome/app/chrome_command_ids.h"
#include "components/grit/brave_components_strings.h"
#include "ui/base/l10n/l10n_util.h"

namespace gestures {

std::optional<int> MaybeToGestureCommandId(int gesture_command_id) {
  switch (gesture_command_id) {
    case kUnassigned:
    case kNavigateToUrl:
    case kSwitchToTab:
    case kSearchSelectedText:
    case kOpenLinkIn:
    case kCopyLinkAddress:
    case kCopySelectedText:
    case kOpenImageIn:
    case kSaveImage:
    case kCopyImage:
      // Each of these is handled directly by `GestureActionDispatcher`
      // instead of going through `chrome::ExecuteCommand`.
      return std::nullopt;
    default:
      break;
  }
  // Any other value is a raw `IDC_*` command id; see
  // `gesture_command_constants.h`.
  return gesture_command_id;
}

std::u16string ToCommandDisplayName(int gesture_command_id) {
  // `IDC_OPTIONS` gets its own entry below, "Settings", since Brave never
  // calls this feature Chromium's default name, "Options".
  static constexpr auto kMessageIds = base::MakeFixedFlatMap<int, int>({
      {kUnassigned, IDS_BRAVE_GESTURES_COMMAND_UNASSIGNED},
      {kNavigateToUrl, IDS_BRAVE_GESTURES_COMMAND_NAVIGATE_TO_URL},
      {kSwitchToTab, IDS_BRAVE_GESTURES_COMMAND_SWITCH_TO_TAB},
      {kSearchSelectedText, IDS_BRAVE_GESTURES_COMMAND_SEARCH_SELECTED_TEXT},
      {kOpenLinkIn, IDS_BRAVE_GESTURES_COMMAND_OPEN_LINK_IN},
      {kCopyLinkAddress, IDS_BRAVE_GESTURES_COMMAND_COPY_LINK_ADDRESS},
      {kCopySelectedText, IDS_BRAVE_GESTURES_COMMAND_COPY_SELECTED_TEXT},
      {kOpenImageIn, IDS_BRAVE_GESTURES_COMMAND_OPEN_IMAGE_IN},
      {kSaveImage, IDS_BRAVE_GESTURES_COMMAND_SAVE_IMAGE},
      {kCopyImage, IDS_BRAVE_GESTURES_COMMAND_COPY_IMAGE},
      {IDC_OPTIONS, IDS_BRAVE_GESTURES_COMMAND_OPEN_SETTINGS_PAGE},
  });
  const int* message_id = base::FindOrNull(kMessageIds, gesture_command_id);
  if (!message_id) {
    // Any other value is a raw `IDC_*` command id..
    return base::UTF8ToUTF16(commands::GetCommandName(gesture_command_id));
  }
  return l10n_util::GetStringUTF16(*message_id);
}

namespace {

// The int-param suffix for `command`, e.g. "New window" or "Brave Ask", if
// it takes one; empty otherwise.
std::u16string ToIntParamDisplayName(int command, int int_param_value) {
  if (command == IDC_OPTIONS) {
    const std::string_view path =
        PageToWebUIPath(static_cast<OpenSettingsPageType>(int_param_value));
    return path.empty() ? u"" : base::UTF8ToUTF16(path);
  }
  if (command == kOpenLinkIn || command == kOpenImageIn) {
    static constexpr auto kOpenTargetMessageIds =
        base::MakeFixedFlatMap<SmartGestureOpenInCommandParamType, int>({
            {SmartGestureOpenInCommandParamType::kNewBackgroundTab,
             IDS_BRAVE_GESTURES_OPEN_TARGET_NEW_BACKGROUND_TAB},
            {SmartGestureOpenInCommandParamType::kNewForegroundTab,
             IDS_BRAVE_GESTURES_OPEN_TARGET_NEW_FOREGROUND_TAB},
            {SmartGestureOpenInCommandParamType::kNewWindow,
             IDS_BRAVE_GESTURES_OPEN_TARGET_NEW_WINDOW},
        });
    const int* message_id = base::FindOrNull(
        kOpenTargetMessageIds,
        static_cast<SmartGestureOpenInCommandParamType>(int_param_value));
    return message_id ? l10n_util::GetStringUTF16(*message_id) : u"";
  }
  if (command == kSearchSelectedText) {
    static constexpr auto kSearchTargetMessageIds =
        base::MakeFixedFlatMap<SmartGestureSelectedTextSearchCommandParamType,
                               int>({
            {SmartGestureSelectedTextSearchCommandParamType::kBraveSearch,
             IDS_BRAVE_GESTURES_SEARCH_TARGET_BRAVE_SEARCH},
            {SmartGestureSelectedTextSearchCommandParamType::kBraveAsk,
             IDS_BRAVE_GESTURES_SEARCH_TARGET_BRAVE_ASK},
        });
    const int* message_id = base::FindOrNull(
        kSearchTargetMessageIds,
        static_cast<SmartGestureSelectedTextSearchCommandParamType>(
            int_param_value));
    return message_id ? l10n_util::GetStringUTF16(*message_id) : u"";
  }
  return {};
}

}  // namespace

std::u16string ToActionsDisplayName(const GestureActions& actions) {
  std::u16string display_name;
  for (const GestureActionInfo& action : actions) {
    if (!display_name.empty()) {
      display_name += kActionsDisplayNameSeparator;
    }
    display_name += ToCommandDisplayName(action.command);
    if (const auto* text_param =
            std::get_if<TextParamInfo>(&action.command_params);
        text_param && !text_param->value.empty()) {
      display_name += u" (" + base::UTF8ToUTF16(text_param->value) + u")";
    }
    if (const auto* int_param =
            std::get_if<IntParamInfo>(&action.command_params)) {
      const std::u16string int_param_name =
          ToIntParamDisplayName(action.command, int_param->value);
      if (!int_param_name.empty()) {
        display_name += u" (" + int_param_name + u")";
      }
    }
  }
  return display_name;
}

}  // namespace gestures
