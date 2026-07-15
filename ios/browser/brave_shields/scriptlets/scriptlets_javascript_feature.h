// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_BRAVE_SHIELDS_SCRIPTLETS_SCRIPTLETS_JAVASCRIPT_FEATURE_H_
#define BRAVE_IOS_BROWSER_BRAVE_SHIELDS_SCRIPTLETS_SCRIPTLETS_JAVASCRIPT_FEATURE_H_

#include "base/no_destructor.h"
#include "brave/ios/web/js_messaging/message_handler_token.h"
#include "brave/ios/web/js_messaging/randomized_message_handler_name.h"
#include "ios/web/public/js_messaging/java_script_feature.h"

class ScriptletsJavaScriptFeature : public web::JavaScriptFeature {
 public:
  // This feature holds no state, so only a single static instance is ever
  // needed.
  static ScriptletsJavaScriptFeature* GetInstance();

  // JavaScriptFeature:
  std::optional<std::string> GetScriptMessageHandlerName() const override;
  bool GetFeatureRepliesToPrompts() const override;
  bool GetFeatureRepliesToMessages() const override;
  void ScriptMessageReceivedWithReply(
      web::WebState* web_state,
      const web::ScriptMessage& message,
      ScriptMessageReplyCallback callback) override;

 private:
  friend class base::NoDestructor<ScriptletsJavaScriptFeature>;

  ScriptletsJavaScriptFeature();
  ~ScriptletsJavaScriptFeature() override;

  // Returns the placeholder replacements used to randomize the script's
  // message handler name and token at injection time.
  FeatureScript::PlaceholderReplacements GetReplacements();

  web::RandomizedMessageHandlerName handler_name_;
  web::MessageHandlerToken token_;
};

#endif  // BRAVE_IOS_BROWSER_BRAVE_SHIELDS_SCRIPTLETS_SCRIPTLETS_JAVASCRIPT_FEATURE_H_
