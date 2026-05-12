// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_IOS_WEB_PUBLIC_JS_MESSAGING_JAVA_SCRIPT_FEATURE_H_
#define BRAVE_CHROMIUM_SRC_IOS_WEB_PUBLIC_JS_MESSAGING_JAVA_SCRIPT_FEATURE_H_

// Adding a new function that will allow JavaScriptFeature's to reply to
// window.prompt communications
#define GetFeatureRepliesToMessages    \
  GetFeatureRepliesToMessages() const; \
  virtual bool GetFeatureRepliesToPrompts
// Friending JSPromptCommunicationHandlerImpl so it can bind
// ScriptMessageReceivedWithReply.
#define JavaScriptContentWorld \
  JavaScriptContentWorld;      \
  friend class JSPromptCommunicationHandlerImpl

#include <ios/web/public/js_messaging/java_script_feature.h>  // IWYU pragma: export

#undef JavaScriptContentWorld
#undef GetFeatureRepliesToMessages

#endif  // BRAVE_CHROMIUM_SRC_IOS_WEB_PUBLIC_JS_MESSAGING_JAVA_SCRIPT_FEATURE_H_
