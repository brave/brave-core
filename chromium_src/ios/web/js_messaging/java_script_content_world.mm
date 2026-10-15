// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <ios/web/js_messaging/java_script_content_world.mm>

namespace web {

const JavaScriptFeature* JavaScriptContentWorld::GetFeatureReplyingToPrompts(
    const std::string& handler_name) {
  auto it = prompt_replying_features_.find(handler_name);
  if (it == prompt_replying_features_.end()) {
    return nullptr;
  }
  return it->second;
}

}  // namespace web
