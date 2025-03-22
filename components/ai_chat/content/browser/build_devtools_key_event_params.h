// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CONTENT_BROWSER_BUILD_DEVTOOLS_KEY_EVENT_PARAMS_H_
#define BRAVE_COMPONENTS_AI_CHAT_CONTENT_BROWSER_BUILD_DEVTOOLS_KEY_EVENT_PARAMS_H_

#include <string>
#include <string_view>

namespace ai_chat {

struct DevToolsKeyEventParams {
 public:
  int native_virtual_key_code;
  int windows_native_virtual_key_code;
  std::string dom_code_string;
  int modifiers = 0;
};

//
// Parses a string in xdotool-like syntax (e.g. "ctrl+shift+a") into
// a DevToolsKeyEventParams struct.
//
DevToolsKeyEventParams BuildDevToolsKeyEventParams(
    std::string_view xdotool_key);

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CONTENT_BROWSER_BUILD_DEVTOOLS_KEY_EVENT_PARAMS_H_
