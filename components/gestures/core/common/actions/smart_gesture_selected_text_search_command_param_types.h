// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_GESTURES_CORE_COMMON_ACTIONS_SMART_GESTURE_SELECTED_TEXT_SEARCH_COMMAND_PARAM_TYPES_H_
#define BRAVE_COMPONENTS_GESTURES_CORE_COMMON_ACTIONS_SMART_GESTURE_SELECTED_TEXT_SEARCH_COMMAND_PARAM_TYPES_H_

namespace gestures {

// Where to send the selected text `kSearchSelectedText` (see
// gesture_command_constants.h) captures. Persisted directly in prefs as
// `IntParamInfo` (see `kValueParamKey` in `int_param_util.h`), so, like
// `GestureActionInfo::command`, never renumber or reorder existing entries;
// only append. Kept in sync with
// `SmartGestureSelectedTextSearchCommandParamType` in
// `smart_gesture_selected_text_search_command_param.ts`.
enum class SmartGestureSelectedTextSearchCommandParamType {
  kBraveSearch = 0,  // The default; search.brave.com.
  kBraveAsk = 1,     // Brave's AI assistant (Leo).
};

}  // namespace gestures

#endif  // BRAVE_COMPONENTS_GESTURES_CORE_COMMON_ACTIONS_SMART_GESTURE_SELECTED_TEXT_SEARCH_COMMAND_PARAM_TYPES_H_
