// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_WEB_JS_MESSAGING_RANDOMIZED_MESSAGE_HANDLER_NAME_H_
#define BRAVE_IOS_WEB_JS_MESSAGING_RANDOMIZED_MESSAGE_HANDLER_NAME_H_

#include <string>

#include "base/uuid.h"
#include "ios/web/public/js_messaging/java_script_feature.h"

namespace web {

// Composable helper that generates a stable, per-instance randomized script
// message handler name for use with JavaScriptFeature subclasses.
//
// Hold an instance as a member of a JavaScriptFeature and delegate
// GetScriptMessageHandlerName() and the FeatureScript placeholder replacements
// callback to it.
//
// To obtain the message handler name from JavaScript import
// `messageHandlerName` from `//brave/ios/web/js_messaging/utils.js`.
class RandomizedMessageHandlerName {
 public:
  using PlaceholderReplacements =
      web::JavaScriptFeature::FeatureScript::PlaceholderReplacements;

  // Returns the randomized message handler name for this instance.
  std::string GetScriptMessageHandlerName() const;

  // Returns the FeatureScript placeholder replacements map that substitutes
  // the JavaScript-side placeholder (`gCrWebPlaceholderMessageHandlerName`)
  // with the randomized name returned by GetScriptMessageHandlerName().
  PlaceholderReplacements GetPlaceholderReplacements() const;

 private:
  base::Uuid handler_name_ = base::Uuid::GenerateRandomV4();
};

}  // namespace web

#endif  // BRAVE_IOS_WEB_JS_MESSAGING_RANDOMIZED_MESSAGE_HANDLER_NAME_H_
