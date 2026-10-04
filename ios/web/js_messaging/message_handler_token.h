// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_WEB_JS_MESSAGING_MESSAGE_HANDLER_TOKEN_H_
#define BRAVE_IOS_WEB_JS_MESSAGING_MESSAGE_HANDLER_TOKEN_H_

#include <optional>
#include <string>

#include "base/uuid.h"
#include "ios/web/public/js_messaging/java_script_feature.h"

namespace web {

class ScriptMessage;

// A composable helper that generates a stable, per-instance token for use with
// JavaScriptFeature subclasses.
//
// Hold an instance as a member of a JavaScriptFeature and provide
// FeatureScripts replacements callback with the GetPlaceholderReplacements call
// and use `GetValidatedScriptMessageBody` in your JavaScriptMessage's message
// received override.
//
// To send tokenized messages to the JavaScriptFeature from TypeScript, use
// `sendTokenizedWebKitMessage` or `sendTokenizedWebKitMessageWithReply` from
// `//brave/ios/js_messaging/resources/utils.js`
class MessageHandlerToken {
 public:
  using PlaceholderReplacements =
      web::JavaScriptFeature::FeatureScript::PlaceholderReplacements;

  // Returns the message passed in from `sendTokenizedWebKitMessage` if the
  // token matches or nullptr if the token is missing or invalid
  std::optional<const base::Value*> GetValidatedScriptMessageBody(
      const web::ScriptMessage& script_message);

  // Returns the FeatureScript placeholder replacements map that substitutes
  // the JavaScript-side placeholder (`gCrWebPlaceholderMessageHandlerToken`)
  PlaceholderReplacements GetPlaceholderReplacements() const;

 private:
  base::Uuid token_ = base::Uuid::GenerateRandomV4();
};

}  // namespace web

#endif  // BRAVE_IOS_WEB_JS_MESSAGING_MESSAGE_HANDLER_TOKEN_H_
