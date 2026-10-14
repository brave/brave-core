// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/actions/smart_gesture_selected_text_search_command_param_util.h"

namespace gestures {

std::string_view ToSearchBaseUrl(
    SmartGestureSelectedTextSearchCommandParamType command_param_type) {
  return command_param_type ==
                 SmartGestureSelectedTextSearchCommandParamType::kAskBrave
             ? "https://search.brave.com/ask?source=gestures&q="
             : "https://search.brave.com/search?q=";
}

}  // namespace gestures
