// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_WEB_JS_MESSAGING_PROTECTED_JAVASCRIPT_FEATURE_H_
#define BRAVE_IOS_WEB_JS_MESSAGING_PROTECTED_JAVASCRIPT_FEATURE_H_

#include "brave/ios/web/js_messaging/message_handler_token.h"
#include "brave/ios/web/js_messaging/randomized_message_handler_name.h"
#include "ios/web/public/js_messaging/java_script_feature.h"

namespace web {

class ProtectedJavaScriptFeature : public web::JavaScriptFeature {
 public:
  using PlaceholderReplacements =
      web::JavaScriptFeature::FeatureScript::PlaceholderReplacements;
  using JavaScriptFeature::JavaScriptFeature;

  ~ProtectedJavaScriptFeature() override;

  // web::JavaScriptFeature
  std::optional<std::string> GetScriptMessageHandlerName() const override;

 protected:
  std::optional<const base::Value*> GetValidatedScriptMessageBody(
      const web::ScriptMessage& script_message);

  PlaceholderReplacements GetPlaceholderReplacements() const;

 private:
  MessageHandlerToken token_;
  RandomizedMessageHandlerName handler_name_;
};

}  // namespace web

#endif  // BRAVE_IOS_WEB_JS_MESSAGING_PROTECTED_JAVASCRIPT_FEATURE_H_
