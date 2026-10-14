// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/smart/smart_gesture_defaults.h"

#include "brave/components/gestures/core/common/actions/gesture_action_util.h"
#include "brave/components/gestures/core/common/actions/gesture_command_constants.h"
#include "brave/components/gestures/core/common/actions/int_param_info.h"
#include "brave/components/gestures/core/common/actions/smart_gesture_open_in_command_param_types.h"
#include "brave/components/gestures/core/common/actions/smart_gesture_selected_text_search_command_param_types.h"
#include "brave/components/gestures/core/common/smart/smart_gesture_content_type.h"
#include "brave/components/gestures/core/common/smart/smart_gesture_direction_util.h"
#include "brave/components/gestures/core/common/smart/smart_gesture_key_util.h"

namespace gestures {

namespace {

GestureActionInfo MakeOpenLinkGestureAction(
    SmartGestureOpenInCommandParamType command_param_type) {
  GestureActionInfo gesture_action = MakeGestureAction(kOpenLinkIn);
  gesture_action.command_params =
      IntParamInfo{.value = static_cast<int>(command_param_type)};
  return gesture_action;
}

GestureActionInfo MakeOpenImageGestureAction(
    SmartGestureOpenInCommandParamType command_param_type) {
  GestureActionInfo gesture_action = MakeGestureAction(kOpenImageIn);
  gesture_action.command_params =
      IntParamInfo{.value = static_cast<int>(command_param_type)};
  return gesture_action;
}

GestureActionInfo MakeSearchSelectedTextGestureAction(
    SmartGestureSelectedTextSearchCommandParamType command_param_type) {
  GestureActionInfo gesture_action = MakeGestureAction(kSearchSelectedText);
  gesture_action.command_params =
      IntParamInfo{.value = static_cast<int>(command_param_type)};
  return gesture_action;
}

}  // namespace

SmartGestureMappings DefaultSmartGestureMappings() {
  return {
      // Right is the one non-navigating action (copy), isolated from the
      // open destinations so they stay contiguous.
      {SmartGestureKey(SmartGestureContentType::kLink,
                       SmartGestureDirectionType::kUp),
       {MakeOpenLinkGestureAction(
           SmartGestureOpenInCommandParamType::kNewBackgroundTab)}},
      {SmartGestureKey(SmartGestureContentType::kLink,
                       SmartGestureDirectionType::kDown),
       {MakeOpenLinkGestureAction(
           SmartGestureOpenInCommandParamType::kNewWindow)}},
      {SmartGestureKey(SmartGestureContentType::kLink,
                       SmartGestureDirectionType::kLeft),
       {MakeOpenLinkGestureAction(
           SmartGestureOpenInCommandParamType::kNewForegroundTab)}},
      {SmartGestureKey(SmartGestureContentType::kLink,
                       SmartGestureDirectionType::kRight),
       {MakeGestureAction(kCopyLinkAddress)}},

      // Mirrors link's up/down/right assignment. No 3rd destination exists
      // here, so left stays unassigned rather than duplicating one above.
      {SmartGestureKey(SmartGestureContentType::kSelectedText,
                       SmartGestureDirectionType::kUp),
       {MakeSearchSelectedTextGestureAction(
           SmartGestureSelectedTextSearchCommandParamType::kBraveSearch)}},
      {SmartGestureKey(SmartGestureContentType::kSelectedText,
                       SmartGestureDirectionType::kDown),
       {MakeSearchSelectedTextGestureAction(
           SmartGestureSelectedTextSearchCommandParamType::kBraveAsk)}},
      {SmartGestureKey(SmartGestureContentType::kSelectedText,
                       SmartGestureDirectionType::kRight),
       {MakeGestureAction(kCopySelectedText)}},

      // Mirrors link/text's up/down/right assignment; left saves it, the
      // one slot unique to images.
      {SmartGestureKey(SmartGestureContentType::kImage,
                       SmartGestureDirectionType::kUp),
       {MakeOpenImageGestureAction(
           SmartGestureOpenInCommandParamType::kNewBackgroundTab)}},
      {SmartGestureKey(SmartGestureContentType::kImage,
                       SmartGestureDirectionType::kDown),
       {MakeOpenImageGestureAction(
           SmartGestureOpenInCommandParamType::kNewWindow)}},
      {SmartGestureKey(SmartGestureContentType::kImage,
                       SmartGestureDirectionType::kLeft),
       {MakeGestureAction(kSaveImage)}},
      {SmartGestureKey(SmartGestureContentType::kImage,
                       SmartGestureDirectionType::kRight),
       {MakeGestureAction(kCopyImage)}},
  };
}

}  // namespace gestures
