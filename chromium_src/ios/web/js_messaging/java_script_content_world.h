// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_IOS_WEB_JS_MESSAGING_JAVA_SCRIPT_CONTENT_WORLD_H_
#define BRAVE_CHROMIUM_SRC_IOS_WEB_JS_MESSAGING_JAVA_SCRIPT_CONTENT_WORLD_H_

// Add a new function `GetFeatureReplyingToPrompts` to the
// `JavaScriptContentWorld` type which will allow us to get a specific
// `JavaScriptFeature` based on a message handler name. We will use this to
// obtain the correct feature to call into when handling window prompts
#define AddFeature(...)                                 \
  AddFeature(__VA_ARGS__);                              \
  const JavaScriptFeature* GetFeatureReplyingToPrompts( \
      const std::string& handler_name)
#define features_ \
  features_;      \
  std::map<std::string, const JavaScriptFeature*> prompt_replying_features_

#include <ios/web/js_messaging/java_script_content_world.h>  // IWYU pragma: export

#undef features_
#undef AddFeature

#endif  // BRAVE_CHROMIUM_SRC_IOS_WEB_JS_MESSAGING_JAVA_SCRIPT_CONTENT_WORLD_H_
