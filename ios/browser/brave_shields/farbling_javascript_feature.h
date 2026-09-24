// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_BRAVE_SHIELDS_FARBLING_JAVASCRIPT_FEATURE_H_
#define BRAVE_IOS_BROWSER_BRAVE_SHIELDS_FARBLING_JAVASCRIPT_FEATURE_H_

#include "base/no_destructor.h"
#include "brave/ios/web/js_messaging/message_handler_token.h"
#include "brave/ios/web/js_messaging/randomized_message_handler_name.h"
#include "ios/web/public/js_messaging/java_script_feature.h"

namespace brave_shields {

class FarblingJavaScriptFeature : public web::JavaScriptFeature {
 public:
  static FarblingJavaScriptFeature* GetInstance();

  // web::JavaScriptFeature
  std::optional<std::string> GetScriptMessageHandlerName() const override;
  bool GetFeatureRepliesToPrompts() const override;
  void ScriptMessageReceivedWithReply(
      web::WebState* web_state,
      const web::ScriptMessage& message,
      ScriptMessageReplyCallback callback) override;

 private:
  friend class base::NoDestructor<FarblingJavaScriptFeature>;

  FeatureScript::PlaceholderReplacements GetReplacements();

  FarblingJavaScriptFeature();
  ~FarblingJavaScriptFeature() override;

  web::RandomizedMessageHandlerName handler_name_;
  web::MessageHandlerToken token_;
};

}  // namespace brave_shields

#endif  // BRAVE_IOS_BROWSER_BRAVE_SHIELDS_FARBLING_JAVASCRIPT_FEATURE_H_
