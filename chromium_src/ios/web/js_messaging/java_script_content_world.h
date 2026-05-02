/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_IOS_WEB_JS_MESSAGING_JAVA_SCRIPT_CONTENT_WORLD_H_
#define BRAVE_CHROMIUM_SRC_IOS_WEB_JS_MESSAGING_JAVA_SCRIPT_CONTENT_WORLD_H_

namespace web {
class BraveJavaScriptFeatureManager;
}  // namespace web

// `JavaScriptContentWorld` looks up `user_content_controller_` in its
// constructor from the *profile-level* `WKWebViewConfigurationProvider`. To
// make the per-tab `BraveJavaScriptFeatureManager` install scripts on the
// per-tab controller we give it friend access so it can overwrite the field
// right after construction. The friend declaration is injected on the private
// `user_content_controller_` field.
#define user_content_controller_ \
  user_content_controller_;      \
  friend class BraveJavaScriptFeatureManager
#include <ios/web/js_messaging/java_script_content_world.h>  // IWYU pragma: export
#undef user_content_controller_

#endif  // BRAVE_CHROMIUM_SRC_IOS_WEB_JS_MESSAGING_JAVA_SCRIPT_CONTENT_WORLD_H_
